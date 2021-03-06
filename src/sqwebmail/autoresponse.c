/*
** $Id: autoresponse.c,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/

/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include	"config.h"
#include	"autoresponse.h"
#include	"maildir/autoresponse.h"
#include	"mailfilter.h"
#include	"unicode/unicode.h"
#include	"sqwebmail.h"
#include	"htmllibdir.h"
#include	"maildir.h"
#include	"maildir/maildirmisc.h"
#include	"maildir/maildirfilter.h"
#include	"rfc2045/rfc2045.h"
#include	"rfc2045/rfc2646.h"
#include	"cgi/cgi.h"
#include	"numlib/numlib.h"
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<signal.h>
#include	<ctype.h>
#include	<errno.h>
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#if	HAVE_SYSLOG_H
#include	<syslog.h>
#else
#define syslog(a,b)
#endif

extern const char *sqwebmail_content_charset;
extern void output_attrencoded(const char *);
extern const char *calc_mime_type(const char *filename);

static int save_autoresponse(const char *p, size_t l, void *vp)
{
	FILE *fp=*(FILE **)vp;

	if (fp)
		fwrite(p, l, 1, fp);
	return (0);
}

static int display_autoresponse(struct rfc2646parser *, int, void *);

struct parseinfo {
	int last_was_flowed;
} ;

static int read_headers(FILE *);

void autoresponse()
{
	if ( *cgi("do.newautoresp"))
	{
		const char *name=cgi("newname");
		char *p;
		FILE *fp;

		p=folder_toutf7(name);

		if (!p || maildir_autoresponse_validate(NULL, p))
		{
			free(p);
			printf("%s", getarg("BADNAME"));
			return;
		}

		if ((fp=maildir_autoresponse_open(NULL, p)) != NULL)
		{
			free(p);
			fclose(fp);
			printf("%s", getarg("EEXIST"));
			return;
		}

		printf("<tr bgcolor=\"#FFFFFC\"><td valign=top align=left>%s</td><td>\n", name);
		printf("<input type=hidden name=autoresponse value=\"");
		output_attrencoded(p);
		printf("\">\n");
		free(p);

		printf("<textarea cols=\"50\" rows=\"10\" "
		       "name=\"text\" wrap=soft>");
		printf("</textarea><br>\n");
		/* by lfan
		printf("%s<input type=file size=20 name=uploadfile><br>",
		       getarg("UPLOAD"));
		*/
		printf("<br><input class=mybtn type=submit name=\"do.autorespcreate\""
		       " value=\"%s\"></td>", getarg("SAVE"));
		return;
	}

	if ( *cgi("do.autorespedit"))
	{
		const char *autorespname=cgi("autoresponse_choose");
		FILE *fp;
		struct rfc2646parser *parser;
		char buf[BUFSIZ];
		int i;
		char *s=folder_fromutf7(autorespname);
		struct parseinfo info;
		const char *pp;

		if (!s)
		{
			printf(getarg("ERROR"), strerror(errno));
			return;
		}

		pp=cgi("replytext");

		if ((fp=maildir_autoresponse_open(NULL, autorespname)) == NULL
		    && !*pp)
		{
			free(s);
			return;
		}

		printf("<tr bgcolor=\"#FFFFFC\"><td valign=top align=left>%s</td><td>\n", s);

		if (fp && read_headers(fp))
		{
			fclose(fp);
			free(s);
			return;
		}

		info.last_was_flowed=0;

		if ((parser=rfc2646_alloc( &display_autoresponse, &info))
		    == NULL)
		{
			free(s);
			if (fp)
				fclose(fp);
			printf(getarg("ERROR"), strerror(errno));
			return;
		}

		printf("<input type=hidden name=autoresponse value=\"");
		output_attrencoded(autorespname);
		printf("\">\n");
		free(s);

		printf("<textarea cols=\"50\" rows=\"10\" "
		       "name=\"text\" wrap=soft>");

		if (pp && *pp)
			output_attrencoded(pp);
		else
			while ((i=fread(buf, 1, sizeof(buf), fp)) > 0)
			{
				if (rfc2646_parse(parser, buf, i))
					break;
			}

		rfc2646_free(parser);
		if (fp)
			fclose(fp);
		printf("\n\n</textarea><br>\n");
		/*
		printf("%s<input type=file size=20 name=uploadfile><br>",
		       getarg("UPLOAD"));
		*/
		printf("<br><input class=mybtn type=submit name=\"do.autorespcreate\""
		       " value=\"%s\"></td>", getarg("SAVE"));
		return;
	}
}

