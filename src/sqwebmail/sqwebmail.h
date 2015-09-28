/*
** $Id: sqwebmail.h,v 1.1.1.1 2003/05/07 02:15:32 lfan Exp $
*/
#ifndef	sqwebmail_h
#define	sqwebmail_h

/*
** Copyright 1998 - 2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/


#if	HAVE_CONFIG_H
#undef	PACKAGE
#undef	VERSION
#include	"config.h"
#endif

extern void error(const char *), error2(const char *, int);

#define	enomem()	error2(__FILE__,__LINE__)

/* Location of the user's Maildir */

#define USER_DIR	"Maildir"

/* For PAM-based authentication */

#define	SQWEBMAIL_PAM	"webmail"

/* Where we keep the sqwebmail-specific password */

#if 0
#define	PASSFILE	"sqwebmail-pass"
#endif

/* Where we keep the IP address we authenticated from */

#define	IPFILE		"sqwebmail-ip"

/* File that keeps the time of last access */

#define	TIMESTAMP	"sqwebmail-timestamp"

/* Various configuration stuff */

#define	CONFIGFILE	"sqwebmail-config"

/* More configuration stuff */

#define GPGCONFIGFILE	"sqwebmail-gpgconfig"

/* Eliminate duplicate messages being sent based on form reloads by using
** unique message tokens.
*/

#define	TOKENFILE	"sqwebmail-token"

/* Sig file */

#define	SIGNATURE	"sqwebmail-sig"

#define	CHECKFILENAME(p) { if (!*p || strchr((p), '/') || *p == '.') enomem(); }

/* Wrap lines for new messages */
#define	MYLINESIZE	76

/* Wrap lines for received messages */

#define	EXTLINESIZE	80

/* Automake dribble */

#ifndef	HAVE_STRDUP
extern char *strdup(const char *);
#endif

#ifndef	HAVE_STRCASECMP
extern int strcasecmp(const char *, const char *);
#endif

#ifndef	HAVE_STRNCASECMP
extern int strncasecmp(const char *, const char *, size_t);
#endif

extern void cleanup();

extern void http_redirect_argu(const char *, unsigned long);
extern void http_redirect_argss(const char *, const char *, const char *);
extern void http_redirect_argsss(const char *, const char *, const char *,
				const char *);

#define	ISCTRL(c)	((unsigned char)(c) < (unsigned char)' ')

#if HAVE_LIBFCGI
#include <stdlib.h>
#include "fcgi_stdio.h"
#endif
extern void fake_exit(int);

extern void addarg(const char *);
extern void freeargs();
extern void insert_include(const char *);
extern const char *getarg(const char *);

#define	GPGDIR "gpg"

#define	MIMEGPGFILENAME "mimegpgfilename"

#endif
