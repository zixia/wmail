/*
** Copyright 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include "debug.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char rcsid[]="$Id: debug.c,v 1.1.1.1 2003/05/07 02:14:39 lfan Exp $";

// for internal use
void auth_debug( const char *fmt, va_list ap );

// 0 - dont debug
// 1 - debug auth
// 2 - debug auth + write out passwords

int auth_debug_login_level = 0;

// purpose: initialize debugging
// function: read environment variable DEBUG_LOGIN
//           and set up debugging according to it
// args: none

void auth_debug_login_init( void )
{
	const char *p=getenv(DEBUG_LOGIN_ENV);

	auth_debug_login_level = atoi( p ? p:"0" );
}

// purpose: print debug messages to logger - handy use
// function: take message with logging level and drop
//           messages with too high level.
//           also include into the message.
// args:
// * level - level to be compared with DEBUG_LOGIN env var.
// * fmt - message format as like in printf().
// * ... - and "arguments" for fmt

void auth_debug_login( int level, const char *fmt, ... ) {

	va_list ap;

	// logging severity
	if( level > auth_debug_login_level )
		return;

	fprintf( stderr, "LOGIN: DEBUG: ip=[%s], ", getenv("TCPREMOTEIP") );
	va_start( ap, fmt );
	auth_debug( fmt, ap );
	va_end( ap );
}

// purpose: print debug messages to logger - general use
// function: read format string and arguments
//           and convert them to suitable form for output.
// args:
// fmt - printf() format string
// ... - variable arguments

void auth_debug( const char *fmt, va_list ap )
{

	char	buf[DEBUG_MESSAGE_SIZE];
	int	i;
	int	len;

	// print into buffer to be able to replace control and other unwanted chars.
	vsnprintf( buf, DEBUG_MESSAGE_SIZE, fmt, ap );
	len = strlen( buf );

	// replace nonprintable chars by dot
	for( i=0 ; i<len ; i++ )
		if( !isprint(buf[i]) )
			buf[i] = '.';

	// emit it
	fprintf( stderr, buf );
	fprintf( stderr, "\n" );
}
