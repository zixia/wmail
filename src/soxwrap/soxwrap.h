#ifndef	soxwrap_h
#define	soxwrap_h

/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

static const char soxwrap_h_rcsid[]="$Id: soxwrap.h,v 1.1.1.1 2003/05/07 02:14:31 lfan Exp $";

#include	"config.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if	SOX_DYNAMIC

#else
/*
** Simply map sox_ to the real system call.  If sockv5 is enabled, socks.h
** will wrap the system calls itself.
*/

#if	HAVE_SOCKS_H

#define	SOCKS
#include	<socks.h>

#define	sox_init	SOCKSinit
#else
extern int sox_init(char *);

#endif
#endif

#include        <sys/types.h>
#include        <sys/socket.h>
#include        <sys/uio.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include        <unistd.h>

#if	SOX_DYNAMIC

#include	"soxwrapproto.h"

#else


#define sox_getpeername	getpeername
#define sox_getsockname	getsockname
#define sox_accept	accept
#define sox_connect	connect
#define sox_bind	bind
#define sox_listen	listen
#define sox_recvfrom	recvfrom
#define sox_sendto	sendto
#define sox_read	read
#define sox_write	write
#define sox_close	close
#define sox_dup		dup
#define sox_dup2	dup2
#define sox_select	select

#endif

/* socket() itself doesn't need anything, ... yet */

#define	sox_socket	socket

#ifdef	__cplusplus
}
#endif
#endif
