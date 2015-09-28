/*
** Copyright 1998 - 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: newmsg_create.c,v 1.4 2003/06/13 04:54:39 lfan Exp $
*/
#include	"config.h"
#include	"cgi/cgi.h"
#include	"sqconfig.h"
#include	"sqwebmail.h"
#include	"auth.h"
#include	"maildir.h"
#include	"newmsg.h"
#include	"folder.h"
#include	"filter.h"
#include	"pref.h"
#include	"gpg.h"
#include	"addressbook.h"
#include	"maildir/maildirmisc.h"
#include	"rfc822/rfc822.h"
#include	"rfc2045/rfc2045.h"
#include	"rfc2045/rfc2646.h"
#include	"rfc822/rfc2047.h"
#include	"gpglib/gpglib.h"
#include	"http11/http11.h"
#include	"htmllibdir.h"

#include	<stdlib.h>
#if HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<ctype.h>
#include	<fcntl.h>

extern const char *rfc822_mkdt(time_t);

extern const char *sqwebmail_content_charset;
extern const char *sqwebmail_content_language;

int newdraftfd;
extern const char *sqwebmail_mailboxid;

const char mimemsg[]="This is a MIME-formatted message.  If you see this text it means that your\nmail software cannot handle MIME-formatted messages.\n\n";

char *newmsg_createdraft_do(const char *, const char *, int);

/* Save message in a draft file */

char *newmsg_createdraft(const char *curdraft)
{
	if (curdraft && *curdraft)
	{
	char	*base=maildir_basename(curdraft);
	char	*filename=maildir_find(DRAFTS, base);

		if (filename)
		{
		char	*p=newmsg_createdraft_do(filename, cgi("message"), 0);

			free(filename);
			free(base);
			return (p);
		}
		free(base);
	}
	return (newmsg_createdraft_do(0, cgi("message"), 0));
}

static void create_draftheader_do(const char *hdrname, const char *p,
	int isrfc822addr);

static void create_draftheader(const char *hdrname, const char *p,
	const char *q, int isrfc822addr)
{
	if (q && *q)	/* Add from address book */
	{
	char	*nick=cgi_multiple("nick", ",");
	char	*s;

		if (nick)
		{
			s=malloc(strlen(p)+strlen(nick)+2);

			if (s)
			{
				strcpy(s, p);
				if (*s && *nick)	strcat(s, ",");
				strcat(s, nick);
				create_draftheader_do(hdrname, s, isrfc822addr);
				free(s);
				free(nick);
				return;
			}
			free(nick);
		}

	}
	create_draftheader_do(hdrname, p, isrfc822addr);
}

static void create_draftheader_do(const char *hdrname, const char *p,
	int isrfc822addr)
{
char	*s;

	if (!*p)	return;

	if (!isrfc822addr)
	{
		s=rfc2047_encode_str(p, sqwebmail_content_charset);
	}
	else
	{
	struct rfc822t *t;
	struct rfc822a *a;
	char *pp;
		// fix the recursively encoding problem
		pp = rfc2047_decode_simple(p);

		/*
		** For proper RFC 2047 encoding, we must RFC822-parse
		** this header.
		*/

		s=0;
		if ((t=rfc822t_alloc_new(pp, NULL, NULL)) != 0)
		{
			if ((a=rfc822a_alloc(t)) != 0)
			{
				s=rfc2047_encode_header(a,
					sqwebmail_content_charset);
				rfc822a_free(a);
			}
			rfc822t_free(t);
		}
		free(pp);
	}

	if (!s)
	{
		close(newdraftfd);
		enomem();
	}
	maildir_writemsgstr(newdraftfd, hdrname);
	maildir_writemsgstr(newdraftfd, s);
	maildir_writemsgstr(newdraftfd, "\n");
	free(s);
}

void newmsg_create_multipart(int newdraftfd, const char *charset,
			const char *multipart_boundary)
{
	maildir_writemsgstr(newdraftfd,
		"Mime-version: 1.0\n"
		"Content-Type: multipart/mixed; boundary=\"");
	maildir_writemsgstr(newdraftfd, multipart_boundary);
	maildir_writemsgstr(newdraftfd, "\"; charset=\"");
	maildir_writemsgstr(newdraftfd, charset);
	maildir_writemsgstr(newdraftfd, 
					"\"\n\n");

	maildir_writemsgstr(newdraftfd, mimemsg);
}


