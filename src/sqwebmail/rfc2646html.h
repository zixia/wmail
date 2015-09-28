#ifndef	rfc2646html_h
#define	rfc2646html_h
/*
** Copyright 2000 Double Precision, Inc.  See COPYING for
** distribution information.
*/

/*
** $Id: rfc2646html.h,v 1.1.1.1 2003/05/07 02:15:31 lfan Exp $
*/

#include	"config.h"
#include	<stdlib.h>
#include	<string.h>


struct rfc2646tohtml {
	int current_quote_depth;
	int prev_was_flowed;
	int prev_was_0length;
	int (*handler)(const char *, int, void *);
	void *voidarg;
} ;

struct rfc2646tohtml *rfc2646tohtml_alloc( int (*)(const char *, int, void *),
					   void *);
int rfc2646tohtml_handler(struct rfc2646parser *, int, void *);
int rfc2646tohtml_free(struct rfc2646tohtml *);

#define RFC2646TOHTML_PARSEALLOC(p) \
	(rfc2646_alloc(&rfc2646tohtml_handler, (p)))

#endif
