/*
** Copyright 1998 - 2000 Double Precision, Inc.  See COPYING for
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
#include	"authstaticlist.h"

static const char rcsid[]="$Id: authpwd.c,v 1.1.1.1 2003/05/07 02:14:40 lfan Exp $";

extern int auth_pwd_pre(const char *userid, const char *service,
        int (*callback)(struct authinfo *, void *),
                        void *arg);

struct callback_info {
	const char *pass;
	char *userret;
	int issession;
	void (*callback_func)(struct authinfo *, void *);
	void *callback_arg;
	};

static int callback_pwd(struct authinfo *a, void *p)
{
struct callback_info *i=(struct callback_info *)p;

	if (a->passwd == 0 || authcheckpassword(i->pass, a->passwd))
		return (-1);

	if ((i->userret=strdup(a->sysusername)) == 0)
	{
		perror("malloc");
		return (1);
	}

	if (i->callback_func == 0)
		authsuccess(a->homedir, a->sysusername, 0, &a->sysgroupid,
			a->address, a->fullname);
	else
	{
		a->address=a->sysusername;
		(*i->callback_func)(a, i->callback_arg);
	}

	return (0);
}

char *auth_pwd(const char *service, const char *authtype, char *authdata,
	int issession,
	void (*callback_func)(struct authinfo *, void *), void *callback_arg)
{
const char *user, *pass;
struct callback_info ci;
int	rc;

	if (strcmp(authtype, AUTHTYPE_LOGIN) ||
		(user=strtok(authdata, "\n")) == 0 ||
		(pass=strtok(0, "\n")) == 0)
	{
		errno=EPERM;
		return (0);
	}

	ci.pass=pass;
	ci.issession=issession;
	ci.callback_func=callback_func;
	ci.callback_arg=callback_arg;
	rc=auth_pwd_pre(user, service, &callback_pwd, &ci);

	if (rc < 0)
	{
		errno=EPERM;
		return (0);
	}
	if (rc > 0)
	{
		errno=EACCES;
		return (0);
	}

	if (putenv("MAILDIR="))
	{
		perror("putenv");
		free(ci.userret);
		return (0);
	}
	return (ci.userret);
}

static void auth_pwd_cleanup()
{
#if	HAVE_ENDPWENT

	endpwent();
#endif
}

struct authstaticinfo authpwd_info={
	"authpwd",
	auth_pwd,
	auth_pwd_pre,
	auth_pwd_cleanup,
	auth_syspasswd,
	NULL,
} ;
