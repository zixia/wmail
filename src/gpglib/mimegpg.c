/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: mimegpg.c,v 1.1.1.1 2003/05/07 02:14:58 lfan Exp $";

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#if	HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <sys/types.h>
#if	HAVE_SYS_TIME_H
#include	<sys/time.h>
#endif
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#include "mimegpgheader.h"
#include "mimegpgstack.h"
#include "mimegpgfork.h"
#include "tempname.h"

char *passphrase_fd=0;

#define ENCODE_INDIVIDUAL	1
#define ENCODE_ENCAPSULATE	2

#define DECODE_CHECKSIGN	1
#define DECODE_UNENCRYPT	2

static int my_rewind(FILE *fp)
{
	if (fflush(fp) || ferror(fp) || fseek(fp, 0L, SEEK_SET))
		return (-1);
	clearerr(fp);
	return (0);
}

static void noexec(FILE *fp)
{
#ifdef FD_CLOEXEC
	if (fp)
		fcntl(fileno(fp), F_SETFD, FD_CLOEXEC);
#endif
}

/*
** Check if the line just read is a MIME boundary line.  Search the
** current MIME stack for a matching MIME boundary delimiter.
*/

static struct mimestack *is_boundary(struct mimestack *s, const char *line,
				     int *isclosing)
{
	struct mimestack *b;

	if (line[0] != '-' || line[1] != '-' ||
	    (b=search_mimestack(s, line+2)) == 0)
		return (NULL);


	*isclosing=strncmp(line+2+strlen(b->boundary), "--", 2) == 0;
	return (b);
}

static const char *get_boundary(struct mimestack *,
				const char *,
				FILE *);

/*
** Skip until EOF or a MIME boundary delimiter other than a closing MIME
** boundary delimiter.  After returning from bind_boundary we expect to
** see MIME headers.  Copy any intermediate lines to fpout.
*/

static void find_boundary(struct mimestack **stack, int *iseof,
			  FILE *fpin, FILE *fpout, int doappend)
{
	char buf[BUFSIZ];

	for (;;)
	{
		int is_closing;
		struct mimestack *b;

		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			return;
		}

		if (!(b=is_boundary(*stack, buf, &is_closing)))
		{
			if (fpout)
				fprintf(fpout, "%s", buf);
			if (strchr(buf, '\n') == 0)
			{
				int c;

				do
				{
					if ((c=getc(fpin)) == EOF)
					{
						*iseof=1;
						return;
					}
					if (fpout)
						putc(c, fpout);
				} while (c != '\n');
			}
			continue;
		}

		if (fpout)
			fprintf(fpout, "--%s%s\n", b->boundary,
				is_closing ? "--":"");
		if (is_closing)
		{
			pop_mimestack_to(stack, b);
			continue;
		}
		break;
	}
}

/*
** Read a set of headers.
*/

static struct header *read_headers(struct mimestack **stack, int *iseof,
				   FILE *fpin,
				   FILE *fpout,
				   int doappend)
{
	char buf[BUFSIZ];
	struct read_header_context rhc;
	struct header *h;

	init_read_header_context(&rhc);

	while (!*iseof)
	{
		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			break;
		}

		if (READ_START_OF_LINE(rhc))
		{
			struct mimestack *b;
			int is_closing;

			if (strcmp(buf, "\n") == 0
			    || strcmp(buf, "\r\n") == 0)
				break;

			b=is_boundary(*stack, buf, &is_closing);

			if (b)
			{
				/*
				** Corrupted MIME message.  We should NOT
				** see a MIME boundary in the middle of the
				** headers!
				**
				** Ignore this damage.
				*/

				struct header *p;

				h=finish_header(&rhc);

				for (p=h; p; p=p->next)
					fprintf(fpout, "%s", p->header);
				fprintf(fpout, "--%s%s", b->boundary,
					is_closing ? "--":"");
				if (is_closing)
				{
					pop_mimestack_to(stack, b);
					find_boundary(stack, iseof,
						      fpin, fpout, doappend);
				}
				free_header(h);

				init_read_header_context(&rhc);
				continue; /* From the top */
			}
		}
		read_header(&rhc, buf);
	}

	return (finish_header(&rhc));
}

/*
** Here we do actual signing/encoding
*/

static int dumpgpg(const char *, size_t, void *);

static int encode_header(const char *h)
{
	if (strncasecmp(h, "content-", 8) == 0)
		return (1);
	return (0);
}

