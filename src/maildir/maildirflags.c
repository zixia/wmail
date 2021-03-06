/*
** Copyright 2000-2002 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include	<sys/types.h>
#include	<string.h>
#include	"config.h"

static const char rcsid[]="$Id: maildirflags.c,v 1.1.1.1 2003/05/07 02:14:19 lfan Exp $";

int maildir_hasflag(const char *filename, char flag)
{
	const char *p=strrchr(filename, '/');

	if (p)
		filename=p+1;

	p=strrchr(filename, MDIRSEP[0]);
	if (p && strncmp(p, MDIRSEP "2,", 3) == 0 &&
	    strchr(p+3, flag))
		return (1);
	return (0);
}