static char	*newmsg_multipart_boundary(FILE *, const char *);
static void newmsg_copy_attachments(FILE *, const char *);

void newmsg_copy_nonmime_headers(FILE *fp)
{
char	*header, *value;
char	*q;

	while ((header=maildir_readheader(fp, &value, 1)) != NULL)
	{
		if (strcmp(header, "mime-version") == 0 ||
			strncmp(header, "content-", 8) == 0)	continue;

		/* Fluff - capitalize header names */

		for (q=header; *q; q++)
		{
			for (*q=toupper(*q); *q; q++)
				if (*q == '-')	break;
			if (!*q)
				break;
		}

		maildir_writemsgstr(newdraftfd, header);
		maildir_writemsgstr(newdraftfd, ": ");
		maildir_writemsgstr(newdraftfd, value);
		maildir_writemsgstr(newdraftfd, "\n");
	}
}

static void do_save(const char *, size_t);

char *newmsg_createdraft_do(const char *curdraft, const char *newmsg,
	int keepheader)
{
char	*draftfilename;
FILE	*fp=0;
char	*multipart_boundary;
const char *content_type;
const char *content_transfer_encoding;
const char *charset;
unsigned long prev_size=0;
off_t	transferencodingpos;
int is_newevent=strcmp(cgi("form"), "newevent") == 0;
/* We're on the 'new event' screen */

	if (curdraft)	/* Reuse a draft filename */
		newdraftfd=maildir_recreatemsg(DRAFTS, curdraft, &draftfilename);
	else
		newdraftfd=maildir_createmsg(DRAFTS, 0, &draftfilename);
	if (newdraftfd < 0)	enomem();

	fp=NULL;
	if (curdraft)
	{
	int	x=maildir_safeopen(curdraft, O_RDONLY, 0);

		if (x >= 0)
			if ((fp=fdopen(x, "r")) == 0)
				close(x);
	}

	if (fp)
	{
	char	*header, *value;
	struct	stat	stat_buf;

		if (fstat(fileno(fp), &stat_buf))
		{
			fclose(fp);
			enomem();
		}
		prev_size=stat_buf.st_size;

		while ((header=maildir_readheader(fp, &value, 1)) != NULL)
		{
			if (keepheader == NEWMSG_SQISPELL)
			{
				if (strcasecmp(header, "mime-version") == 0 ||
				    strncasecmp(header, "content-", 8) == 0)
					continue;
			}
			else if (keepheader == NEWMSG_PCP)
			{
				if (strcasecmp(header, "mime-version") == 0 ||
				    strncasecmp(header, "content-", 8) == 0 ||
				    strcasecmp(header, "date") == 0 ||
				    strcasecmp(header, "from") == 0 ||
				    strcasecmp(header, "subject") == 0)
					continue;
			}
			else
			{
				if (strcmp(header, "in-reply-to") &&
					strcmp(header, "references") &&
					strncmp(header, "x-", 2))	continue;
				/* Do not discard these headers */
			}

			maildir_writemsgstr(newdraftfd, header);
			maildir_writemsgstr(newdraftfd, ": ");
			maildir_writemsgstr(newdraftfd, value);
			maildir_writemsgstr(newdraftfd, "\n");
		}
	}
	else if (is_newevent)
		maildir_writemsgstr(newdraftfd, "X-Event: 1\n");

	if (!keepheader
	    || keepheader == NEWMSG_PCP)
	/* Coming back from msg edit, set headers */
	{
	const	char *p=cgi("headerfrom");

		if (!*p)	p=pref_from;
		if (!p || !*p || access(NOCHANGINGFROM, 0) == 0)
			p=login_fromhdr();

		create_draftheader("From: ", p, 0, 1);

		if (!pref_from || strcmp(p, pref_from))
			pref_setfrom(p);

/* sam ????
	create_draftheader("In-Reply-To: ", cgi("headerin-reply-to"));
*/
		if (!is_newevent)
		{
			create_draftheader("To: ", cgi("headerto"),
					   cgi("addressbook_to"), 1);
			create_draftheader("Cc: ", cgi("headercc"),
					   cgi("addressbook_cc"), 1);
			create_draftheader("Bcc: ", cgi("headerbcc"),
					   cgi("addressbook_bcc"), 1);
			create_draftheader("Reply-To: ", cgi("headerreply-to"), 0, 1);
		}
	}

	if (!keepheader || keepheader == NEWMSG_PCP)
	{
	time_t	t;

		create_draftheader("Subject: ", cgi("headersubject"), 0, 0);

		time(&t);
		create_draftheader("Date: ", rfc822_mkdate(t), 0, 0);
	}

	/* If the message has attachments, calculate multipart boundary */

	multipart_boundary=fp ? newmsg_multipart_boundary(fp, newmsg) : NULL;

	if (multipart_boundary)
	{
	struct	rfc2045	*rfcp=rfc2045_fromfp(fp), *q;
				/* Copy over existing charset */

		if (!rfcp)
		{
			close(newdraftfd);
			fclose(fp);
			enomem();
		}

		rfc2045_mimeinfo(rfcp, &content_type,
			&content_transfer_encoding, &charset);

		newmsg_create_multipart(newdraftfd,
			sqwebmail_content_charset, multipart_boundary);

		maildir_writemsgstr(newdraftfd, "--");
		maildir_writemsgstr(newdraftfd, multipart_boundary);

		for (q=rfcp->firstpart; q; q=q->next)
		{
			if (q->isdummy)	continue;
			rfc2045_mimeinfo(q, &content_type,
				&content_transfer_encoding, &charset);
			if (strcmp(content_type, "text/plain") == 0) break;
		}

		maildir_writemsgstr(newdraftfd,
				    "\nContent-Type: text/plain; format=xdraft");

		if (q)
		{
			maildir_writemsgstr(newdraftfd, "; charset=\"");
			maildir_writemsgstr(newdraftfd,
				sqwebmail_content_charset);
			maildir_writemsgstr(newdraftfd, "\"");
		}
		rfc2045_free(rfcp);
		maildir_writemsgstr(newdraftfd, "\n");
	}
	else
	{
	struct	rfc2045 *rfcp;

		if (fp)
		{
			rfcp=rfc2045_fromfp(fp);
			if (!rfcp)
			{
				close(newdraftfd);
				fclose(fp);
				enomem();
			}

			rfc2045_mimeinfo(rfcp, &content_type,
					&content_transfer_encoding, &charset);
		}
		else
		{
			rfcp=0;
			charset=sqwebmail_content_charset;
		}

		maildir_writemsgstr(newdraftfd, "Mime-Version: 1.0\n"
				    "Content-Type: text/plain; format=xdraft;"
				    " charset=\"");
		maildir_writemsgstr(newdraftfd, charset);
		maildir_writemsgstr(newdraftfd, "\"\n");

		if (rfcp)
			rfc2045_free(rfcp);
	}

	maildir_writemsgstr(newdraftfd, "Content-Transfer-Encoding: ");
	transferencodingpos=writebufpos;
	maildir_writemsgstr(newdraftfd, "7bit\n\n");

	/*	maildir_writemsgstr(newdraftfd, "\n"); */


	{
	char	*buf=strdup(newmsg);
	size_t	i,j;

		for (i=j=0; buf[i]; i++)
			if (buf[i] != '\r')	buf[j++]=buf[i];

		/* Trim excessive trailing empty lines */

		while (j > 4 && strncmp(buf+j-3, "\n\n\n", 3) == 0)
			--j;
		filter_start(FILTER_FOR_SAVING, &do_save);
		filter(buf, j);
		filter("\n", 1);
		filter_end();
		free(buf);
	}

	if ( multipart_boundary)
	{
		newmsg_copy_attachments(fp, multipart_boundary);
		maildir_writemsgstr(newdraftfd, "\n--");
		maildir_writemsgstr(newdraftfd, multipart_boundary);
		maildir_writemsgstr(newdraftfd, "--\n");
		free(multipart_boundary);
	}
	if (fp)	fclose(fp);

	if ( maildir_writemsg_flush(newdraftfd) == 0 && writebuf8bit)
	{
		if (lseek(newdraftfd, transferencodingpos, SEEK_SET) < 0 ||
			write(newdraftfd, "8", 1) != 1)
		{
			close(newdraftfd);
			enomem();
		}
	}

	if ( maildir_closemsg(newdraftfd, DRAFTS, draftfilename, -1, prev_size))
		cgi_put("error", "quota");

	return(draftfilename);
}

