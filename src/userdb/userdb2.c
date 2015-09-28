/*
** Copyright 1998 - 1999 Double Precision, Inc.
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

static const char rcsid[]="$Id: userdb2.c,v 1.1.1.1 2003/05/07 02:13:34 lfan Exp $";

char	*userdbshadow(const char *sh, const char *u)
{
struct dbobj d;
char	*p,*q;
size_t	l;

	dbobj_init(&d);

	if (dbobj_open(&d, sh, "R"))
		return (0);

	q=dbobj_fetch(&d, u, strlen(u), &l, "");
	dbobj_close(&d);
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
