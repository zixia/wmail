/*
** Copyright 1998 - 2003 Double Precision, Inc.
** See COPYING for distribution information.
*/

/*
** $Id: cginocache.c,v 1.2 2003/06/13 04:55:31 lfan Exp $
*/
#if HAVE_LIBFCGI
#include <stdlib.h>
#include "fcgi_stdio.h"
#else
#include	<stdio.h>
#endif

void cginocache()
{
	printf("Cache-Control: no-store, max-age=0, s-maxage=0, proxy-revalidate, must-revalidate\n");
	printf("Expires: -1\n");
	printf("Pragma: no-cache\n");
	return;
		
	if (cgi_useragent("MSIE"))
		cginocache_msie();
	else
		cginocache_other();
}

void cginocache_other()
{
	printf("Cache-Control: no-store\n");
	printf("Pragma: no-cache\n");
}

/* MSIE sucks */

void cginocache_msie()
{
	printf("Cache-Control: private\n");
	printf("Pragma: private\n");
}
