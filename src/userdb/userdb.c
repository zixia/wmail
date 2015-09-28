/*
** Copyright 1998 - 2001 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif
#include	"dbobj.h"
#include	"userdb.h"
#include	<string.h>
#include	<stdlib.h>
#include	<errno.h>
#include	<time.h>
#include	<sys/types.h>
#if	HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif

static const char rcsid[]="$Id: userdb.c,v 1.1.1.1 2003/05/07 02:13:34 lfan Exp $";

static struct dbobj d;
static time_t dt;
static ino_t di;

static int initialized=0;

/* Open userdb.dat, if already opened, see if it changed, if so reopen */

void userdb_init(const char *n)
{
struct	stat	stat_buf;

	if (initialized)
	{
		if (stat(n, &stat_buf) ||
			stat_buf.st_mtime != dt ||
			stat_buf.st_ino != di)
		{
			dbobj_close(&d);
			initialized=0;
			dt=stat_buf.st_mtime;
			di=stat_buf.st_ino;
		}
	}
	else if (stat(n, &stat_buf))
	{
		return;
	}
	else
	{
		dt=stat_buf.st_mtime;
		di=stat_buf.st_ino;
	}

	if (!initialized)
	{
		if (dbobj_open(&d, n, "R"))
		{
			return;
		}
		initialized=1;
	}
}

void userdb_close()
{
	if (initialized)
	{
		dbobj_close(&d);
		initialized=0;
	}
}

/* Fetch a record from userdb.dat */

char	*userdb(const char *u)
{
char	*p,*q;
size_t	l;

	if (!initialized)
	{
		errno=ENOENT;
		return (0);
	}

	q=dbobj_fetch(&d, u, strlen(u), &l, "");
	if (!q)
	{
		errno=ENOENT;
		return(0);
	}

	p=malloc(l+1);
	if (!p)	return (0);

	if (l)	memcpy(p, q, l);
	free(q);
	p[l]=0;
	return (p);
}

/* Return a pointer to a specific field in this record */

const char	*userdb_get(const char *u, const char *n, int *l)
{
int	nl=strlen(n);

	while (u && *u)
	{
		if (memcmp(u, n, nl) == 0 &&
			(u[nl] == 0 || u[nl] == '=' || u[nl] == '|'))
		{
			u += nl;
			*l=0;
			if (*u == '=')
			{
				++u;
				while ( u[*l] && u[*l] != '|')
					++ *l;
			}
			return (u);
		}
		u=strchr(u, '|');
		if (u)	++u;
	}
	return (0);
}

/* Extract field as an unsigned int */

unsigned userdb_getu(const char *u, const char *n, unsigned defnum)
{
	int l;
	const char *p;

	if ((p=userdb_get(u, n, &l)) != 0)
	{
		defnum=0;
		while (l && *p >= '0' && *p <= '9')
		{
			defnum = defnum * 10 + (*p++ - '0');
			--l;
		}
	}
	return (defnum);
}

/* Extract a field into a dynamically allocated buffer */

char *userdb_gets(const char *u, const char *n)
{
	int l;
	const char *p;
	char	*q;

	if ((p=userdb_get(u, n, &l)) != 0)
	{
		q=malloc(l+1);
		if (!q)
			return (0);

		if (l)	memcpy(q, p, l);
		q[l]=0;
		return (q);
	}
	errno=ENOENT;
	return (0);
}

/* Create a userdbs structure based upon a uid (reverse lookup) */

struct userdbs *userdb_createsuid(uid_t u)
{
char	buf[80];
char	*p=buf+sizeof(buf)-1, *q;
struct	userdbs *s;

	/* Lookup uid= record */

	*p=0;
	*--p='=';
	do
	{
		*--p= "0123456789"[u % 10];
		u=u/10;
	} while (u);
	p=userdb(p);
	if (!p)	return (0);

	/* Have account name, now look it up. */

	q=userdb(p);
	if (!q)
	{
		free(p);
		return (0);
	}
	s=userdb_creates(q);
	if (s)
		s->udb_name=p;
	else
		free(p);
	free(q);
	return (s);
}

/* Extracted a userdb.dat record, convert it to a userdbs structure */

struct userdbs *userdb_creates(const char *u)
{
struct userdbs *udbs=(struct userdbs *)malloc(sizeof(struct userdbs));
char	*s;

	if (!udbs)	return (0);
	memset((char *)udbs, 0, sizeof(*udbs));

	if ((udbs->udb_dir=userdb_gets(u, "home")) == 0)
	{
		userdb_frees(udbs);
		return (0);
	}

	if ((s=userdb_gets(u, "uid")) != 0)
	{
		udbs->udb_uid=atol(s);
		free(s);
		if ((s=userdb_gets(u, "gid")) != 0)
		{
			udbs->udb_gid=atol(s);
			free(s);

			if ((s=userdb_gets(u, "shell")) != 0)
				udbs->udb_shell=s;
			else if (errno != ENOENT)
			{
				userdb_frees(udbs);
				return (0);
			}

			if ((s=userdb_gets(u, "mail")) != 0)
				udbs->udb_mailbox=s;
			else if (errno != ENOENT)
			{
				userdb_frees(udbs);
				return (0);
			}
			if ((s=userdb_gets(u, "quota")) != 0)
				udbs->udb_quota=s;
			else if (errno != ENOENT)
			{
				userdb_frees(udbs);
				return (0);
			}
			if ((s=userdb_gets(u, "gecos")) != 0)
				udbs->udb_gecos=s;
			else if (errno != ENOENT)
			{
				userdb_frees(udbs);
				return (0);
			}
			udbs->udb_source=userdb_gets(u, "_");
			return (udbs);
		}
	}
	userdb_frees(udbs);
	return (0);
}

void	userdb_frees(struct userdbs *u)
{
	if (u->udb_name)	free(u->udb_name);
	if (u->udb_gecos)	free(u->udb_gecos);
	if (u->udb_dir)		free(u->udb_dir);
	if (u->udb_shell)	free(u->udb_shell);
	if (u->udb_mailbox)	free(u->udb_mailbox);
	if (u->udb_quota)	free(u->udb_quota);
	if (u->udb_source)	free(u->udb_source);
	free(u);
}

