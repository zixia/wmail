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
#include	"userdb/userdb.h"

static const char rcsid[]="$Id: preauthuserdbcommon.c,v 1.1.1.1 2003/05/07 02:14:40 lfan Exp $";

int auth_userdb_pre_common(const char *userid, const char *service,
	int needpass,
	int (*callback)(struct authinfo *, void *),
                        void *arg)
{
char	*u;
struct	userdbs *udb;
struct authinfo auth;
char	*udbs;
char	*services;
char	*passwords=0;
int	rc;

	userdb_init(USERDB ".dat");
        if ( (u=userdb(userid)) == 0)
	{
		userdb_close();
		return (-1);
	}

        if ((udb=userdb_creates(u)) == 0)
        {
		free(u);
                return (-1);
        }
	free(u);

        memset(&auth, 0, sizeof(auth));

        auth.sysusername=userid;
	auth.sysuserid= &udb->udb_uid;
        auth.sysgroupid=udb->udb_gid;
        auth.homedir=udb->udb_dir;
        auth.address=userid;
        auth.fullname=udb->udb_gecos;

	if (needpass)
	{
		udbs=userdbshadow(USERDB "shadow.dat", userid);

		if ((services=malloc(strlen(service)+sizeof("pw"))) == 0)
		{
			perror("malloc");
			if (udbs) free(udbs);
			userdb_frees(udb);
			return (1);
		}

		strcat(strcpy(services, service), "pw");

		passwords=udbs ? userdb_gets(udbs, services):0;
		free(services);

		if (passwords == 0)
			passwords=udbs ? userdb_gets(udbs, "systempw"):0;

		auth.passwd=passwords;
		if (udbs)	free(udbs);
	}

	auth.maildir=udb->udb_mailbox;
	auth.quota=udb->udb_quota;
	rc= (*callback)(&auth, arg);
	if (passwords)	free(passwords);
	userdb_frees(udb);
	return (rc);
}

void auth_userdb_cleanup()
{
	userdb_close();
}