/*
** Read the MIME headers in the autoresponse file, to make sure that we
** can show it.
*/

static int read_headers(FILE *fp)
{
	struct rfc2045 *rfc2045p=rfc2045_alloc();
	static const char mv[]="Mime-Version: 1.0\n";
	char buf[BUFSIZ];
	char *s;
	const char *content_type, *content_transfer_encoding, *charset;

#if HAVE_SQWEBMAIL_UNICODE
	const struct unicode_info *u1, *u2;
#endif

	rfc2045_parse(rfc2045p, mv, sizeof(mv)-1);

	while ((s=fgets(buf, sizeof(buf), fp)) != NULL)
	{
		rfc2045_parse(rfc2045p, s, strlen(s));
		if (strcmp(s, "\n") == 0 || strcmp(s, "\r\n") == 0)
			break;
	}
	rfc2045_parse_partial(rfc2045p);

	rfc2045_mimeinfo(rfc2045p, &content_type,
			 &content_transfer_encoding,
			 &charset);

	if (strcmp(content_type, "text/plain") ||
	    !rfc2045_isflowed(rfc2045p))
	{
		printf(getarg("ATT"), content_type);
		rfc2045_free(rfc2045p);
		return (-1);
	}

#if HAVE_SQWEBMAIL_UNICODE
	u1=unicode_find(charset);
	u2=unicode_find(sqwebmail_content_charset);

	if (!u1 || !u2 || strcmp(u1->chset, u2->chset))
#else
       	if (strcmp(charset, sqwebmail_content_charset))
#endif
	{
		printf(getarg("CHSET"), charset,
		       sqwebmail_content_charset);
		rfc2045_free(rfc2045p);
		return (-1);
	}

	rfc2045_free(rfc2045p);
	return (0);
}

static int display_autoresponse(struct rfc2646parser *lineinfo,
				int isflowed, void *vp)
{
	struct parseinfo *info=(struct parseinfo *)vp;

	if (!isflowed && lineinfo->line[0] == 0 &&
	    info->last_was_flowed)
	{
		printf("\n");
		info->last_was_flowed=0;
		return (0);
	}

	if (info->last_was_flowed)
		printf(" ");
	else
		printf("\n");

	output_attrencoded(lineinfo->line);
	info->last_was_flowed=isflowed;
	return (0);
}

static FILE *upload_attachment(const char *);

void autoresponsedelete()
{
	if ( *cgi("do.autorespcreate"))
	{
		const char *autorespname=cgi("autoresponse");
		const char *autoresptxt=cgi("text");
		FILE *fp;
		struct rfc2646create *create_ptr;
		size_t l;

		if ((fp=upload_attachment(autorespname)) == NULL)
		{
			if ((create_ptr=
			     rfc2646create_alloc(&save_autoresponse, &fp))
			    == NULL ||
			    (fp=maildir_autoresponse_create(NULL,
							    autorespname))
			    == NULL)
			{
				if (create_ptr)
					rfc2646create_free(create_ptr);
				printf(getarg("SAVEFAILED"), strerror(errno));
				return;
			}

			l=strlen(autoresptxt);
			while (l && (autoresptxt[l-1] == '\r' ||
				     autoresptxt[l-1] == '\n'))
				--l;

			fprintf(fp, "Content-Type: text/plain; format=flowed; "
				"charset=\"%s\"\n"
				"Content-Transfer-Encoding: 8bit\n\n",
				sqwebmail_content_charset);

			rfc2646create_parse(create_ptr, autoresptxt, l);
			rfc2646create_parse(create_ptr, "\n\n", 2);
			rfc2646create_free(create_ptr);
		}

		if (fflush(fp) || ferror(fp))
		{
			fclose(fp);
			printf(getarg("SAVEFAILED"), strerror(errno));
			return;
		}
		if (maildir_autoresponse_create_finish(NULL, autorespname, fp))
		{
			if (errno == ENOSPC)
			{
				cgi_put("do.autorespedit", "1");
				cgi_put("autoresponse_choose", autorespname);
				cgi_put("replytext", cgi("text"));
				printf(getarg("QUOTA"), strerror(errno));
			}
			else
				printf(getarg("SAVEFAILED"), strerror(errno));
		}
		return;
	}

	if ( *cgi("do.autorespdelete"))
	{
		const char *autorespname=cgi("autoresponse_choose");

		if (mailfilter_autoreplyused(autorespname))
		{
			char *s=folder_fromutf7(autorespname);
			printf(getarg("INUSE"), s ? s:"");
			if (s)
				free(s);
		}
		else
			maildir_autoresponse_delete(NULL, autorespname);
		return;
	}
}

