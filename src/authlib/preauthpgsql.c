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
#include	"authmod.h"
#include	"authpgsql.h"


static const char rcsid[]="$Id: preauthpgsql.c,v 1.1.1.1 2003/05/07 02:14:41 lfan Exp $";


int	auth_pgsql_pre(const char *user, const char *service,
		int (*callback)(struct authinfo *, void *), void *arg)
{
struct authpgsqluserinfo *authinfo;
struct	authinfo	aa;

	authinfo=auth_pgsql_getuserinfo(user);

	if (!authinfo)		/* Fatal error - such as MySQL being down */
		return (1);

	if (!authinfo->home)	/* User not found */
		return (-1);

	memset(&aa, 0, sizeof(aa));

	/*aa.sysusername=user;*/
	aa.sysuserid= &authinfo->uid;
	aa.sysgroupid= authinfo->gid;
	aa.homedir=authinfo->home;
	aa.maildir=authinfo->maildir && authinfo->maildir[0] ?
		authinfo->maildir:0;
	aa.address=authinfo->username;
	aa.passwd=authinfo->cryptpw;
	aa.clearpasswd=authinfo->clearpw;
	aa.fullname=authinfo->fullname;
	aa.quota=authinfo->quota && authinfo->quota[0] ?
		authinfo->quota:0;
	return ((*callback)(&aa, arg));
}
