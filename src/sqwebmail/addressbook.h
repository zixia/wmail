/*
** Copyright 2000 Double Precision, Inc.  See COPYING for
** distribution information.
*/
/*
** $Id: addressbook.h,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/
#ifndef	addressbook_h
#define	addressbook_h

#include	<stdio.h>

extern void addressbook();
extern void ab_listselect();
extern void ab_listselect_fp(FILE *);
extern const char *ab_find(const char *);
extern void ab_add(const char *name, const char *address, const char *nick);
extern void ab_addrselect();
extern int ab_get_nameaddr( int (*)(const char *, const char *, void *),
			    void *);
extern void ab_nameaddr_show(const char *, const char *);

#endif
