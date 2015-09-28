#ifndef mimegpgfork_h
#define mimegpgfork_h

/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char mimegpgfork_h_rcsid[]="$Id: mimegpgfork.h,v 1.1.1.1 2003/05/07 02:14:58 lfan Exp $";

#include "config.h"
#include <stdio.h>
#include <sys/types.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct gpgmime_forkinfo {
	int togpg_fd;
	int fromgpg_fd;
	int fromgpg_errfd;

	char gpg_writebuf[BUFSIZ];
	char gpg_errbuf[1024];

	unsigned gpg_writecnt;
	unsigned gpg_errcnt;

	int gpg_errflag;
	pid_t gpg_pid;

	int (*gpg_readhandler)(const char *, size_t, void *);
	void *gpg_voidarg;
} ;

int gpgmime_fork_signencrypt(const char *,	/* gpgdir */
			     int,	/* Flags: */

#define GPG_SE_SIGN	1
#define	GPG_SE_ENCRYPT	2

			     int, char **,	/* argc/argv */

			     int (*)(const char *, size_t, void *),
			     /* Encrypted output */
			     void *,	/* 3rd arg to encrypted output */

			     struct gpgmime_forkinfo *
			     /* Allocated struct */
			     );

int gpgmime_forkchecksign(const char *,	/* gpgdir */
			  const char *,	/* content filename */
			  const char *,	/* signature filename */
			  int, char **,	/* argc/argv */
			  struct gpgmime_forkinfo *);	/* Allocated struct */

int gpgmime_forkdecrypt(const char *,	/* gpgdir */
			int, char **,	/* argc/argv */
			int (*)(const char *, size_t, void *),
			/* Output callback function */
			void *,	/* 3rd arg to callback function */

			struct gpgmime_forkinfo *);	/* Allocated struct */

void gpgmime_write(struct gpgmime_forkinfo *, const char *, size_t);
int gpgmime_finish(struct gpgmime_forkinfo *);

const char *gpgmime_getoutput(struct gpgmime_forkinfo *);
const char *gpgmime_getcharset(struct gpgmime_forkinfo *);

#ifdef  __cplusplus
} ;
#endif

#endif