static void do_save(const char *p, size_t l)
{
	maildir_writemsg(newdraftfd, p, l);
}

/* Create message in the sent folder */

static void sentmsg_copy(FILE *, struct rfc2045 *);
static void sentmsg_reformat(FILE *, struct rfc2045 *, int);

extern void header_uc(char *h);

struct lookup_buffers {
	struct lookup_buffers *next;
	char *buf;
	char *buf2;
	} ;

static int lookup_addressbook_do(const char *header, const char *value,
	struct lookup_buffers **lookup_buffer_list)
{
struct	rfc822t *t;
struct	rfc822a *a;
int	i;
char	*newbuf;
struct lookup_buffers *ptr;
int	expanded=0;

	t=rfc822t_alloc_new(value, NULL, NULL);
	if (!t)	enomem();
	a=rfc822a_alloc(t);
	if (!a)
	{
		rfc822t_free(t);
		enomem();
	}

	for (i=0; i<a->naddrs; i++)
	{
	char	*p;
	const	char *q;
	struct lookup_buffers *r;

		if (a->addrs[i].tokens == 0)
			continue;
		if (a->addrs[i].name)
			continue;	/* Can't be a nickname */

		p=rfc822_getaddr(a, i);
		if (!p)
		{
			rfc822a_free(a);
			rfc822t_free(t);
			free(p);
			return (-1);
		}

		for (ptr= *lookup_buffer_list; ptr; ptr=ptr->next)
			if (strcmp(ptr->buf2, p) == 0)
				break;

		if (ptr)	/* Address book loop */
		{
		int	j;

			for (j=i+1; j<a->naddrs; j++)
				a->addrs[j-1]=a->addrs[j];
			--a->naddrs;
			--i;
			free(p);
			continue;
		}

		if ((q=ab_find(p)) == 0)
		{
			free(p);
			continue;
		}

		r=malloc(sizeof(struct lookup_buffers));
		if (r)	r->buf=r->buf2=0;

		if (!r || !(r->buf=strdup(q)) || !(r->buf2=strdup(p)))
		{
			free(p);
			if (r && r->buf)	free(r->buf);
			if (r)	free(r);
			rfc822a_free(a);
			rfc822t_free(t);
			return (-1);
		}
		free(p);
		r->next= *lookup_buffer_list;
		*lookup_buffer_list=r;
		a->addrs[i].tokens->next=0;
		a->addrs[i].tokens->token=0;
		a->addrs[i].tokens->ptr=r->buf;
		a->addrs[i].tokens->len=strlen(r->buf);
		expanded=1;
	}