static void dogpgencrypt(struct mimestack **stack,
			 struct header *h, int *iseof,
			 FILE *fpin, FILE *fpout,
			 int argc,
			 char **argv,
			 int dosign)
{
	struct header *hp;
	char buf[BUFSIZ];
	struct gpgmime_forkinfo gpg;
	int clos_flag=0;
	struct mimestack *b=0;
	int rc;
	const char *boundary;
	int need_crlf;

	boundary=get_boundary(*stack, "", NULL);

	if (gpgmime_fork_signencrypt(NULL,
				     (dosign ? GPG_SE_SIGN:0) | GPG_SE_ENCRYPT,
				     argc, argv,
				     &dumpgpg, fpout,
				     &gpg))
	{
		perror("fork");
		exit(1);
	}

	for (hp=h; hp; hp=hp->next)
	{
		if (encode_header(hp->header))
			continue;
		fprintf(fpout, "%s", hp->header);
	}

	fprintf(fpout, "Content-Type: multipart/encrypted;\n"
		"    boundary=\"%s\";\n"
		"    protocol=\"application/pgp-encrypted\"\n"
		"\n"
		"This is a mimegpg-encrypted message.  If you see this text, it means\n"
		"that your E-mail software does not support MIME formatted messages.\n"
		"\n--%s\n"
		"Content-Type: application/pgp-encrypted\n"
		"Content-Transfer-Encoding: 7bit\n"
		"\n"
		"Version: 1\n"
		"\n--%s\n"
		"Content-Type: application/octet-stream\n"
		"Content-Transfer-Encoding: 7bit\n\n",
		boundary, boundary, boundary);

	/* For Eudora compatiblity */
        gpgmime_write(&gpg, "Mime-Version: 1.0\r\n", 19);

	for (hp=h; hp; hp=hp->next)
	{
		const char *p;

		if (!encode_header(hp->header))
			continue;

		for (p=hp->header; *p; p++)
		{
			if (*p == '\r')
				continue;

			if (*p == '\n')
				gpgmime_write(&gpg, "\r\n", 2);
			else
				gpgmime_write(&gpg, p, 1);
		}
	}

	/*
	** Chew the content until the next MIME boundary.
	*/
	need_crlf=1;

	while (!*iseof)
	{
		const char *p;

		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			break;
		}

		if (need_crlf)
		{
			if ((b=is_boundary(*stack, buf, &clos_flag)) != NULL)
				break;

			gpgmime_write(&gpg, "\r\n", 2);
		}

		need_crlf=0;
		for (;;)
		{
			for (p=buf; *p; p++)
			{
				if (*p == '\r')
					continue;
				if (*p == '\n')
				{
					need_crlf=1;
					break;
				}

				gpgmime_write(&gpg, p, 1);
			}
			if (*p == '\n')
				break;

			if (fgets(buf, sizeof(buf), fpin) == NULL)
			{
				*iseof=1;
				break;
			}
		}
	}

	/*
	** This needs some 'splainin.  Note that we spit out a newline at
	** the BEGINNING of each line, above.  This generates the blank
	** header->body separator line.  Now, if we're NOT doing multiline
	** content, we need to follow the last line of the content with a
	** newline.  If we're already doing multiline content, that extra
	** newline (if it exists) is already there.
	*/

	if (!*stack)
	{
		gpgmime_write(&gpg, "\r\n", 2);
	}

	rc=gpgmime_finish(&gpg);

	if (rc)
	{
		fprintf(stderr, "%s",
			gpgmime_getoutput(&gpg));
		exit(1);
	}

	fprintf(fpout, "\n--%s--\n", boundary);

	if (*iseof)
		return;

	fprintf(fpout, "\n--%s%s\n", b->boundary, clos_flag ? "--":"");

	if (clos_flag)
	{
		pop_mimestack_to(stack, b);
		find_boundary(stack, iseof, fpin, fpout, 1);
	}
}

