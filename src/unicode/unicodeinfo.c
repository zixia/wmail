/*
** Copyright 2000-2003 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: unicodeinfo.c,v 1.1.1.1 2003/05/07 02:13:56 lfan Exp $
*/

#include	"unicode_config.h"
#include	"unicode.h"
#include	<string.h>
#include	<stdlib.h>
#include	<stdio.h>

int main()
{
int i;
const char *c, *d;

	for (i=0; unicode_chsetlist[i].chsetname; i++)
		printf("chset=%s\n", unicode_chsetlist[i].chsetname);

	c=unicode_default_chset();
	d=unicode_find(c)->chset;

	printf("default_chset=%s\n", c);
	printf("real_default_chset=%s\n", d);

	exit(0);
	return (0);
}
