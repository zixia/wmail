/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: import.c,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

#include	"config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
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
** Import a key.
*/

int gpg_import_start(const char *gpgdir, int secret)
{
	char *argvec[5];

	argvec[0]="gpg";
	argvec[1]="--import";
	argvec[2]="--no-tty";
	argvec[3]=0;

#if GPG_HAS_ALLOW_SECRET_KEY_IMPORT
	if (secret)
	{
		argvec[3]="--allow-secret-key-import";
		argvec[4]=0;
	}
#endif

	if (gpg_fork(&gpg_stdin, &gpg_stdout, NULL, gpgdir, argvec) < 0)
		return (-1);
	return (0);
}

int gpg_import_do(const char *p, size_t n,
		  int (*dump_func)(const char *, size_t, void *),
		  void *voidarg)
{
	if (gpg_write(p, n, dump_func, dump_func, NULL, 0, voidarg))
	{
		gpg_cleanup();
		return (-1);
	}
	return (0);
}

int gpg_import_finish(int (*dump_func)(const char *, size_t, void *),
		      void *voidarg)
{
	if (gpg_read(dump_func, dump_func, NULL, 0, voidarg))
		return (-1);
	return (gpg_cleanup());
}
