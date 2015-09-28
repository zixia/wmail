/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include	"config.h"
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>

static const char rcsid[]="$Id: maildirshared2.c,v 1.1.1.1 2003/05/07 02:14:20 lfan Exp $";

FILE *maildir_shared_fopen(const char *maildir, const char *mode)
{
char	*m;
FILE	*fp;

	m=malloc(strlen(maildir)+sizeof("/shared-maildirs"));
	if (!m)
	{
		perror("malloc");
		return (0);
	}
	strcat(strcpy(m, maildir), "/shared-maildirs");

	fp=fopen(m, mode);
	free(m);
	return (fp);
}

void maildir_shared_fparse(char *p, char **name, char **dir)
{
char	*q;

	*name=0;
	*dir=0;

	if ((q=strchr(p, '\n')) != 0)	*q=0;
	if ((q=strchr(p, '#')) != 0)	*q=0;

	for (q=p; *q; q++)
		if (isspace((int)(unsigned char)*q))	break;
	if (!*q)	return;
	*q++=0;
	while (*q && isspace((int)(unsigned char)*q))
		++q;
	if (*q)
	{
		*name=p;
		*dir=q;
	}
}
