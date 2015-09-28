/*
** $Id: cgi.h,v 1.1.1.1 2003/05/07 02:14:47 lfan Exp $
*/
#ifndef	cgi_h
#define	cgi_h

/*
** Copyright 1998 - 2003 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#if HAVE_LIBFCGI
#include <stdlib.h>
#include "fcgi_stdio.h"
#endif
extern void fake_exit(int);

void cgi_setup();
void cgi_cleanup();
const char *cgi(const char *);
char *cgi_multiple(const char *, const char *);

char	*cgi_cookie(const char *);
void	cgi_setcookie(const char *, const char *);

int	cgi_useragent(const char *);

struct cgi_arglist {
	struct cgi_arglist *next;
	struct cgi_arglist *prev;	/* Used by cgi_multiple */
	const char *argname;
	const char *argvalue;
	} ;

extern struct cgi_arglist *cgi_arglist;

extern void cgiurldecode(char *);
extern void cgi_put(const char *, const char *);

extern char *cgiurlencode(const char *);
extern char *cgiurlencode_noamp(const char *);
extern char *cgiurlencode_noeq(const char *);

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

int cgi_getfiles( int (*)(const char *, const char *, void *),
		int (*)(const char *, size_t, void *),
		void (*)(void *), size_t, void *);

extern const char *cgihttpscriptptr();
extern const char *cgihttpsscriptptr();
extern const char *cgirelscriptptr();
extern void cginocache(), cginocache_msie();
extern void cgiredirect();
extern void cgiversion(unsigned *, unsigned *);
extern int cgihasversion(unsigned, unsigned);

extern void cgiformdatatempdir(const char *);
		/* Specify directory for formdata temp file */
#endif
