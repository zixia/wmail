/*
** Copyright 1998 - 1999 Double Precision, Inc.
** See COPYING for distribution information.
*/

/*
** $Id: cgihttpscriptptr.c,v 1.1.1.1 2003/05/07 02:14:47 lfan Exp $
*/
#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#include	"cgi.h"

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>

static const char *scriptptr=0;

extern void error(const char *);

const char *cgihttpscriptptr()
{
	if (!scriptptr)
	{
	char	*p=getenv("SCRIPT_NAME");
	char	*h=getenv("HTTP_HOST");
	char	*q;

		if (!h)	h="";
		if (!p)	p="";

		q=malloc(strlen(p)+strlen(h)+sizeof("http://"));
		if (!q)	error("Out of memory.");
		sprintf(q, "http:%s%s%s", (*h ? "//":""), h, p);
		scriptptr=q;
	}
	return (scriptptr);
}
