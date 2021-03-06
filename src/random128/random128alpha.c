/*
** Copyright 1998 - 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<ctype.h>
#include	<string.h>
#include	"random128.h"

static const char rcsid[]="$Id: random128alpha.c,v 1.1.1.1 2003/05/07 02:13:27 lfan Exp $";

const char *random128_alpha()
{
static char randombuf[ 128 / 8 * 2 + 1];
char *p;

	strcpy(randombuf, random128());

	for (p=randombuf; *p; p++)
		if ( isdigit((int)(unsigned char)*p))
			*p= "GHIJKLMNOP"[ *p - '0' ];
	return (randombuf);
}
