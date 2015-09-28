/*
** Copyright 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: pcpdauthme.c,v 1.1.1.1 2003/05/07 02:15:10 lfan Exp $
*/

#include	"config.h"

#if	HAVE_SYSLOG_H
#include	<syslog.h>
#endif

#if	HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<pwd.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<string.h>
#include	<errno.h>
#include	<sysconfdir.h>
#include	"calendardir.h"

char *auth_myhostname()
{
char    buf[1024];
static char *my_hostname=0;
FILE	*f;

	if (my_hostname == 0)
	{
		buf[0]=0;

		f=fopen(HOSTNAMEFILE, "r");

		if (!f)
			f=fopen(HOSTNAMEFILE2, "r");

		if (f != 0)
		{
		char *p;

			fgets(buf, sizeof(buf), f);
			fclose(f);

			if ((p=strchr(buf, '\n')) != 0)
				*p=0;
		}

		if (buf[0] == 0 && gethostname(buf, sizeof(buf)-1))
			strcpy(buf, "localhost");

		if ((my_hostname=malloc(strlen(buf)+1)) == 0)
		{
			syslog(LOG_NOTICE, "malloc: out of memory.");
			return ("localhost");
		}

		strcpy(my_hostname, buf);
	}
	return (my_hostname);
}
