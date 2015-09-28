#ifndef	soxwrapproto_h
#define	soxwrapproto_h

/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

static const char soxwrapproto_h_rcsid[]="$Id: soxwrapproto.h,v 1.1.1.1 2003/05/07 02:14:31 lfan Exp $";

#ifdef	__cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>


extern int sox_init(char *);
extern int sox_getpeername(int, struct sockaddr *, socklen_t *);
extern int sox_getsockname(int, struct sockaddr *, socklen_t *);
extern int sox_accept(int, struct sockaddr *, socklen_t *);
extern int sox_connect(int, const struct sockaddr *, socklen_t);
extern int sox_bind(int, const struct sockaddr *, socklen_t);
extern int sox_listen(int, int);
extern int sox_recvfrom(int, void *, size_t, int, struct sockaddr *,
		socklen_t *);
extern int sox_sendto(int, const void *, size_t, int,
	const struct sockaddr *, socklen_t);
extern int sox_read(int, void *, int);
extern int sox_write(int, const void *, int);
extern int sox_close(int);
extern int sox_dup(int);
extern int sox_dup2(int, int);
extern int sox_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#ifdef	__cplusplus
}
#endif

#endif
