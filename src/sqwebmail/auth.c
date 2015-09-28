/*
** Copyright 1998 - 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: auth.c,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/

#include	"sqwebmail.h"
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
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<string.h>
#include	<errno.h>

#include	"authlib/auth.h"
#include	"authlib/authmod.h"
#include	"authlib/authstaticlist.h"

#include	"auth.h"
#include	"htmllibdir.h"


extern int check_sqwebpass(const char *);

const char *myhostname()
{
char    buf[512];
static char *my_hostname=0;
FILE	*f;

	if (my_hostname == 0)
	{
		buf[0]=0;
		if ((f=fopen(HOSTNAMEFILE, "r")) != 0)
		{
		char *p;

			fgets(buf, sizeof(buf), f);
			fclose(f);

			if ((p=strchr(buf, '\n')) != 0)
				*p=0;
		}

		if (buf[0] == 0 && gethostname(buf, sizeof(buf)-1))
			strcpy(buf, "localhost");

		if ((my_hostname=malloc(strlen(buf)+1)) == 0)
			enomem();
		strcpy(my_hostname, buf);
	}
	return (my_hostname);
}

static int login_maildir(const char *maildir)
{
	if (!maildir || !*maildir)
		maildir="Maildir";
	if (chdir(maildir))	return (-1);
	return (0);
}

#if 0
struct preinfo {
	int i;
	int haswebpass;
	} ;

static int pre_callback(struct authinfo *authinfo, void *vp)
{
const char *h=authinfo->homedir;
struct preinfo *pi= (struct preinfo *)vp;

	if (!h)	return (-1);

	pi->i=authinfo->staticindex;
	pi->haswebpass=0;

#if ENABLE_WEBPASS
   {
const char *p=authinfo->maildir;
char	*buf;
struct stat stat_buf;

	if (!p || !*p)	p="Maildir";

	buf=malloc(strlen(h)+strlen(p)+sizeof(PASSFILE)+100);

	if (!buf)
	{
		perror("malloc");
		return (1);
	}
	if (*p == '/')
		strcpy(buf, p);
	else
		strcat(strcat(strcpy(buf, h), "/"), p);
	strcat(buf, "/" PASSFILE);

	if (stat(buf, &stat_buf) == 0)	pi->haswebpass=1;
	free(buf);
   }
#endif

	return (0);
}
#endif

static int doauthlogin(struct authinfo *a, void *vp)
{
        authsuccess(a->homedir, a->sysuserid ? 0:a->sysusername,
			a->sysuserid, &a->sysgroupid,
			a->address, a->fullname);
	if (login_maildir(a->maildir))
		return (-1);
	return (0);
}

static const char *do_login(const char *u, const char *p, const char **driver,
			    int *has_changepwd)
{
char	*buf;
static char	*uid=0;
int	i;

#if 0
struct	preinfo pi;
int	rc;

/*
** We first need to check if sqwebmail saved its own password in sqwebmail-pass.
** To do that, we call the authlib preauthentication functions until we find
** one that accepts our userid.  However, if it succeeds, we will no longer
** as root.  Therefore, let's fork, and do our thing in the child proc.
*/

	pi.haswebpass=0;
	pi.i=0;

	rc=authstaticlist_search(u, "webmail", MODULEFILE,
		&pre_callback, &pi);

	if (rc)
		return (0);

#if ENABLE_WEBPASS
	if (pi.haswebpass)
	{
		if ((*authstaticmodulelist[pi.i]->auth_prefunc)(u, "webmail",
                        &doauthlogin, 0))
		{
			return (0);
		}

		if (check_sqwebpass(p) == 0)
		{
			*driver=authstaticmodulelist[pi.i]->auth_name;
			*has_changepwd=
				authstaticmodulelist[pi.i]->auth_changepwd
				!= 0;
			return (u);
		}
		return (0);
	}
#endif

#endif

	/* Let the authlib library check the password instead, then we'll
	** adopt it as our own.
	*/

	if ((buf=malloc(strlen(u)+strlen(p)+3)) == 0)
		enomem();

	strcat(strcat(strcat(strcpy(buf, u), "\n"), p), "\n");

	if (uid)	free(uid);

	uid=authlogin_search(MODULEFILE, "webmail", AUTHTYPE_LOGIN, buf,
			     0, NULL, NULL, &i);
	free(buf);

	if (uid != 0)
	{
		if (login_maildir(getenv("MAILDIR")))
			error("Unable to open the maildir for this account -- the maildir doesn't exist or has incorrect ownership or permissions.");

		*driver=authstaticmodulelist[i]->auth_name;
		*has_changepwd=
			authstaticmodulelist[i]->auth_changepwd
			!= 0;
		return (uid);
	}
	return (0);
}