static void dogpgsign(struct mimestack **stack, struct header *h, int *iseof,
		      FILE *fpin, FILE *fpout,
		      int argc,
		      char **argv)
{
	struct header *hp;
	char buf[BUFSIZ];
	struct gpgmime_forkinfo gpg;
	int clos_flag=0;
	struct mimestack *b=0;
	int rc;
	char signed_content_name[TEMPNAMEBUFSIZE];
	int signed_content;
	FILE *signed_content_fp;
	const char *boundary;
	int need_crlf;

	for (hp=h; hp; hp=hp->next)
	{
		if (encode_header(hp->header))
			continue;
		fprintf(fpout, "%s", hp->header);
	}

	signed_content=mimegpg_tempfile(signed_content_name);
	if (signed_content < 0 ||
	    (signed_content_fp=fdopen(signed_content, "w+")) == NULL)
	{
		if (signed_content >= 0)
		{
			close(signed_content);
			unlink(signed_content_name);
		}
		perror("mktemp");
		exit(1);
	}
	noexec(signed_content_fp);
	unlink(signed_content_name);	/* UNIX semantics */

	for (hp=h; hp; hp=hp->next)
	{
		const char *p;

		if (!encode_header(hp->header))
			continue;

		for (p=hp->header; *p; p++)
		{
			if (*p == '\r')
				continue;

			if (*p == '\n')
				putc('\r', signed_content_fp);
			putc(*p, signed_content_fp);
		}
	}

	/*
	** Chew the content until the next MIME boundary.
	*/
	need_crlf=1;
	while (!*iseof)
	{
		const char *p;

		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			break;
		}

		if (need_crlf)
		{
			if ((b=is_boundary(*stack, buf, &clos_flag)) != NULL)
				break;

			fprintf(signed_content_fp, "\r\n");
		}

		need_crlf=0;
		for (;;)
		{
			for (p=buf; *p; p++)
			{
				if (*p == '\r')
					continue;
				if (*p == '\n')
				{
					need_crlf=1;
					break;
				}

				putc(*p, signed_content_fp);
			}
			if (*p == '\n')
				break;

			if (fgets(buf, sizeof(buf), fpin) == NULL)
			{
				*iseof=1;
				break;
			}
		}
	}

	/*
	** This needs some 'splainin.  Note that we spit out a newline at
	** the BEGINNING of each line, above.  This generates the blank
	** header->body separator line.  Now, if we're NOT doing multiline
	** content, we need to follow the last line of the content with a
	** newline.  If we're already doing multiline content, that extra
	** newline (if it exists) is already there.
	*/

	if (!*stack)
	{
		fprintf(signed_content_fp, "\r\n");
	}

	if (fflush(signed_content_fp) < 0 || ferror(signed_content_fp))
	{
		perror(signed_content_name);
		exit(1);
	}

	boundary=get_boundary(*stack, "", signed_content_fp);

	if (my_rewind(signed_content_fp) < 0)
	{
		perror(signed_content_name);
		exit(1);
	}

	if (gpgmime_fork_signencrypt(NULL, GPG_SE_SIGN,
				     argc, argv,
				     &dumpgpg, fpout,
				     &gpg))
	{
		perror("fork");
		exit(1);
	}

	fprintf(fpout, "Content-Type: multipart/signed;\n"
		"    boundary=\"%s\";\n"
		"    micalg=pgp-sha1;"
		" protocol=\"application/pgp-signature\"\n"
		"\n"
		"This is a mimegpg-signed message.  If you see this text, it means that\n"
		"your E-mail software does not support MIME-formatted messages.\n"
		"\n--%s\n",
		boundary, boundary);

	while (fgets(buf, sizeof(buf), signed_content_fp) != NULL)
	{
		const char *p;

		gpgmime_write(&gpg, buf, strlen(buf));

		for (p=buf; *p; p++)
			if (*p != '\r')
				putc(*p, fpout);
	}

	fprintf(fpout, "\n--%s\n"
		"Content-Type: application/pgp-signature\n"
		"Content-Transfer-Encoding: 7bit\n\n", boundary);

	rc=gpgmime_finish(&gpg);

	if (rc)
	{
		fprintf(stderr, "%s",
			gpgmime_getoutput(&gpg));
		exit(1);
	}

	fprintf(fpout, "\n--%s--\n", boundary);

	fclose(signed_content_fp);
	if (*iseof)
		return;

	fprintf(fpout, "\n--%s%s\n", b->boundary, clos_flag ? "--":"");

	if (clos_flag)
	{
		pop_mimestack_to(stack, b);
		find_boundary(stack, iseof, fpin, fpout, 1);
	}
}

static int dumpgpg(const char *p, size_t n, void *v)
{
	FILE *fp=(FILE *)v;

	while (n)
	{
		--n;
		putc(*p, fp);
		++p;
	}
	return (0);
}

static int isgpg(struct mime_header *);
static void checksign(struct mimestack **, int *, struct header *,
		      FILE *, FILE *, int, char **);
static void decrypt(struct mimestack **, int *, struct header *,
		    FILE *, FILE *, int, char **);

static void print_noncontent_headers(struct header *h, FILE *fpout)
{
	struct header *p;

	for (p=h; p; p=p->next)
	{
		if (strncasecmp(p->header, "content-", 8) == 0)
			continue;
		fprintf(fpout, "%s", p->header);
	}
}

