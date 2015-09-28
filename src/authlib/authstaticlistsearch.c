/*
** Copyright 2000-2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include	"auth.h"
#include	"authstaticlist.h"
#include	<stdio.h>
#include	<errno.h>
#include	<ctype.h>
#include	<stdlib.h>
#include	<string.h>
#if	HAVE_FCNTL_H
#include	<fcntl.h>
#endif
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

static const char rcsid[]="$Id: authstaticlistsearch.c,v 1.1.1.1 2003/05/07 02:14:39 lfan Exp $";

int auth_changepass=AUTHCHANGEPASS;

static char *configfile=0;
static int has_init;

static void openconfigfile(const char *filename)
{
int	fd=open(filename, O_RDONLY);
char	buf[BUFSIZ];
int	i;

	has_init=1;
	if (fd < 0)
	{
		if (errno != ENOENT)
			perror(filename);
		return;
	}
	i=read(fd, buf, sizeof(buf));
	close(fd);
	if (i < 0)
	{
		perror(filename);
		return;
	}
	if ((configfile=malloc(i+1)) == 0)
	{
		perror("malloc");
		return;
	}
	memcpy(configfile, buf, i);
	configfile[i]=0;
}

struct callback_func {

	int (*callback)(struct authinfo *, void *);
		/* Original callback func */
	void *callback_arg;	/* Original callback arg */

	unsigned i;	/* Current index in authstaticlist */
	} ;

static int my_callback(struct authinfo *a, void *p)
{
struct callback_func *f=(struct callback_func *)p;

	a->staticindex=f->i;

	return ( (*f->callback)(a, f->callback_arg));
}

int authstaticlist_search(const char *userid, const char *service,
	const char *filename,
	int (*callback)(struct authinfo *, void *),
	void *callback_arg)
{
int	i, rc;
const char *p;
char	namebuf[32];	/* Names of authentication modules should fit in here */
struct callback_func c;

	c.callback=callback;
	c.callback_arg=callback_arg;

	if (!has_init)	openconfigfile(filename);

	if (!configfile)
	{
		for (i=0; authstaticmodulelist[i]; i++)
		{
			c.i=i;

			if ((rc=(*authstaticmodulelist[i]->auth_prefunc)
			     (
			      userid,
			      service,
			      &my_callback, &c)) == 0)
				return (0);

			if (rc > 0)	return (rc);
		}
		return (-1);
	}

	p=configfile;
	while (*p)
	{
		if ( isspace((int)(unsigned char)*p))
		{
			++p;
			continue;
		}

		for (i=0; p[i] && !isspace((int)(unsigned char)p[i]); i++)
			;
		namebuf[0]=0;
		strncat(namebuf, p, i < sizeof(namebuf)-1 ? i:
				sizeof(namebuf)-1);
		p += i;

		for (i=0; authstaticmodulelist[i]; i++)
		{
			if (strcmp(authstaticmodulelist[i]->auth_name,
				   namebuf))
				continue;

			c.i=i;

			if ((rc=(*authstaticmodulelist[i]->auth_prefunc)
			     (userid,
			      service,
			      &my_callback, &c)) >= 0)
				return (rc);
			break;
		}
	}
	return (-1);
}

char *authlogin_search(const char *configfilename,
		       const char *service,
		       const char *authtype,
		       const char *authdata,
		       int issession,
		       void (*callback_func)(struct authinfo *, void *),
		       void *callback_arg,
		       int *driver)
{
	int	i;
	const char *p;
	char	namebuf[32];
	/* Names of authentication modules should fit in here */
	char *authdata_cpy=strdup(authdata);

	if (!authdata_cpy)
		return (NULL);

	if (!has_init)	openconfigfile(configfilename);

	if (!configfile)
	{
		for (i=0; authstaticmodulelist[i]; i++)
		{
			char *uid=
				(*authstaticmodulelist[i]->
				 auth_func)(service,
					    authtype,
					    strcpy(authdata_cpy, authdata),
					    issession,
					    callback_func,
					    callback_arg);

			if (uid)
			{
				*driver=i;
				free(authdata_cpy);
				return (uid);
			}

			if (errno != EPERM)
				break;
		}
		free(authdata_cpy);
		return (NULL);
	}

	p=configfile;
	while (*p)
	{
		if ( isspace((int)(unsigned char)*p))
		{
			++p;
			continue;
		}

		for (i=0; p[i] && !isspace((int)(unsigned char)p[i]); i++)
			;
		namebuf[0]=0;
		strncat(namebuf, p, i < sizeof(namebuf)-1 ? i:
				sizeof(namebuf)-1);
		p += i;

		for (i=0; authstaticmodulelist[i]; i++)
		{
			char *uid;

			if (strcmp(authstaticmodulelist[i]->auth_name,
				   namebuf))
				continue;


			uid=(*authstaticmodulelist[i]->
			     auth_func)(service,
					authtype,
					strcpy(authdata_cpy, authdata),
					issession,
					callback_func,
					callback_arg);

			if (uid)
			{
				*driver=i;
				free(authdata_cpy);
				return (uid);
			}
			if (errno != EPERM)
				break;
		}
	}
	free(authdata_cpy);
	return (NULL);
}
