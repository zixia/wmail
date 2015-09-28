/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: export.c,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

#include	"config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>

#include	"gpg.h"
#include	"gpglib.h"

#include	"unicode/unicode.h"
#include	"numlib/numlib.h"

extern int gpg_stdin, gpg_stdout, gpg_stderr;
extern pid_t gpg_pid;

/*
** List keys
*/

int gpg_exportkey(const char *gpgdir,
		  int secret,
		  const char *fingerprint,
		  int (*out_func)(const char *, size_t, void *),
		  int (*err_func)(const char *, size_t, void *),
		  void *voidarg)
{
	char *argvec[5];
	int rc;

	argvec[0]="gpg";
	argvec[1]= secret ? "--export-secret-keys":"--export";
	argvec[2]="--armor";
	argvec[3]="--no-tty";
	argvec[4]=0;

	if (gpg_fork(&gpg_stdin, &gpg_stdout, &gpg_stderr, gpgdir, argvec) < 0)
		rc= -1;
	else
	{
		int rc2;

		close(gpg_stdin);
		gpg_stdin=-1;

		rc=gpg_read(out_func, err_func, NULL, 0, voidarg);
		rc2=gpg_cleanup();
		if (rc2)
			rc=rc2;
	}
	return (rc);
}
