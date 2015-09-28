/*
** $Id: newmsg.h,v 1.1.1.1 2003/05/07 02:15:29 lfan Exp $
*/
#ifndef	newmsg_h
#define	newmsg_h

#if	HAVE_CONFIG_H
#undef	PACKAGE
#undef	VERSION
#include	"config.h"
#endif

/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


#include	<stdlib.h>

extern void newmsg_init(const char *, const char *);
extern void newmsg_do(const char *);
extern void preview_start();
extern int preview_callback(const char *, size_t, void *);
extern void preview_end();

extern char *newmsg_createdraft_do(const char *, const char *, int);
#define NEWMSG_SQISPELL	1
#define NEWMSG_PCP	2

#endif