	newbuf=rfc822_getaddrs_wrap(a, 70);
	rfc822a_free(a);
	rfc822t_free(t);
	if (!newbuf)	return (-1);

	if (expanded)	/* Look through the address book again */
	{
	int	rc=lookup_addressbook_do(header, newbuf, lookup_buffer_list);

		free(newbuf);
		return (rc);
	}

	create_draftheader_do(header, newbuf, 1);
	free(newbuf);
	return (0);
}

static void lookup_addressbook(const char *header, const char *value)
{
struct lookup_buffers *lookup_buffer_list=0;
int	rc;
char	*s=strdup(value);

	if (!s)	enomem();
	rc=lookup_addressbook_do(header, s, &lookup_buffer_list);
	free(s);

	while (lookup_buffer_list)
	{
	struct lookup_buffers *p=lookup_buffer_list;

		lookup_buffer_list=p->next;
		free(p->buf);
		free(p->buf2);
		free(p);
	}
	if (rc)	enomem();
}

char *newmsg_createsentmsg(const char *draftname, int *isgpgerr, int footer)
{
char	*filename=maildir_find(DRAFTS, draftname);
FILE	*fp;
char	*sentname;
char	*header, *value;
struct	rfc2045 *rfcp;
int	x;

#if 0
off_t	transferencodingpos;
#endif

	*isgpgerr=0;
 
	if (!filename)	return (0);

	fp=0;

	x=maildir_safeopen(filename, O_RDONLY, 0);
	if (x >= 0)
		if ((fp=fdopen(x, "r")) == 0)
			close(x);

	if (fp == 0)
	{
		free(filename);
		enomem();
	}

	rfcp=rfc2045_fromfp(fp);
	if (!rfcp || fseek(fp, 0L, SEEK_SET) < 0)
	{
		fclose(fp);
		close(newdraftfd);
		enomem();
	}

	newdraftfd=maildir_createmsg(SENT, 0, &sentname);
	if (newdraftfd < 0)
	{
		rfc2045_free(rfcp);
		free(filename);
		fclose(fp);
		enomem();
	}
	/* First, copy all headers except X- headers */

	while ((header=maildir_readheader(fp, &value, 1)) != 0)
	{
		if (strncmp(header, "x-", 2) == 0)	continue;
		header_uc(header);
		if (strcasecmp(header, "To") == 0)
		{
			lookup_addressbook("To: ", value);
			continue;
		}

		if (strcasecmp(header, "Cc") == 0)
		{
			lookup_addressbook("Cc: ", value);
			continue;
		}

		if (strcasecmp(header, "Bcc") == 0)
		{
			lookup_addressbook("Bcc: ", value);
			continue;
		}

		if (strcasecmp(header, "Content-Type") == 0 &&
		    !rfcp->firstpart)	/* Need to punt this header */
		{
			maildir_writemsgstr(newdraftfd,
					    "Content-Type: text/plain;"
					    " format=flowed; charset=\"");
			maildir_writemsgstr(newdraftfd,
					    sqwebmail_content_charset);
			maildir_writemsgstr(newdraftfd, "\"\n");
			continue;
		}

		maildir_writemsgstr(newdraftfd, header);
		maildir_writemsgstr(newdraftfd, ": ");
		maildir_writemsgstr(newdraftfd, value);
		maildir_writemsgstr(newdraftfd, "\n");
	}
	if (access(USEXSENDER, 0) == 0)
	{
		maildir_writemsgstr(newdraftfd, "X-Sender: ");
		maildir_writemsgstr(newdraftfd, login_returnaddr());
		maildir_writemsgstr(newdraftfd, "\n");
	}

#if 0
	maildir_writemsgstr(newdraftfd, "Content-Transfer-Encoding: ");
	transferencodingpos=writebufpos;
	maildir_writemsgstr(newdraftfd, "7bit\n");

#endif
	maildir_writemsgstr(newdraftfd, "\n");

	if (!rfcp->firstpart)
		sentmsg_reformat(fp, rfcp, footer);
	else
	{
	int	found_textplain=0;
	struct rfc2045 *p;

		for (p=rfcp->firstpart; p; p=p->next)
		{
		const char *content_type;
		const char *content_transfer_encoding;
		const char *charset;

			rfc2045_mimeinfo(p, &content_type,
				&content_transfer_encoding, &charset);

			if (strcmp(content_type, "text/plain") == 0 &&
				!p->isdummy && !found_textplain)
			{
				maildir_writemsgstr(newdraftfd,
						    "Content-Type: text/plain;"
						    " format=flowed;"
						    " charset=");
				maildir_writemsgstr(newdraftfd, charset);
				maildir_writemsgstr(newdraftfd,
					"\nContent-Transfer-Encoding: ");
				maildir_writemsgstr(newdraftfd,
					content_transfer_encoding);
				maildir_writemsgstr(newdraftfd, "\n\n");
				sentmsg_reformat(fp, p, footer);
				found_textplain=1;
			}
			else	sentmsg_copy(fp, p);
			maildir_writemsgstr(newdraftfd, "\n--");
			maildir_writemsgstr(newdraftfd, rfc2045_boundary(rfcp));
			if (!p->next)
				maildir_writemsgstr(newdraftfd, "--");
			maildir_writemsgstr(newdraftfd, "\n");
		}
	}
	if ( maildir_writemsg_flush(newdraftfd))
	{
		free(sentname);
		return (0);
	}

#if 0
	if (writebuf8bit)
	{
		if (lseek(newdraftfd, transferencodingpos, SEEK_SET) < 0 ||
			write(newdraftfd, "8", 1) != 1)
		{
			free(sentname);
			return (0);
		}
	}
#endif

	if ( maildir_writemsg_flush(newdraftfd))
	{
		maildir_closemsg(newdraftfd, SENT, sentname, 0, 0);
		free(sentname);
		return (0);
	}

#if HAVE_SQWEBMAIL_UNICODE
	if (has_gpg(GPGDIR) == 0)
	{
		char dosign= *cgi("sign");
		char doencrypt= *cgi("encrypt");
		const char *signkey= cgi("signkey");
		char *encryptkeys=cgi_multiple("encryptkey", " ");

		if (!encryptkeys)
			enomem();

		if (gpgbadarg(encryptkeys) || !*encryptkeys)
		{
			free(encryptkeys);
			encryptkeys=0;
		}

		if (gpgbadarg(signkey) || !*signkey)
		{
			signkey=0;
		}

		if (!encryptkeys)
			doencrypt=0;

		if (!signkey)
			dosign=0;

		if (lseek(newdraftfd, 0L, SEEK_SET) < 0)
		{
			maildir_closemsg(newdraftfd, SENT,
					 sentname, 0, 0);
			free(sentname);
			return (0);
		}

		if (!dosign)
			signkey=0;
		if (!doencrypt)
			encryptkeys=0;

		if (dosign || doencrypt)
		{
			/*
			** What we do is create another draft, then substitute
			** it for newdraftfd/sentname.  Sneaky.
			*/

			char *newnewsentname;
			int newnewdraftfd=maildir_createmsg(SENT, 0,
							    &newnewsentname);

			if (newnewdraftfd < 0)
			{
				maildir_closemsg(newdraftfd, SENT,
						 sentname, 0, 0);
				free(sentname);
				free(encryptkeys);
				return (0);
			}

			if (gpgdomsg(newdraftfd, newnewdraftfd,
				     signkey, encryptkeys))
			{
				maildir_closemsg(newnewdraftfd, SENT,
						 newnewsentname, 0, 0);
				free(newnewsentname);
				maildir_closemsg(newdraftfd, SENT,
						 sentname, 0, 0);
				free(sentname);
				free(encryptkeys);
				*isgpgerr=1;
				return (0);
			}

			maildir_closemsg(newdraftfd, SENT, sentname, 0, 0);
			free(sentname);
			sentname=newnewsentname;
			newdraftfd=newnewdraftfd;

		}
		free(encryptkeys);
	}
#endif

	if ( maildir_closemsg(newdraftfd, SENT, sentname, 1, 0))
	{
		free(sentname);
		return (0);
	}
	return (sentname);
}

