/*
** $Id: html.h,v 1.1.1.1 2003/05/07 02:15:27 lfan Exp $
*/
#ifndef	html_h
#define	html_h

/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


extern void htmlfilter_init( void (*)(const char *, size_t));
extern void htmlfilter(const char *, size_t);
extern void htmlfilter_washlink(const char *p);
extern void htmlfilter_washlinkmailto(const char *p);
extern void htmlfilter_convertcid( char *(*)(const char *, void *), void *);
extern void htmlfilter_contentbase( const char *);

#endif