struct upload_attach_info {
	FILE *fp;
	const char *filename;
	const char *name;
	const char *autorespname;
} ;

static int start_upload(const char *, const char *, void *);
static int upload(const char *, size_t, void *);
static void end_upload(void *);

static FILE *upload_attachment(const char *autorespname)
{
	struct upload_attach_info uai;

	uai.fp=NULL;
	uai.autorespname=autorespname;

	if (cgi_getfiles( &start_upload, &upload, &end_upload, 1, &uai ))
	{
		if (uai.fp)
			fclose(uai.fp);

		return (NULL);
	}

	return (uai.fp);
}

static int start_upload(const char *name, const char *filename, void *vp)
{
	struct upload_attach_info *uai=(struct upload_attach_info *)vp;
	const char *p;

	p=strrchr(filename, '/');
	if (p)	filename=p+1;

	p=strrchr(filename, '\\');
	if (p)	filename=p+1;

	if (*filename)
	{
		uai->filename=filename;
	}
	else
	{
		p=strrchr(name, '/');
		if (p)	name=p+1;

		p=strrchr(name, '\\');
		if (p)	name=p+1;
		uai->filename=p;
	}

	uai->fp=tmpfile();
	if (!uai->fp)
		enomem();
	return (0);
}

static int upload(const char *c, size_t n, void *vp)
{
	struct upload_attach_info *uai=(struct upload_attach_info *)vp;

	if (fwrite(c, n, 1, uai->fp) != 1)
	{
		fclose(uai->fp);
		enomem();
	}
	return (0);
}

static int upload_messagerfc822(FILE *, FILE *);

static void end_upload(void *vp)
{
	struct upload_attach_info *uai=(struct upload_attach_info *)vp;
	const char *mimetype;
	char *argvec[10];
	int n;
	pid_t pid1, pid2;
	int waitstat;
	FILE *afp;

	if (fflush(uai->fp) || ferror(uai->fp)
	    || fseek(uai->fp, 0L, SEEK_SET) < 0)
	{
		fclose(uai->fp);
		enomem();
	}

	mimetype=calc_mime_type(uai->filename);

	if (strcasecmp(mimetype, "message/rfc822") == 0)
	{
		/* Magic */

		afp=maildir_autoresponse_create(NULL, uai->autorespname);
		if (!afp)
		{
			fclose(uai->fp);
			enomem();
		}

		if (upload_messagerfc822(uai->fp, afp) ||
		    fflush(afp) || ferror(afp))
		{
			fclose(uai->fp);
			fclose(afp);
			enomem();
		}
		fclose(uai->fp);
		uai->fp=afp;
		return;
	}
	argvec[0]="makemime";
	argvec[1]="-c";
	argvec[2]=(char *)mimetype;

	n=3;
	if (strncasecmp(argvec[2], "text/", 5) == 0 ||
	    strcasecmp(argvec[2], "auto") == 0)
	{
		argvec[3]="-C";
		argvec[4]=(char *)sqwebmail_content_charset;
		n=5;
	}

	argvec[n++]="-";
	argvec[n]=0;

	afp=maildir_autoresponse_create(NULL, uai->autorespname);
	if (!afp)
	{
		fclose(uai->fp);
		enomem();
	}

	signal(SIGCHLD, SIG_DFL);
	pid1=fork();

	if (pid1 < 0)
	{
		fclose(afp);
		fclose(uai->fp);
		enomem();
	}

	if (pid1 == 0)
	{
		close(0);
		dup(fileno(uai->fp));
		close(1);
		dup(fileno(afp));
		fclose(uai->fp);
		fclose(afp);
		execv(MAKEMIME, argvec);
		syslog(LOG_DAEMON | LOG_CRIT,
		       "sqwebmail: %s: %s", MAKEMIME, strerror(errno));
		exit(1);
	}

	for (;;)
	{
		pid2=wait(&waitstat);

		if (pid2 == pid1)
		{
			waitstat= WIFEXITED(waitstat) ? WEXITSTATUS(waitstat)
				: 1;
			break;
		}

		if (pid2 == -1)
		{
			waitstat=1;
			break;
		}
	}

	if (waitstat)
	{
		fclose(afp);
		fclose(uai->fp);
		enomem();
	}

	fclose(uai->fp);
	uai->fp=afp;
}

