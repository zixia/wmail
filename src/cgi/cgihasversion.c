/*
** Copyright 1998 - 1999 Double Precision, Inc.
** See COPYING for distribution information.
*/

/*
** $Id: cgihasversion.c,v 1.1.1.1 2003/05/07 02:14:47 lfan Exp $
*/
#include	"cgi.h"

int cgihasversion(unsigned major, unsigned minor)
{
unsigned vmajor, vminor;

	cgiversion(&vmajor, &vminor);
	return (vmajor > major || (vmajor == major && vminor >= minor));
}