/* Note - sentmsg_copy is also reused in newmsg_copy_attachments */

static void sentmsg_copy(FILE *f, struct rfc2045 *p)
{
off_t start_pos, end_pos, start_body;
char buf[512];
int n;
off_t	dummy;

	rfc2045_mimepos(p, &start_pos, &end_pos, &start_body, &dummy, &dummy);
	if (fseek(f, start_pos, SEEK_SET) == -1)
	{
		fclose(f);
		close(newdraftfd);
		enomem();
	}

	while (start_pos < end_pos)
	{
	int	cnt=sizeof(buf);

		if (cnt > end_pos - start_pos)
			cnt=end_pos - start_pos;

		if ((n=fread(buf, 1, cnt, f)) <= 0)
		{
			fclose(f);	
			close(newdraftfd);
			enomem();
		}

		maildir_writemsg(newdraftfd, buf, n);
		start_pos += n;
	}
}

static int write_to_draft(const char *p, size_t l, void *dummy)
{
	maildir_writemsg(newdraftfd, p, l);
	return (0);
}

static void sentmsg_reformat(FILE *f, struct rfc2045 *p, int do_footer)
{
	off_t start_pos, end_pos, start_body;
	char buf[BUFSIZ];
	int n;
	off_t	dummy;
	FILE	*fp;
	struct rfc2646create *createptr;

	rfc2045_mimepos(p, &start_pos, &end_pos, &start_body,
		&dummy, &dummy);
	if (fseek(f, start_body, SEEK_SET) == -1)
	{
		fclose(f);
		close(newdraftfd);
		enomem();
	}

	createptr=rfc2646create_alloc( &write_to_draft, NULL);
	if (!createptr)
	{
		fclose(f);
		close(newdraftfd);
		enomem();
	}

	while (start_body < end_pos)
	{
	int	cnt=sizeof(buf);

		if (cnt > end_pos - start_body)
			cnt=end_pos - start_body;

		if ((n=fread(buf, 1, cnt, f)) <= 0
		    || rfc2646create_parse(createptr, buf, n))
		{
			rfc2646create_free(createptr);
			fclose(f);
			close(newdraftfd);
			enomem();
		}
		start_body += n;
	}

	if (do_footer)
	{
		char	*templatedir=getenv("SQWEBMAIL_TEMPLATEDIR");
	
		if (!templatedir || !*templatedir)
			templatedir=HTMLLIBDIR;

		fp=http11_open_langfile(templatedir,
					sqwebmail_content_language,
					"footer");
		if (fp != 0)
		{
			while ((n=fread(buf, 1, sizeof(buf), fp)) > 0)
				rfc2646create_parse(createptr, buf, n);
			fclose(fp);
		}
	}

	if (rfc2646create_free(createptr))
	{
		fclose(f);
		close(newdraftfd);
		enomem();
	}

}