/*
** If we get something that's MIMEed as message/rfc822, read it, strip its
** headers except for the MIME content- headers, then save what's left as
** our autoreply.  This allows for a convenient way to upload
** multipart/alternative content.
*/

static int upload_messagerfc822(FILE *i, FILE *o)
{
	char buf[BUFSIZ];
	int skip_hdr;
	int c;
	const char *pp;

	skip_hdr=0;

	for (;;)
	{
		if (fgets(buf, sizeof(buf), i) == NULL)
		{
			fprintf(o, "\n");
			return (0);
		}

		if (strcmp(buf, "\n") == 0 || strcmp(buf, "\r\n") == 0)
		{
			fprintf(o, "\n");
			break;
		}

		if (!isspace((int)(unsigned char)*buf))
			skip_hdr=strncasecmp(buf, "content-", 8) != 0;

		if (skip_hdr)
			continue;

		for (pp=buf; *pp; pp++)
			if (*pp != '\r')
				if (putc((int)(unsigned char)*pp, o)
				    == EOF)
					return (-1);
	}

	while ((c=getc(i)) != EOF)
		if (c != '\r')
			if (putc(c, o) == EOF)
				return (-1);
	return (0);
}



static int comp_autorespname(const void *a, const void *b)
{
	const char *ca=*(const char **)a;
	const char *cb=*(const char **)b;

	char *sa=folder_fromutf7(ca);
	char *sb=folder_fromutf7(cb);

	int i=sa && sb ? strcoll(sa, sb):0;

	free(sa);
	free(sb);
	return (i);
}

void autoresponselist()
{
	char **list=maildir_autoresponse_list(NULL); /* I'm sorry... */
	size_t i;

	if (!list)
	{
		printf(getarg("ERROR"), strerror(errno));
		return;
	}

	for (i=0; list[i]; i++)
		;

	qsort(list, i, sizeof(list[0]), &comp_autorespname);

	for (i=0; list[i]; i++)
	{
		char *s;

		printf("<option value=\"");
		output_attrencoded(list[i]);
		printf("\">");

		s=folder_fromutf7(list[i]);
		output_attrencoded(s);
		free(s);
	}

	maildir_autoresponse_list_free(list);
}

void autoresponsepick()
{
	char **list=maildir_autoresponse_list(NULL);
	size_t i;
	const char *choice=cgi("autoresponse_choose");

	if (!list)
	{
		printf(getarg("ERROR"), strerror(errno));
		return;
	}

	for (i=0; list[i]; i++)
		;

	qsort(list, i, sizeof(list[0]), &comp_autorespname);

	for (i=0; list[i]; i++)
	{
		char *s;

		printf("<option%s value=\"",
		       strcmp(choice, list[i]) ? "":" selected");
		output_attrencoded(list[i]);
		printf("\">");

		s=folder_fromutf7(list[i]);
		output_attrencoded(s);
		free(s);
	}

	maildir_autoresponse_list_free(list);
}