static int badstr(const char *p)
{
	while (*p)
	{
		if ((int)(unsigned char)*p < ' '
		    || *p == '\\' || *p == '\'' || *p == '"')
			return (1);
		++p;
	}
	return (0);
}

static void verifyuid(char *uid)
{
	if (badstr(uid))
	{
		free(uid);
		enomem();
	}
}

/*
** The returned mailboxid is of the form user.authdriver
**
** authdriver is included so that we can find the same authentication module
** quickly, for each request.  authdriver is appended, rather than prepended,
** because the logincache hashes on the first couple of characters of the id.
*/

const char *login(const char *uid, const char *pass, int *can_changepwd)
{
const char *driver;
const char *p=do_login(uid, pass, &driver, can_changepwd);
static char *login_id_buf=0;

	if (p)
	{
		if (badstr(uid) || badstr(pass))
			return (NULL);

		if (login_id_buf)	free(login_id_buf);
		if ((login_id_buf=malloc(strlen(p)+strlen(driver)+2)) == 0)
			enomem();
		p=strcat(strcat(strcpy(login_id_buf, p), "."), driver);
	}
	return (p);
}

/*
** login_changepwd tries to call the password change function for the
** authentication module.
**
** Returns -1 if authentication module does not have a password change
** function.
**
** Returns 0 if the authentication module has a password change function.
**
** *rc is set to 0 if password was changed, non-zero otherwise
*/

int login_changepwd(const char *u, const char *oldpwd, const char *newpwd,
		    int *rc)
{
	char *uid=strdup(u);
	char *driver;
	int	i;

	if (!uid)
		enomem();

	verifyuid(uid);

	if ((driver=strrchr(uid, '.')) == 0)
	{
		free(uid);
		enomem();
	}

	*driver++=0;

	for (i=0; authstaticmodulelist[i]; i++)
	{
		if (strcmp(authstaticmodulelist[i]->auth_name, driver) == 0)
		{
			*rc=badstr(uid) || badstr(oldpwd) || badstr(newpwd)
				? 1:(*authstaticmodulelist[i]->auth_changepwd)
				("webmail", uid, oldpwd, newpwd);

			free(uid);
			return (0);
		}
	}
	return (-1);
}

int prelogin(const char *u)
{
char	*p=malloc(strlen(u)+1);
char	*driver;
int	i;

	if (!p)	enomem();
	strcpy(p, u);

	verifyuid(p);

	if ((driver=strrchr(p, '.')) == 0)
	{
		free(p);
		enomem();
	}
	*driver++ = 0;

	for (i=0; authstaticmodulelist[i]; i++)
	{
		if (strcmp(authstaticmodulelist[i]->auth_name, driver) == 0)
		{
			if ((*authstaticmodulelist[i]->auth_prefunc)(p,
								     "webmail",
				&doauthlogin, 0) == 0)
			{
				free(p);
				return (0);
			}
			free(p);
			return (-1);
		}
	}
	free(p);
	return (-1);
}

const char *login_returnaddr()
{
const char *p=getenv("AUTHADDR");
const char *domain=myhostname();

static char *addrbuf=0;

	if (!p)	p="";
	if (addrbuf)	free(addrbuf);
	addrbuf=malloc(strlen(domain)+strlen(p)+2);
	if (!addrbuf)	enomem();
	strcpy(addrbuf, p);
	if (strchr(addrbuf, '@') == 0)
		strcat(strcat(addrbuf, "@"), domain);
	return (addrbuf);
}

const char *login_fromhdr()
{
const char *address=login_returnaddr();
const char *fullname=getenv("AUTHFULLNAME");
int	l;
const char *p;
char	*q;

static char *hdrbuf=0;

	if (!fullname || !*fullname)
		return (address);	/* That was easy */

	l=sizeof("\"\" <>")+strlen(address)+strlen(fullname);

	for (p=fullname; *p; p++)
		if (*p == '"' || *p == '\\' || *p == '(' || *p == ')' ||
			*p == '<' || *p == '>')	++l;

	for (p=address; *p; p++)
		if (*p == '"' || *p == '\\' || *p == '(' || *p == ')' ||
			*p == '<' || *p == '>')	++l;

	if (hdrbuf)	free(hdrbuf);
	hdrbuf=malloc(l);
	if (!hdrbuf)	enomem();
	q=hdrbuf;
	*q++='"';
	for (p=fullname; *p; p++)
	{
		if (*p == '"' || *p == '\\' || *p == '(' || *p == ')' ||
			*p == '<' || *p == '>')	*q++ = '\\';
		*q++= *p;
	}
	*q++='"';
	*q++=' ';
	*q++='<';
	for (p=address; *p; p++)
	{
		if (*p == '"' || *p == '\\' || *p == '(' || *p == ')' ||
			*p == '<' || *p == '>')	*q++ = '\\';
		*q++= *p;
	}
	*q++='>';
	*q=0;

	return (hdrbuf);
}