static int dosignencode(int dosign, int doencode, int dodecode,
			FILE *fpin, FILE *fpout,
			int argc, char **argv)
{
	struct mimestack *boundary_stack=0;
	int iseof=0;

	while (!iseof)
	{
		static const char ct_s[]="content-type:";
		struct header *h=read_headers(&boundary_stack, &iseof, fpin,
					      fpout, dodecode ? 0:1),
			*hct;

		if (iseof && !h)
			continue;	/* Artifact */

		hct=find_header(h, ct_s);

		/*
		** If this is a multipart MIME section, we can keep on
		** truckin'.
		**
		*/

		if (hct)
		{
			struct mime_header *mh=
				parse_mime_header(hct->header+
						  (sizeof(ct_s)-1));

			const char *bv;

			if (strcasecmp(mh->header_name, "multipart/x-mimegpg")
			    == 0)
			{
				/* Punt */

				char *buf=malloc(strlen(hct->header)+100);
				const char *p;

				if (!buf)
				{
					free_mime_header(mh);
					free_header(h);
					perror("malloc");
					exit(1);
				}
				strcpy(buf, "Content-Type: multipart/mixed");
				p=strchr(hct->header, ';');
				strcat(buf, p ? p:"");
				free(hct->header);
				hct->header=buf;

				mh=parse_mime_header(hct->header+
						     sizeof(ct_s)-1);
			}

			if (strncasecmp(mh->header_name, "multipart/", 10)==0
			    && (bv=get_mime_attr(mh, "boundary")) != 0

			    && (doencode & ENCODE_ENCAPSULATE) == 0
			    )
			{
				struct header *p;

				push_mimestack(&boundary_stack, bv);

				if (dodecode)
				{
					if (strcasecmp(mh->header_name,
						       "multipart/signed")==0
					    && (dodecode & DECODE_CHECKSIGN)
					    && isgpg(mh))
					{
						print_noncontent_headers(h,
									 fpout
									 );
						free_mime_header(mh);
						checksign(&boundary_stack,
							  &iseof,
							  h, fpin, fpout,
							  argc, argv);
						free_header(h);
						continue;
					}

					if (strcasecmp(mh->header_name,
						       "multipart/encrypted")
					    ==0
					    && (dodecode & DECODE_UNENCRYPT)
					    && isgpg(mh))
					{
						print_noncontent_headers(h,
									 fpout
									 );
						free_mime_header(mh);
						decrypt(&boundary_stack,
							&iseof,
							h,
							fpin, fpout,
							argc, argv);
						free_header(h);
						continue;
					}
				}

				for (p=h; p; p=p->next)
				{
					fprintf(fpout, "%s", p->header);
				}

				putc('\n', fpout);
				free_header(h);
				free_mime_header(mh);

				find_boundary(&boundary_stack, &iseof, fpin,
					      fpout, dodecode ? 0:1);
				continue;
			}
			free_mime_header(mh);
		}

		if (dodecode)
		{
			struct header *p;
			int is_message_rfc822=0;

			for (p=h; p; p=p->next)
			{
				fprintf(fpout, "%s", p->header);
			}
			putc('\n', fpout);

			/*
			** If this is a message/rfc822 attachment, we can
			** resume reading the next set of headers.
			*/

			hct=find_header(h, ct_s);
			if (hct)
			{
				struct mime_header *mh=
					parse_mime_header(hct->header+
							  (sizeof(ct_s)-1));

				if (strcasecmp(mh->header_name,
					       "message/rfc822") == 0)
					is_message_rfc822=1;
				free_mime_header(mh);
			}
			free_header(h);

			if (!is_message_rfc822)
				find_boundary(&boundary_stack, &iseof,
					      fpin, fpout, 0);
			continue;
		}

		if (doencode)
			dogpgencrypt(&boundary_stack, h, &iseof,
				     fpin, fpout, argc, argv, dosign);
		else
			dogpgsign(&boundary_stack, h, &iseof,
				  fpin, fpout, argc, argv);
		free_header(h);
	}

	if (ferror(fpout))
		return (1);
	return (0);
}

static int isgpg(struct mime_header *mh)
{
	const char *attr;

	attr=get_mime_attr(mh, "protocol");

	if (!attr)
		return (0);

	if (strcasecmp(attr, "application/pgp-encrypted") == 0)
		return (1);

	if (strcasecmp(attr, "application/pgp-signature") == 0)
	{
		return (1);
	}
	return (0);
}

static int nybble(char c)
{
	static const char x[]="0123456789ABCDEFabcdef";

	const char *p=strchr(x, c);
	int n;

	if (!p) p=x;

	n= p-x;
	if (n >= 16)
		n -= 6;
	return (n);
}

/*
** Check signature
*/

static void dochecksign(struct mimestack *,
			FILE *,
			FILE *,
			const char *,
			const char *,
			int, char **);

