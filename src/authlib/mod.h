/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include	"auth.h"
#include	"authmod.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<errno.h>

extern char *MODULE(const char *, const char *, char *, int,
	void (*)(struct authinfo *, void *), void *);

static const char mod_h_rcsid[]="$Id: mod.h,v 1.1.1.1 2003/05/07 02:14:39 lfan Exp $";

int main(int argc, char **argv)
{
const char *service, *type;
char *authdata;
char	*user;

	authmod_init(argc, argv, &service, &type, &authdata);
	user=MODULE(service, type, authdata, 1, 0, 0);
	if (!user || !*user)
	{
		if (user || errno != EPERM)
			authmod_fail_completely();

		authmod_fail(argc, argv);
	}
	authmod_success(argc, argv, user);
	return (0);
}

