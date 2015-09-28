/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: strncasecmp.c,v 1.1.1.1 2003/05/07 02:15:33 lfan Exp $
*/
#include	<ctype.h>
#include	<string.h>

int strncasecmp(const char *a, const char *b, size_t n)
{
	while (n && (*a || *b))
	{
	int	ca=toupper(*a);
	int	cb=toupper(*b);

		if (ca < cb)	return (-1);
		if (ca > cb)	return (1);
		++a;
		++b;
		--n;
	}
	return (0);
}
