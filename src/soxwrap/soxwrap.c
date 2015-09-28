/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include	"config.h"
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/uio.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<errno.h>
#include	<unistd.h>

#if	HAVE_DLFCN_H
#include	<dlfcn.h>
#endif

static const char rcsid[]="$Id: soxwrap.c,v 1.1.1.1 2003/05/07 02:14:31 lfan Exp $";

#if	HAVE_DANTE
#define	SHLIB	"libdsocks.so"
#else
#define	SHLIB	"libsocks5_sh.so"
#endif

#if	SOX_DYNAMIC

/* create ptrs to wrapped functions in libsocks5_sh.so wrapper */

#include	"soxwrapproto.h"

extern char sox_n_init[],
	sox_n_getpeername[],
	sox_n_getsockname[],
	sox_n_accept[],
	sox_n_recvfrom[],
	sox_n_connect[],
	sox_n_listen[],
	sox_n_select[],
	sox_n_bind[],
	sox_n_close[],
	sox_n_dup[],
	sox_n_dup2[],
	sox_n_sendto[],
	sox_n_read[],
	sox_n_write[];

struct funcptr {
	char *funcname;
	void *defaultptr;
	void *dlptr;
	} ;

static int fake_sox_init(char *p)
{
	return (0);
}

struct funcptr init_ptr = {sox_n_init, (void *)&fake_sox_init};

#define	DEF(n)	struct funcptr init_ ## n = { sox_n_ ##n, (void *)&n };

DEF(getpeername)
DEF(getsockname)
DEF(accept)
DEF(recvfrom)
DEF(connect)
DEF(listen)
DEF(select)
DEF(bind)
DEF(close)
DEF(dup)
DEF(dup2)
DEF(sendto)
DEF(read)
DEF(write)

struct funcptr *funcarray[]={
	&init_ptr,
	&init_getpeername,
	&init_getsockname,
	&init_accept,
	&init_recvfrom,
	&init_connect,
	&init_listen,
	&init_select,
	&init_bind,
	&init_close,
	&init_dup,
	&init_dup2,
	&init_sendto,
	&init_read,
	&init_write,
	0 };

/*
** On first access, attempt to load libsocks5_sh.so and resolve the
** function addresses.
*/

static int dl_tried=0;
static void *dlptr=0;

static void *dl_get(struct funcptr *ptr)
{
const char *p;
int	i;
int	havesock5sh=0;

	dl_tried=1;

#define	CHECK(z)	if ( access(z, 0) == 0) havesock5sh=1;
#include	"libsock5confdir.h"
#undef	CHECK

	if (havesock5sh)
	{
		int	i=0;

		dlptr=dlopen(SHLIB,
#ifdef RTLD_NOW
			     RTLD_NOW
#else
			     0
#endif
			     );
		if (!dlptr)
		{

/*
** if libsocks5_sh.so is not there, I choose to kill the process, because
** this maybe an unintentional misconfiguration, and forging ahead may
** result in a security issue.
*/
			fprintf(stderr, SHLIB ": %s\n",
				dlerror());
			exit(1);
		}

		for (i=0; funcarray[i]; i++)
		{
			if ((funcarray[i]->dlptr=dlsym(dlptr,
				funcarray[i]->funcname)) == 0)
			{
#if HAVE_DANTE
				if (strcmp(funcarray[i]->funcname, "close")
				    == 0)
				{
				/* Dante doesn't wrap close() */

					funcarray[i]->dlptr=
						funcarray[i]->defaultptr;
					continue;
				}
#endif
				fprintf(stderr, "%s: %s\n",
					funcarray[i]->funcname,
					dlerror());
				exit(1);
			}
		}
	}
	else	/* SOCKS disabled, not available */

		for (i=0; funcarray[i]; i++)
			funcarray[i]->dlptr=
				funcarray[i]->defaultptr;

	return ( ptr->dlptr );
}

/*
	Borrrrring...
*/

