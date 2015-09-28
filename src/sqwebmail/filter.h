/*
** $Id: filter.h,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/
#ifndef	filter_h
#define	filter_h

/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
	The filter set of function is used to format message text for
	display, and to process text entered by the user.

	filter_start(mode, handler)

	- mode selects what kind of processing is to be done.  It is one of
	the following:

	FILTER_FOR_DISPLAY - format message contents for display.  The message
	contents must be word-wrapped if necessary (newlines inserted to
	limit line length).  It is possible that the message is already
	word-wrapped.  Special characters like <, >, &, and ", *must* be
	represented as HTML escape codes: &lt; &gt; &amp; and &quot;

	FILTER_FOR_PREVIEW - like FILTER_FOR_DISPLAY, except word wrapping
	occurs for shorter lines (76 characters).

	FILTER_FOR_SAVING - format message contents for saving into a
	file.  Do not line wrap.

	- handler is the output function which will be called.  The output
	function.


	filter(ptr, cnt) - repeated calls to this function are used to
	supply text being filtered.

	filter_end() - is called when the end of the text being filtered
	is reached.

*/
#define	FILTER_FOR_SAVING	0
#define	FILTER_FOR_SENDING	1
#define	FILTER_FOR_DISPLAY	2
#define	FILTER_FOR_PREVIEW	3

#if	HAVE_CONFIG_H
#undef	PACKAGE
#undef	VERSION
#include	"config.h"
#endif

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

#include	<stdlib.h>

void	filter_start(int, void (*)(const char *, size_t));
void	filter(const char *, size_t);
void	filter_end(void);

#endif
