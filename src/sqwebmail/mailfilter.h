/*
** $Id: mailfilter.h,v 1.1.1.1 2003/05/07 02:15:29 lfan Exp $
*/
#ifndef	mailfilter_h
#define	mailfilter_h

/*
** Copyright 2000 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#include	"maildir/maildirfilter.h"

void mailfilter_init();
void mailfilter_list();
void mailfilter_listfolders();
void mailfilter_submit();

int mailfilter_autoreplyused(const char *);
int mailfilter_folderused(const char *);

#endif