/* ---------------------------------------------------------------------- */

/* Create a potential multipart boundary separator tag */

char *multipart_boundary_create()
{
char	pidbuf[MAXLONGSIZE];
char	timebuf[MAXLONGSIZE];
time_t	t;
char	cntbuf[MAXLONGSIZE];
unsigned long cnt=0;
char	*p;

	sprintf(pidbuf, "%lu", (unsigned long)getpid());
	time(&t);
	sprintf(timebuf, "%lu", (unsigned long)t);
	sprintf(cntbuf, "%lu", cnt++);
	p=malloc(strlen(pidbuf)+strlen(timebuf) +strlen(cntbuf)+10);
	sprintf(p, "=_%s_%s_%s", cntbuf, pidbuf, timebuf);
	return (p);
}

/* Search for the boundary tag in a string buffer - this is the new message
** we're creating.  We should really look for the tag at the beginning of the
** line, however, the text is not yet linewrapped, besides, why make your
** life hard?
*/

int multipart_boundary_checks(const char *boundary, const char *msg)
{
size_t	boundarylen=strlen(boundary);

	while (*msg)
	{
		if (msg[0] == '-' && msg[1] == '-' && msg[2] != '-' &&
			strncasecmp(msg+2, boundary, boundarylen) == 0)
				return (-1);
		++msg;
	}
	return (0);
}

