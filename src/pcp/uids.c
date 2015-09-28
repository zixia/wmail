/*
** Copyright 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: uids.c,v 1.1.1.1 2003/05/07 02:15:08 lfan Exp $";

#include "config.h"
#include "pcp.h"
#include "uids.h"

const char *pcpuid()
{
	return uid;
}

const char *pcpgid()
{
	return gid;
}
