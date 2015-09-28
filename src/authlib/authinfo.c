/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include	"auth.h"
#include	"authstaticlist.h"
#include	"authsasl.h"
#include	<stdio.h>

static const char rcsid[]="$Id: authinfo.c,v 1.1.1.1 2003/05/07 02:14:39 lfan Exp $";

extern struct authstaticinfo *authdaemonmodulelist[];

int main()
{
int	i;

	printf("AUTHENTICATION_MODULES=\"");
	for (i=0; authstaticmodulelist[i]; i++)
		printf("%s%s", i ? " ":"", authstaticmodulelist[i]->auth_name);
	printf("\"\n");

	if (authdaemonmodulelist[0])
	{
		printf("AUTHDAEMONMODULELIST=\"");
		for (i=0; authdaemonmodulelist[i]; i++)
			printf("%s%s", i ? " ":"", authdaemonmodulelist[i]->auth_name);
		printf("\"\n");
	}

	printf("SASL_AUTHENTICATION_MODULES=\"");
	for (i=0; authsasl_list[i].sasl_method; i++)
		printf("%s%s", i ? " ":"", authsasl_list[i].sasl_method);
	printf("\"\n");
	return (0);
}
