/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include	"auth.h"
#include	"authmod.h"
#include	"authldap.h"

static const char rcsid[]="$Id: preauthldap.c,v 1.1.1.1 2003/05/07 02:14:41 lfan Exp $";

int auth_ldap_pre(const char *userid, const char *service,
        int (*callback)(struct authinfo *, void *),
                        void *arg)
{
	return (authldapcommon(userid, 0, callback, arg));
}
