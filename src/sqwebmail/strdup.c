/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: strdup.c,v 1.1.1.1 2003/05/07 02:15:33 lfan Exp $
*/
#include	<stdlib.h>
#include	<string.h>

char *strdup(const char *p)
{
char *s;

	if ((s=malloc(strlen(p)+1)) != 0)	strcpy(s, p);
	return (s);
}