/* Again, just look for it at the beginning of the line -- why make your
** life hard? */

int multipart_boundary_checkf(const char *boundary, FILE *f)
{
size_t	boundarylen=strlen(boundary);
const char *line;

	if (fseek(f, 0L, SEEK_SET) == -1)
	{
		fclose(f);
		close(newdraftfd);
		enomem();
	}

	while ((line=maildir_readline(f)) != 0)
		if (line[0] == '-' && line[1] == '-' &&
			strncasecmp(line+2, boundary, boundarylen) == 0)
				return (-1);
	return (0);
}

/* ---------------------------------------------------------------------- */

/* Copy existing attachments into the new draft message */

/* multipart_boundary - determine if current draft has attachments */

static struct rfc2045 *copyrfc2045;

static char	*newmsg_multipart_boundary(FILE *f, const char *msg)
{
char	*p=0;

	copyrfc2045=rfc2045_fromfp(f);
	if (!copyrfc2045)
	{
		fclose(f);
		close(newdraftfd);
		enomem();
	}
	if ( copyrfc2045->firstpart == 0)
	{
		rfc2045_free(copyrfc2045);
		return (0);
	}

	do
	{
		if (p)	free(p);
		p=multipart_boundary_create();
	} while (multipart_boundary_checks(p, msg)
		|| multipart_boundary_checkf(p, f));
	return (p);
}

static void	newmsg_copy_attachments(FILE *f, const char *boundary)
{
struct	rfc2045 *p;
int	foundtextplain=0;
const char *content_type;
const char *content_transfer_encoding;
const char *charset;

	for (p=copyrfc2045->firstpart; p; p=p->next)
	{
		if (p->isdummy)	continue;
		rfc2045_mimeinfo(p, &content_type,
			&content_transfer_encoding, &charset);
		if (!foundtextplain && strcmp(content_type,
				"text/plain") == 0)
		{	/* Previous version of this message */

			foundtextplain=1;
			continue;
		}
		maildir_writemsgstr(newdraftfd, "\n--");
		maildir_writemsgstr(newdraftfd, boundary);
		maildir_writemsgstr(newdraftfd, "\n");
		sentmsg_copy(f, p);	/* Reuse some code */
	}
	rfc2045_free(copyrfc2045);
}
