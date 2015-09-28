/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: token.c,v 1.1.1.1 2003/05/07 02:15:32 lfan Exp $
*/
#include	"sqwebmail.h"
#include	"config.h"
#include	"token.h"
#include	"sqconfig.h"
#include	"cgi/cgi.h"
#include	<stdio.h>
#include	<string.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#if     TIME_WITH_SYS_TIME
#include        <sys/time.h>
#include        <time.h>
#else
#if     HAVE_SYS_TIME_H
#include        <sys/time.h>
#else
#include        <time.h>
#endif
#endif

void tokennew()
{
time_t	t;

	time(&t);
	printf("<INPUT TYPE=HIDDEN NAME=\"msgtoken\" VALUE=\"%ld-%ld\">",
		(long)t, (long)getpid());
}

void tokennewget()
{
time_t	t;

	time(&t);
	printf("&msgtoken=%ld-%ld", (long)t, (long)getpid());
}

int tokencheck()
{
const char *token=cgi("msgtoken");
const char *savedtoken;

	savedtoken=read_sqconfig(".", TOKENFILE, 0);
	if (savedtoken && strcmp(token, savedtoken) == 0)
		return (-1);
	return (0);
}

void tokensave()
{
	write_sqconfig(".", TOKENFILE, cgi("msgtoken"));
}

