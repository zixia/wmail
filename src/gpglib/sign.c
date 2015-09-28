/*
** Copyright 2001-2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: sign.c,v 1.1.1.1 2003/05/07 02:14:58 lfan Exp $";

#include	"config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>
#if HAVE_FCNTL_H
#include	<fcntl.h>
#endif
#include	"gpg.h"
#include	"gpglib.h"

#include	"unicode/unicode.h"
#include	"numlib/numlib.h"

extern int gpg_stdin, gpg_stdout, gpg_stderr;
extern pid_t gpg_pid;


/*
** Sign a key.
*/

static int dosignkey(int (*)(const char *, size_t, void *),
		     const char *cmdstr,
		     void *);

int gpg_signkey(const char *gpgdir, const char *signthis, const char *signwith,
		int passphrase_fd,
		int (*dump_func)(const char *, size_t, void *),
		int trust_level,
		void *voidarg)
{
	char *argvec[12];
	int rc;
	char passphrase_fd_buf[NUMBUFSIZE];
	int i;

	argvec[0]="gpg";
	argvec[1]="--command-fd";
	argvec[2]="0";
	argvec[3]="--default-key";
	argvec[4]=(char *)signwith;
	argvec[5]="-q";
	argvec[6]="--no-tty";

	i=7;
	if (passphrase_fd >= 0 && fcntl(passphrase_fd, F_SETFD, 0) >= 0)
	{
		GPGARGV_PASSPHRASE_FD(argvec, i, passphrase_fd,
				      passphrase_fd_buf);
	}

	argvec[i++]="--sign-key";
	argvec[i++]=(char *)signthis;
	argvec[i]=0;

	if (gpg_fork(&gpg_stdin, &gpg_stdout, NULL, gpgdir, argvec) < 0)
		rc= -1;
	else
	{
		int rc2;

		char cmdstr[10];

#if GPG_HAS_CERT_CHECK_LEVEL

		cmdstr[0]='0';

		if (trust_level > 0 && trust_level <= 9)
			cmdstr[0]='0' + trust_level;

		strcpy(cmdstr+1, "\nY\n");

#else
		strcpy(cmdstr, "Y\n");
#endif

		rc=dosignkey(dump_func, cmdstr, voidarg);
		rc2=gpg_cleanup();
		if (rc2)
			rc=rc2;
	}
	return (rc);
}

static int dosignkey(int (*dump_func)(const char *, size_t, void *),
		     const char *cmdstr,
		     void *voidarg)
{
	int rc=gpg_write( cmdstr, strlen(cmdstr),
			 dump_func, NULL, NULL, 0, voidarg);
	int rc2;

	if (rc == 0)
		rc=gpg_read(dump_func, NULL, NULL, 0, voidarg);
	rc2=gpg_cleanup();
	if (rc == 0)
		rc=rc2;
	return (rc);
}
