/*
** Copyright 1998 - 2000 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: newmsg_newdraft.c,v 1.1.1.1 2003/05/07 02:15:30 lfan Exp $
*/
#include	"config.h"
#include	"cgi/cgi.h"
#include	"sqconfig.h"
#include	"sqwebmail.h"
#include	"auth.h"
#include	"pref.h"
#include	"maildir.h"
#include	"folder.h"
#include	"mailinglist.h"
#include	"maildir/maildirmisc.h"
#include	"rfc822/rfc822.h"
#include	"rfc822/rfc2047.h"
#include	"rfc2045/rfc2045.h"
#include	"rfc2045/rfc2045charset.h"

#include	<stdlib.h>
#include	<fcntl.h>
#if HAVE_UNISTD_H
#include	<unistd.h>
#endif

extern const char *sqwebmail_mailboxid;
extern const char *sqwebmail_content_charset;

extern char *get_msgfilename(const char *, size_t *);

static int draftfd;

static int ismyaddr(const char *p, void *dummy)
{
	return (strcmp(p, login_returnaddr()) == 0);
}

static void writefunc(const char *p, size_t l, void *dummy)
{
	maildir_writemsg(draftfd, p, l);
}

static void newmsg_writesig(void *dummy)
{
FILE	*fp=fopen(SIGNATURE, "r");
char	buf[256];
int	n;

	if (!fp)	return;
	maildir_writemsg(draftfd, "\n\n", 2);
	while ((n=fread(buf, 1, sizeof(buf), fp)) > 0)
		maildir_writemsg(draftfd, buf, n);
	fclose(fp);
	maildir_writemsg(draftfd, "\n", 1);
}

char *newmsg_newdraft(const char *folder, const char *pos,
			const char *forwardsep, const char *replysalut)
{
char	*filename=0;
char	*replymode;
size_t	pos_n;
FILE	*fp;

const	char *mimeidptr;
char	*draftfilename;
struct	rfc2045 *rfc2045p, *rfc2045partp;
int	x;

	if (*cgi(replymode="reply") ||
		*cgi(replymode="replyall") ||
		*cgi(replymode="replylist") ||
		*cgi(replymode="forward") ||
		*cgi(replymode="forwardatt"))
	{
		pos_n=atol(pos);

		filename=get_msgfilename(folder, &pos_n);
	}

	if (!filename)	return (0);

	fp=0;
	x=maildir_semisafeopen(filename, O_RDONLY, 0);

	if (x >= 0)
		if ((fp=fdopen(x, "r")) == 0)
			close(x);

	if (fp == 0)
	{
		free(filename);
		return (0);
	}

	rfc2045p=rfc2045_fromfp(fp);

	if (!rfc2045p)
	{
		fclose(fp);
		enomem();
	}

	mimeidptr=cgi("mimeid");

	rfc2045partp=0;

	if (*mimeidptr)
	{
		rfc2045partp=rfc2045_find(rfc2045p, mimeidptr);
		if (rfc2045partp)
		{
		const char      *content_type, *dummy;

			rfc2045_mimeinfo(rfc2045partp, &content_type,
				&dummy, &dummy);

			if (!content_type || strcmp(content_type, "message/rfc822"))
				rfc2045partp=0;
			else
				rfc2045partp=rfc2045partp->firstpart;
		}
	}

	if (!rfc2045partp)
		rfc2045partp=rfc2045p;


	draftfd=maildir_createmsg(DRAFTS, 0, &draftfilename);
	if (draftfd < 0)
	{
		fclose(fp);
		rfc2045_free(rfc2045p);
		enomem();
	}

	maildir_writemsgstr(draftfd, "From: ");
	{
	const char *f=pref_from;

		if (!f || !*f)	f=login_fromhdr();
		if (!f)	f="";
		maildir_writemsgstr(draftfd, f);
		maildir_writemsgstr(draftfd, "\n");
	}

	{
		char *ml=getmailinglists();
		struct rfc2045_mkreplyinfo ri;
		int rc;

		memset(&ri, 0, sizeof(ri));
		ri.fd=fileno(fp);
		ri.rfc2045partp=rfc2045partp;
		ri.replymode=replymode;
		ri.replysalut=replysalut;
		ri.forwardsep=forwardsep;
		ri.myaddr_func=ismyaddr;
		ri.write_func=writefunc;
		ri.writesig_func=newmsg_writesig;
		ri.mailinglists=ml;
		ri.charset=sqwebmail_content_charset;

		if (strcmp(replymode, "forward") == 0
		    || strcmp(replymode, "forwardatt") == 0)
		{
#if	HAVE_SQWEBMAIL_UNICODE
			rc=rfc2045_makereply_unicode(&ri);
#else
			rc=rfc2045_makereply(&ri);
#endif
		}
		else
		{
		char *basename=maildir_basename(filename);

			maildir_writemsgstr(draftfd, "X-Reply-To-Folder: ");
			maildir_writemsgstr(draftfd, folder);
			maildir_writemsgstr(draftfd, "\nX-Reply-To-Msg: ");

			maildir_writemsgstr(draftfd, basename);
			free(basename);
			maildir_writemsgstr(draftfd, "\n");

#if	HAVE_SQWEBMAIL_UNICODE
			rc=rfc2045_makereply_unicode(&ri);
#else
			rc=rfc2045_makereply(&ri);
#endif
		}
		free(ml);

		if (rc)
		{
			fclose(fp);
			close(draftfd);
			rfc2045_free(rfc2045p);
			enomem();
		}
	}

	fclose(fp);
	if (maildir_closemsg(draftfd, DRAFTS, draftfilename, 1, 0))
	{
		free(draftfilename);
		draftfilename=0;
		cgi_put("error", "quota");
	}
	free(filename);
	rfc2045_free(rfc2045p);
	return(draftfilename);
}
