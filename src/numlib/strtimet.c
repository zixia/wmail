/*
** Copyright 1998 - 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif
#include	"numlib.h"
#include	<string.h>

static const char rcsid[]="$Id: strtimet.c,v 1.1.1.1 2003/05/07 02:13:08 lfan Exp $";

char *libmail_str_time_t(time_t t, char *arg)
{
char	buf[NUMBUFSIZE];
char	*p=buf+sizeof(buf)-1;

	*p=0;
	do
	{
		*--p= '0' + (t % 10);
		t=t / 10;
	} while(t);
	return (strcpy(arg, p));
}