static void checksign(struct mimestack **stack, int *iseof,
		      struct header *h,
		      FILE *fpin, FILE *fpout,
		      int argc, char **argv)
{
	char buf[BUFSIZ];
	struct header *h2;

	char signed_content[TEMPNAMEBUFSIZE];
	char signature[TEMPNAMEBUFSIZE];
	int signed_file, signature_file;
	FILE *signed_file_fp, *signature_file_fp;
	int clos_flag;
	int need_nl, check_boundary;
	struct mimestack *b=0;
	struct mime_header *mh;
	int qpdecode=0;

	signed_file=mimegpg_tempfile(signed_content);

	if (signed_file < 0 || (signed_file_fp=fdopen(signed_file, "w+")) == 0)
	{
		if (signed_file > 0)
		{
			close(signed_file);
			unlink(signed_content);
		}
		perror("open");
		exit(1);
	}
	noexec(signed_file_fp);

	find_boundary(stack, iseof, fpin, NULL, 0);
	if (*iseof)
		return;

	need_nl=0;
	check_boundary=1;

	while (!*iseof)
	{
		const char *p;

		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			continue;
		}

		if (check_boundary
		    && (b=is_boundary(*stack, buf, &clos_flag)) != 0)
			break;
		if (need_nl)
			fprintf(signed_file_fp, "\r\n");

		for (p=buf; *p && *p != '\n'; p++)
			putc(*p, signed_file_fp);
		need_nl=check_boundary= *p != 0;
	}

	if (my_rewind(signed_file_fp) < 0)
	{
		perror(signed_content);
		fclose(signed_file_fp);
		unlink(signed_content);
		exit(1);
	}

	if (clos_flag)
	{
		fclose(signed_file_fp);
		unlink(signed_content);
		if (b)
			pop_mimestack_to(stack, b);
		find_boundary(stack, iseof, fpin, fpout, 1);
		return;
	}

	h=read_headers(stack, iseof, fpin, fpout, 0);

	if (!h || !(h2=find_header(h, "content-type:")))
	{
		fclose(signed_file_fp);
		unlink(signed_content);
		if (!*iseof)
			find_boundary(stack, iseof, fpin, fpout, 1);
		return;
	}

	mh=parse_mime_header(h2->header+sizeof("content-type:")-1);

	if (!mh)
	{
		perror("malloc");
		free_header(h);
		fclose(signed_file_fp);
		unlink(signed_content);
		exit(1);
	}

	if (!mh || strcasecmp(mh->header_name, "application/pgp-signature"))
	{
		if (!mh)
			free_mime_header(mh);
		free_header(h);
		fclose(signed_file_fp);
		unlink(signed_content);
		if (!*iseof)
			find_boundary(stack, iseof, fpin, fpout, 1);
		return;
	}
	free_mime_header(mh);

	/*
	** In rare instances, the signature is qp-encoded.
	*/

	if ((h2=find_header(h, "content-transfer-encoding:")) != NULL)
	{
		mh=parse_mime_header(h2->header
				     +sizeof("content-transfer-encoding:")-1);
		if (!mh)
		{
			perror("malloc");
			free_header(h);
			fclose(signed_file_fp);
			unlink(signed_content);
			exit(1);
		}

		if (strcasecmp(mh->header_name,
			       "quoted-printable") == 0)
			qpdecode=1;
		free_mime_header(mh);
	}
	free_header(h);

	signature_file=mimegpg_tempfile(signature);

	if (signature_file < 0
	    || (signature_file_fp=fdopen(signature_file, "w+")) == 0)
	{
		if (signature_file > 0)
		{
			close(signature_file);
			unlink(signature);
		}
		fclose(signed_file_fp);
		unlink(signed_content);
		perror("open");
		exit(1);
	}

	while (!*iseof)
	{
		const char *p;

		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			continue;
		}

		if ((b=is_boundary(*stack, buf, &clos_flag)) != 0)
			break;

		for (p=buf; *p; p++)
		{
			int n;

			if (!qpdecode)
			{
				putc(*p, signature_file_fp);
				continue;
			}

			if (*p == '=' && p[1] == '\n')
				break;

			if (*p == '=' && p[1] && p[2])
			{
				n=nybble(p[1]) * 16 + nybble(p[2]);
				if ( (char)n )
				{
					putc((char)n, signature_file_fp);
					p += 2;
				}
				p += 2;
				continue;
			}
			putc(*p, signature_file_fp);

			/* If some spits out qp-lines > BUFSIZ, they deserve
			** this crap.
			*/
		}
	}

	fflush(signature_file_fp);
	if (ferror(signature_file_fp) || fclose(signature_file_fp))
	{
		unlink(signature);
		fclose(signed_file_fp);
		unlink(signed_content);
		perror("open");
		exit(1);
	}

	dochecksign(*stack, signed_file_fp, fpout, signed_content, signature,
		    argc, argv);

	fclose(signed_file_fp);
	unlink(signature);
	unlink(signed_content);

	fprintf(fpout, "\n--%s--\n", b->boundary);

	while (!clos_flag)
	{
		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			break;
		}
		if (!(b=is_boundary(*stack, buf, &clos_flag)))
			clos_flag=0;
	}
	if (b)
		pop_mimestack_to(stack, b);

	if (iseof)
		return;
}

static const char *newboundary()
{
	static char buffer[256];
	static unsigned counter=0;
	time_t t;
	char hostnamebuf[256];

	time(&t);
	hostnamebuf[sizeof(hostnamebuf)-1]=0;
	if (gethostname(hostnamebuf, sizeof(hostnamebuf)-1) < 0)
		hostnamebuf[0]=0;

	sprintf(buffer, "=_mimegpg-%1.128s-%u-%u-%04u",
		hostnamebuf, (unsigned)getpid(),
		(unsigned)t, ++counter);
	return (buffer);
}

