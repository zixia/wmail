#ifndef	maildirgetquota_h
#define	maildirgetquota_h

/*
** Copyright 1998 - 1999 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#include	<sys/types.h>
#include	<stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

static const char maildirgetquota_h_rcsid[]="$Id: maildirgetquota.h,v 1.1.1.1 2003/05/07 02:14:19 lfan Exp $";

#define	QUOTABUFSIZE	256

int maildir_getquota(const char *, char [QUOTABUFSIZE]);

#ifdef  __cplusplus
}
#endif

#endif
