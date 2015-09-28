/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: pcpdauth.c,v 1.1.1.1 2003/05/07 02:15:10 lfan Exp $
*/

#include	"config.h"

#if	HAVE_SYSLOG_H
#include	<syslog.h>
#endif

#if	HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<pwd.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<string.h>
#include	<errno.h>
#include	<sysconfdir.h>

#include	"authlib/auth.h"
#include	"authlib/authmod.h"
#include	"authlib/authstaticlist.h"

#include	"pcpdauth.h"
#include	"calendardir.h"

#define MODULEFILE SYSCONFDIR "/authmodulelist"

char *auth_choplocalhost(const char *u)
{
	const char *p;
	char *s;

	p=strrchr(u, '@');
	if (p && strcasecmp(p+1, auth_myhostname()) == 0)
	{
		s=malloc(p-u+1);
		if (s)
		{
			memcpy(s, u, p-u);
			s[p-u]=0;
		}
	}
	else
		s=strdup(u);

	if (!s)
		syslog(LOG_ALERT, "pcpd: malloc failed.");
	return (s);
}

static int badstr(const char *p)
{
	while (*p)
	{
		if ((int)(unsigned char)*p < ' '
		    || *p == '\\' || *p == '\'' || *p == '"')
		{
			errno=EIO;
			return (1);
		}
		++p;
	}
	return (0);
}

static int callback_userid(struct authinfo *a, void *vp)
{
	struct userid_callback *uid=(struct userid_callback *)vp;

	uid->driver=authstaticmodulelist[a->staticindex]->auth_name;
	uid->homedir=a->homedir;
	uid->maildir=a->maildir;

	if (a->sysuserid)
		uid->uid= *a->sysuserid;
	else if (!a->sysusername)
		return (-1);
	else
	{
		struct passwd *pw=getpwnam(a->sysusername);

		if (!pw)
			return (-1);
		uid->uid=pw->pw_uid;
	}
	return ( (*uid->callback_func)(uid, uid->callback_arg));
}

static void callback_login(struct authinfo *a, void *vp)
{
	struct userid_callback *uid=(struct userid_callback *)vp;

	if (callback_userid(a, vp))
		uid->driver=NULL;
}

int auth_userid(const char *u, int (*func)(struct userid_callback *, void *),
		void *funcarg)
{
	char *s=NULL;
	int rc;
	struct userid_callback uinfo;

	if (badstr(u))
		return (-1);

	s=auth_choplocalhost(u);
	if (!s)
		return (1);

	memset(&uinfo, 0, sizeof(uinfo));
	uinfo.userid=s;
	uinfo.callback_func=func;
	uinfo.callback_arg=funcarg;

	rc=authstaticlist_search(s, "calendar", MODULEFILE,
				 callback_userid, &uinfo);

	free(s);
	return (rc);
}


/*
** The returned mailboxid is of the form user.authdriver
**
** authdriver is included so that we can find the same authentication module
** quickly, for each request.  authdriver is appended, rather than prepended,
** because the logincache hashes on the first couple of characters of the id.
*/

int auth_login(const char *uid, const char *pass,
	       int (*func)(struct userid_callback *, void *),
	       void *arg)
{
	struct userid_callback uinfo;
	char	*buf;
	static char	*uidbuf=0;
	int	i;

	memset(&uinfo, 0, sizeof(uinfo));
	uinfo.callback_func=func;
	uinfo.callback_arg=arg;

	if (badstr(uid) || badstr(pass))
		return (-1);


	/* Let the authlib library check the password instead, then we'll
	** adopt it as our own.
	*/

	if ((buf=malloc(strlen(uid)+strlen(pass)+3)) == 0)
		return (-1);

	strcat(strcat(strcat(strcpy(buf, uid), "\n"), pass), "\n");

	if (uidbuf)	free(uidbuf);

	uidbuf=authlogin_search(MODULEFILE, "calendar", AUTHTYPE_LOGIN, buf,
				0, callback_login, &uinfo, &i);
	free(buf);

	if (uidbuf && uinfo.driver)
	{
		uinfo.userid=uidbuf;
		return (0);
	}
	errno=EPERM;
	return (-1);
}