#define	FUNCTION(z)	( dl_tried ? (z)->dlptr:dl_get(z))

int sox_init(char *p)
{
int (*func)(char *)=
	(int (*)(char *))FUNCTION(&init_ptr);

	return ((*func)(p));
}

int sox_getpeername(int fd, struct sockaddr *a, socklen_t *al)
{
int (*func)(int, struct sockaddr *, socklen_t *)=
	(int (*)(int, struct sockaddr *, socklen_t *))FUNCTION(&init_getpeername);

	return ((*func)(fd, a, al));
}

int sox_getsockname(int fd, struct sockaddr *a, socklen_t *al)
{
int (*func)(int, struct sockaddr *, socklen_t *)=
	(int (*)(int, struct sockaddr *, socklen_t *))FUNCTION(&init_getsockname);

	return ((*func)(fd, a, al));
}

int sox_accept(int fd, struct sockaddr *a, socklen_t *al)
{
int (*func)(int, struct sockaddr *, socklen_t *)=
	(int (*)(int, struct sockaddr *, socklen_t *))FUNCTION(&init_accept);

	return ((*func)(fd, a, al));
}

int sox_connect(int fd, const struct sockaddr *a, socklen_t al)
{
int (*func)(int, const struct sockaddr *, socklen_t)=
	(int (*)(int, const struct sockaddr *, socklen_t))FUNCTION(&init_connect);

	return ((*func)(fd, a, al));
}

int sox_bind(int fd, const struct sockaddr *a, socklen_t al)
{
int (*func)(int, const struct sockaddr *, socklen_t)=
	(int (*)(int, const struct sockaddr *, socklen_t))FUNCTION(&init_bind);

	return ((*func)(fd, a, al));
}

int sox_listen(int fd, int ql)
{
int (*func)(int, int)=
	(int (*)(int, int))FUNCTION(&init_listen);

	return ((*func)(fd, ql));
}

int sox_recvfrom(int fd, void *fromp, size_t froml,
	int flags, struct sockaddr *a, socklen_t *al)
{
int (*func)(int , void *, size_t, int, struct sockaddr *, socklen_t *)=
	(int (*)(int , void *, size_t, int, struct sockaddr *, socklen_t *))
		FUNCTION(&init_recvfrom);

	return ((*func)(fd, fromp, froml, flags, a, al));

}

int sox_sendto(int fd, const void *top, size_t tol, int flags,
	const struct sockaddr *a, socklen_t al)
{
int (*func)(int, const void *, size_t, int, const struct sockaddr *, socklen_t)=
	(int (*)(int, const void *, size_t, int, const struct sockaddr *,
		socklen_t))FUNCTION(&init_sendto);

	return ((*func)(fd, top, tol, flags, a, al));

}

int sox_read(int fd, void *buf, int cnt)
{
int (*func)(int, void *, int)=
	(int (*)(int, void *, int))FUNCTION(&init_read);

	return ((*func)(fd, buf, cnt));

}

int sox_write(int fd, const void *buf, int cnt)
{
int (*func)(int, const void *, int)=
	(int (*)(int, const void *, int))FUNCTION(&init_write);

	return ((*func)(fd, buf, cnt));

}

int sox_close(int fd)
{
int (*func)(int)=
	(int (*)(int))FUNCTION(&init_close);

	return ((*func)(fd));

}

int sox_dup(int fd)
{
int (*func)(int)=
	(int (*)(int))FUNCTION(&init_dup);

	return ((*func)(fd));

}

int sox_dup2(int fd1, int fd2)
{
int (*func)(int, int)=
	(int (*)(int, int))FUNCTION(&init_dup2);

	return ((*func)(fd1, fd2));

}

int sox_select(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
int (*func)(int, fd_set *, fd_set *, fd_set *, struct timeval *)=
	(int (*)(int, fd_set *, fd_set *, fd_set *, struct timeval *))FUNCTION(
		&init_select);

	return ((*func)(fd, r, w, e, t));

}

#endif
