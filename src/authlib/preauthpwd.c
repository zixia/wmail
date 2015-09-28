/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include	<pwd.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

#include	"auth.h"

static const char rcsid[]="$Id: preauthpwd.c,v 1.1.1.1 2003/05/07 02:14:40 lfan Exp $";

int auth_pwd_pre(const char *userid, const char *service,
	int (*callback)(struct authinfo *, void *),
			void *arg)
{
struct authinfo auth;
struct passwd *pw;

	memset(&auth, 0, sizeof(auth));

	if ((pw=getpwnam(userid)) == 0)
	{
		if (errno == ENOMEM)	return (1);
		return (-1);
	}

	auth.sysusername=userid;
	auth.sysgroupid=pw->pw_gid;
	auth.homedir=pw->pw_dir;
	auth.address=userid;
	auth.fullname=pw->pw_gecos;
	auth.passwd=pw->pw_passwd;

	return ((*callback)(&auth, arg));
}
