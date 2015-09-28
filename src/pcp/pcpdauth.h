#ifndef pcpdauth_h
#define pcpdauth_h

/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char pcpdauth_h_rcsid[]="$Id: pcpdauth.h,v 1.1.1.1 2003/05/07 02:15:10 lfan Exp $";

#include "config.h"
#include <sys/types.h>
#include <pwd.h>

struct userid_callback {
	const char *userid;
	const char *driver;

	const char *homedir;
	const char *maildir;
	uid_t uid;

	int (*callback_func)(struct userid_callback *, void *);
	void *callback_arg;
} ;

int auth_userid(const char *, int (*)(struct userid_callback *, void *),
		void *);

int auth_login(const char *, const char *,
	       int (*)(struct userid_callback *, void *),
	       void *);


char *auth_myhostname();
char *auth_choplocalhost(const char *);

#endif
