/*
** Copyright 1998 - 1999 Double Precision, Inc.
** See COPYING for distribution information.
*/

/*
** $Id: cgiredirect.c,v 1.1.1.1 2003/05/07 02:14:47 lfan Exp $
*/
#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#include	"cgi.h"
#include	<stdio.h>

void cgiredirect(const char *buf)
{
	if (cgihasversion(1,1))
		printf("Status: 303 Moved\n");
				/* Alas, Communicator can't handle it */
	cginocache();
	printf("URI: %s\n", buf);
	printf("Location: %s\n\n", buf);
}