static int good_boundary(const char *boundary,
			 struct mimestack *m, const char *errmsg, FILE *fp)
{
	int dummy;
	int l=strlen(boundary);
	const char *p;
	char buf[BUFSIZ];

	if (is_boundary(m, boundary, &dummy))
		return (0);

	for (p=errmsg; *p; )
	{
		if (*p == '-' && p[1] == '-' && strncasecmp(p+2, boundary, l)
		    == 0)
			return (0);

		while (*p)
			if (*p++ == '\n')
				break;
	}

	if (fp)
	{
		if (my_rewind(fp) < 0)
		{
			perror("fseek");
			exit(1);
		}

		while (fgets(buf, sizeof(buf), fp))
		{
			if (buf[0] == '-' && buf[1] == '-' &&
			    strncasecmp(buf+2, boundary, l) == 0)
				return (0);
		}
	}
	return (1);
}

static const char *get_boundary(struct mimestack *m,
				const char *errmsg,
				FILE *fp)
{
	const char *p;

	do
	{
		p=newboundary();
	} while (!good_boundary(p, m, errmsg, fp));
	return (p);
}

static const char *encoding_str(const char *p)
{
	while (*p)
	{
		if (*p <= 0 || *p >= 0x7F)
			return ("8bit");
		++p;
	}
	return ("7bit");
}

static void open_result_multipart(FILE *, int, const char *, const char *,
				  const char *);

static void dochecksign(struct mimestack *stack,
			FILE *content_fp,
			FILE *out_fp,
			const char *content_filename,
			const char *signature_filename,
			int argc,
			char **argv)
{
	struct gpgmime_forkinfo gpg;
	int i;
	const char *new_boundary;
	const char *output;

	if (gpgmime_forkchecksign(NULL, content_filename,
				  signature_filename,
				  argc, argv,
				  &gpg))
	{
		perror("fork");
		exit(1);
	}

	i=gpgmime_finish(&gpg);

	output=gpgmime_getoutput(&gpg);

	new_boundary=get_boundary(stack, output, content_fp);

	open_result_multipart(out_fp, i, new_boundary, output,
			      gpgmime_getcharset(&gpg));
	fprintf(out_fp, "\n--%s\n", new_boundary);
	if (my_rewind(content_fp) < 0)
	{
		perror("fseek");
		exit(1);
	}
	while ((i=getc(content_fp)) != EOF)
	{
		if (i == '\r')
			continue;
		putc(i, out_fp);
	}
	fprintf(out_fp, "\n--%s--\n", new_boundary);
}

static void open_result_multipart(FILE *fp, int rc,
				  const char *new_boundary,
				  const char *err_str,
				  const char *err_charset)
{
	fprintf(fp, "Content-Type: multipart/x-mimegpg; xpgpstatus=%d;"
		" boundary=\"%s\"\n"
		"\nThis is a mimegpg-processed message.  If you see this text, it means\n"
		"that your E-mail software does not support MIME-formatted messages.\n\n"
		"--%s\n"
		"Content-Type: text/x-gpg-output; charset=%s\n"
		"Content-Transfer-Encoding: %s\n\n%s",
		rc, new_boundary, new_boundary,
		err_charset,
		encoding_str(err_str),
		err_str);
}

static void close_mime(struct mimestack **stack, int *iseof,
		       FILE *fpin, FILE *fpout)
{
	char buf[BUFSIZ];
	int is_closing;
	struct mimestack *b;

	for (;;)
	{
		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			break;
		}

		fprintf(fpout, "%s", buf);
		if (!(b=is_boundary(*stack, buf, &is_closing)))
			continue;
		if (!is_closing)
			continue;

		pop_mimestack_to(stack, b);
		break;
	}
}

static void dodecrypt(struct mimestack **, int *,
		      FILE *, FILE *, int, char **, const char *,
		      FILE *);

