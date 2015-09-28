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
#include	"vpopmail_config.h"
#include	"vpopmail.h"
#include	"vauth.h"

static const char rcsid[]="$Id: authvchkpw.c,v 1.1.1.1 2003/05/07 02:14:38 lfan Exp $";

extern int auth_vchkpw_pre(const char *userid, const char *service,
        int (*callback)(struct authinfo *, void *),
                        void *arg);

extern char *authvchkpw_isvirtual(char *);

extern FILE *authvchkpw_file(const char *, const char *);

struct callback_info {
	const char *pass;
	char *userret;
	int issession;
	void (*callback_func)(struct authinfo *, void *);
	void *callback_arg;
	};

static int callback_vchkpw(struct authinfo *a, void *p)
{
struct callback_info *i=(struct callback_info *)p;

	if (a->passwd == 0 || authcheckpassword(i->pass, a->passwd))
		return (-1);

	if ((i->userret=strdup(a->address)) == 0)
	{
		perror("malloc");
		return (1);
	}

	if (i->callback_func == 0)
	{
		authsuccess(a->homedir, 0, a->sysuserid, &a->sysgroupid,
			a->address, a->fullname);
		putenv("MAILDIR=");
	}
	else
		(*i->callback_func)(a, i->callback_arg);

        return (0);
}

char *auth_vchkpw(const char *service, const char *authtype, char *authdata,
	int issession,
	void (*callback_func)(struct authinfo *, void *), void *callback_arg)
{
char *user, *pass;
int	rc;
struct	callback_info	ci;

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
	rc=auth_vchkpw_pre(user, service, &callback_vchkpw, &ci);

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

	putenv("MAILDIR=");

	return (ci.userret);
}

static void authvchkpwclose()
{
}

static int auth_vchkpw_changepass(const char *service,
				  const char *username,
				  const char *pass,
				  const char *npass)
{
	struct vqpasswd *vpw;
	char	*s, *q;
	uid_t	uid;
	gid_t	gid;
	char	*usercopy;

	usercopy=strdup(username);
	if (!usercopy)
	{
		perror("strdup");
		errno=EPERM;
		return (-1);
	}

	if ((s=authvchkpw_isvirtual(usercopy)) != 0)
	{
	char	*t;

		*s++=0;
		while ((t=strchr(s, '/')) != 0)
			*t='.';
	}
	else
	{
		s = DEFAULT_DOMAIN;
	}

        vget_assign(s,NULL,0,&uid, &gid);
        vpw=vauth_getpw(usercopy, s);

	if (vpw == NULL)
	{
		free(usercopy);
		errno=ENOENT;
		return (-1);
	}

	if (vpw->pw_passwd == 0 || authcheckpassword(pass, vpw->pw_passwd)
	    || (q=strdup(npass)) == 0)
	{
		free(usercopy);
		errno=EPERM;
		return (-1);
	}

	vpasswd(usercopy, s, q, 0);
	return (0);
}

struct authstaticinfo authvchkpw_info={
	"authvchkpw",
	auth_vchkpw,
	auth_vchkpw_pre,
	authvchkpwclose,
	auth_vchkpw_changepass,
	NULL};

