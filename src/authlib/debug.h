#ifndef _debug_
#define _debug_

/*
** Copyright 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/
#if	HAVE_CONFIG_H
#include	"config.h"
#endif

#ifdef	__cplusplus
extern "C" {
#endif

static const char authdebug_h_rcsid[]="$Id: debug.h,v 1.1.1.1 2003/05/07 02:14:39 lfan Exp $";

#define DEBUG_LOGIN_ENV "DEBUG_LOGIN"
#define DEBUG_MESSAGE_SIZE (1<<10)

void auth_debug_login_init( void );
void auth_debug_login( int level, const char *fmt, ... );

extern int auth_debug_login_level;

#ifdef	__cplusplus
}
#endif

#endif