static void decrypt(struct mimestack **stack, int *iseof,
		    struct header *h,
		    FILE *fpin, FILE *fpout, int argc, char **argv)
{
	struct header *p, *q;
	char temp_file[TEMPNAMEBUFSIZE];
	int temp_fd;
	FILE *temp_fp;
	struct mime_header *mh;
	int flag;

	temp_fd=mimegpg_tempfile(temp_file);
	if (temp_fd < 0 || (temp_fp=fdopen(temp_fd, "w+")) == 0)
	{
		if (temp_fd >= 0)
			close(temp_fd);
		perror("open");
		exit(1);
	}

	for (p=h; p; p=p->next)
	{
		fprintf(temp_fp, "%s", p->header);
	}
	putc('\n', temp_fp);

	find_boundary(stack, iseof, fpin, temp_fp, 0);
	if (*iseof)
	{
		fclose(temp_fp);
		unlink(temp_file);
		return;
	}

	p=read_headers(stack, iseof, fpin, temp_fp, 0);

	if (*iseof)
	{
		free_header(p);
		fclose(temp_fp);
		unlink(temp_file);
		return;
	}

	q=find_header(p, "content-type:");

	flag=0;

	if (q)
	{
		mh=parse_mime_header(q->header+13);
		if (!mh)
		{
			perror("malloc");
			free_header(p);
			fclose(temp_fp);
			unlink(temp_file);
			exit(1);
		}

		if (strcasecmp(mh->header_name, "application/pgp-encrypted")
		    == 0)
			flag=1;
		free_mime_header(mh);
	}

	for (q=p; q; q=q->next)
	{
		fprintf(temp_fp, "%s", q->header);
	}
	free_header(p);
	putc('\n', temp_fp);

	p=read_headers(stack, iseof, fpin, temp_fp, 0);

	if (*iseof)
	{
		free_header(p);
		fclose(temp_fp);
		unlink(temp_file);
		return;
	}

	q=find_header(p, "version:");

	if (flag)
	{
		if (!q || atoi(p->header + 8) != 1)
			flag=0;
	}
	for (q=p; q; q=q->next)
	{
		fprintf(temp_fp, "%s", q->header);
	}
	free_header(p);
	putc('\n', temp_fp);

	find_boundary(stack, iseof, fpin, temp_fp, 0);

	if (*iseof)
	{
		fclose(temp_fp);
		unlink(temp_file);
		return;
	}

	p=read_headers(stack, iseof, fpin, temp_fp, 0);

	if (*iseof)
	{
		free_header(p);
		fclose(temp_fp);
		unlink(temp_file);
		return;
	}

	q=find_header(p, "content-type:");

	if (q && flag)
	{
		flag=0;
		mh=parse_mime_header(q->header+13);
		if (!mh)
		{
			perror("malloc");
			free_header(p);
			fclose(temp_fp);
			unlink(temp_file);
			exit(1);
		}

		if (strcasecmp(mh->header_name, "application/octet-stream")
		    == 0)
			flag=1;
		free_mime_header(mh);

		q=find_header(p, "content-transfer-encoding:");
		if (q && flag)
		{
			flag=0;
			mh=parse_mime_header(strchr(q->header, ':')+1);
			if (!mh)
			{
				perror("malloc");
				free_header(p);
				fclose(temp_fp);
				unlink(temp_file);
				exit(1);
			}

			if (strcasecmp(mh->header_name, "7bit") == 0 ||
			    strcasecmp(mh->header_name, "8bit") == 0)
				flag=1;
			free_mime_header(mh);
		}
	}

	for (q=p; q; q=q->next)
	{
		fprintf(temp_fp, "%s", q->header);
	}
	free_header(p);
	putc('\n', temp_fp);

	if (fflush(temp_fp) || ferror(temp_fp) || my_rewind(temp_fp) < 0)
	{
		perror(temp_file);
		fclose(temp_fp);
		unlink(temp_file);
		exit(1);
	}

	if (!flag)
	{
		int c;

		while ((c=getc(temp_fp)) != EOF)
		{
			putc(c, fpout);
		}
		fclose(temp_fp);
		unlink(temp_file);
		close_mime(stack, iseof, fpin, fpout);
		return;
	}

	fclose(temp_fp);
	if ((temp_fp=fopen(temp_file, "w+")) == NULL)
	{
		perror(temp_file);
		unlink(temp_file);
		exit(1);
	}
	noexec(temp_fp);
	dodecrypt(stack, iseof, fpin, temp_fp, argc, argv, temp_file,
		  fpout);
	fclose(temp_fp);
	unlink(temp_file);
}

static int dumpdecrypt(const char *c, size_t n, void *vp)
{
	FILE *fp=(FILE *)vp;

	fwrite(c, n, 1, fp);
	return (0);
}

static void dodecrypt(struct mimestack **stack, int *iseof,
		      FILE *fpin, FILE *fpout, int argc, char **argv,
		      const char *temp_file, FILE *realout)
{
	struct gpgmime_forkinfo gpg;
	char buf[BUFSIZ];
	int is_closing;
	struct mimestack *b=NULL;
	int dowrite=1;
	int rc;
	const char *new_boundary;
	const char *output;

	if (gpgmime_forkdecrypt(NULL, argc, argv, &dumpdecrypt, fpout, &gpg))
	{
		perror("fork");
		exit(1);
	}

	for (;;)
	{
		if (fgets(buf, sizeof(buf), fpin) == NULL)
		{
			*iseof=1;
			break;
		}

		if (dowrite)
			gpgmime_write(&gpg, buf, strlen(buf));

		if (!(b=is_boundary(*stack, buf, &is_closing)))
			continue;
		dowrite=0;
		if (!is_closing)
			continue;
		break;
	}

	rc=gpgmime_finish(&gpg);
	if (fflush(fpout) || ferror(fpout) || my_rewind(fpout) < 0)
	{
		perror(temp_file);
		fclose(fpout);
		unlink(temp_file);
		exit(1);
	}

	if (*iseof)
		return;

	output=gpgmime_getoutput(&gpg),

	new_boundary=get_boundary(*stack, output, rc ? NULL:fpout);

	open_result_multipart(realout, rc, new_boundary,
			      output,
			      gpgmime_getcharset(&gpg));

	if (rc == 0)
	{
		int c;

		if (fseek(fpout, 0L, SEEK_SET) < 0)
		{
			perror(temp_file);
			fclose(fpout);
			unlink(temp_file);
			exit(1);
		}

		fprintf(realout, "\n--%s\n", new_boundary);

		while ((c=getc(fpout)) != EOF)
			if (c != '\r')
				putc(c, realout);
	}

	fprintf(realout, "\n--%s--\n", new_boundary);
	pop_mimestack_to(stack, b);
}

static void usage()
{
	fprintf(stderr, "Usage: mimegp [-s] [-e] [-c] [-d] -- [gpg options]\n");
}

static void reformime(const char *argv0)
{
	int reformime_pipe[2];
	pid_t p;
	int rc=0;
	int waitstat;

	if (pipe(reformime_pipe) < 0)
	{
		perror("pipe");
		exit(1);
	}

	p=fork();
	if (p < 0)
	{
		perror("fork");
		exit(1);
	}

	if (p == 0)
	{
		char *reformime_path;
		const char *p;

		close(1);
		dup(reformime_pipe[1]);
		close(reformime_pipe[0]);
		close(reformime_pipe[1]);

		if ((p=strrchr(argv0, '/')) != 0)
		{
			reformime_path=malloc(p - argv0
					      + sizeof("/reformime"));
			if (!reformime_path)
			{
				perror("malloc");
				exit(1);
			}
			memcpy(reformime_path, argv0, p-argv0);
			strcpy(reformime_path + (p-argv0), "/reformime");
			execl(reformime_path, "reformime", "-r7", (char *)0);
		}
		else
		{
			reformime_path="reformime";
			execlp(reformime_path, reformime_path, "-r7",
			       (char *)0);
		}
		perror(reformime_path);
		exit(1);
	}
	close(0);
	dup(reformime_pipe[0]);
	close(reformime_pipe[0]);
	close(reformime_pipe[1]);

	p=fork();

	if (p == 0)
		return;

	/* Make sure both child processes terminate cleanly. */

	close(0);
	close(1);

	for (;;)
	{
		if ((p=wait(&waitstat)) < 0 && errno == ECHILD)
			break;

		if (!WIFEXITED(waitstat) || WEXITSTATUS(waitstat))
			rc=1;
	}

	exit(rc);
}

int main(int argc, char **argv)
{
	int argn;
	int dosign=0;
	int doencode=0;
	int dodecode=0;
	pid_t reformime_pid;
	int rc;

	setlocale(LC_ALL, "C");

	for (argn=1; argn < argc; argn++)
	{
		if (strcmp(argv[argn], "--") == 0)
		{
			++argn;
			break;
		}

		if (strcmp(argv[argn], "-s") == 0)
		{
			dosign=1;
			continue;
		}

		if (strcmp(argv[argn], "-e") == 0)
		{
			doencode=ENCODE_INDIVIDUAL;
			continue;
		}

		if (strcmp(argv[argn], "-E") == 0)
		{
			doencode=ENCODE_ENCAPSULATE;
			continue;
		}

		if (strcmp(argv[argn], "-d") == 0)
		{
			dodecode |= DECODE_UNENCRYPT;
			continue;
		}

		if (strcmp(argv[argn], "-c") == 0)
		{
			dodecode |= DECODE_CHECKSIGN;
			continue;
		}

		if (strcmp(argv[argn], "-p") == 0)
		{
			++argn;
			if (argn < argc)
			{
				passphrase_fd=argv[argn];
				continue;
			}
			--argn;
		}

		fprintf(stderr, "Unknown option: %s\n", argv[argn]);
		exit(1);
	}

	if (!dosign && !doencode && !dodecode)
	{
		usage();
		return (1);
	}

	signal(SIGCHLD, SIG_DFL);
	signal(SIGPIPE, SIG_IGN);

	/* Make things sane */

	if (dosign || doencode)
		dodecode=0;

	reformime_pid=0;

	if (dosign && !doencode)
		reformime(argv[0]);

	noexec(stdin);
	noexec(stdout);
	rc=dosignencode(dosign, doencode, dodecode,
			   stdin, stdout, argc-argn,
			argv+argn);


	if (rc == 0 && (fflush(stdout) || ferror(stdout)))
	{
		perror("write");
		rc=1;
	}

	exit(rc);
	return (0);
}
