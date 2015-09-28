/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: libgpg.c,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

#include	"config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#if	HAVE_SYS_TIME_H
#include	<sys/time.h>
#endif
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#include	"gpg.h"
#include	"gpglib.h"

int gpg_stdin= -1, gpg_stdout= -1, gpg_stderr= -1;
pid_t gpg_pid= -1;

int gpg_cleanup()
{
	int rc=0;

	if (gpg_stdin >= 0)
	{
		close(gpg_stdin);
		gpg_stdin= -1;
	}

	if (gpg_stdout >= 0)
	{
		close(gpg_stdout);
		gpg_stdout= -1;
	}

	if (gpg_stderr >= 0)
	{
		close(gpg_stderr);
		gpg_stderr= -1;
	}

	if (gpg_pid >= 0 && kill(gpg_pid, 0) >= 0)
	{
		int waitstat;
		pid_t p;

		while ((p=wait(&waitstat)) != gpg_pid)
		{
			if (p < 0 && errno == ECHILD)
				return (-1);
		}

		rc= WIFEXITED(waitstat) ? WEXITSTATUS(waitstat): -1;
		gpg_pid= -1;
	}
	return (rc);
}

static char *optionfile(const char *gpgdir)
{
	char *p=malloc(strlen(gpgdir)+sizeof("/options"));

	if (p)
		strcat(strcpy(p, gpgdir), "/options");
	return (p);
}

/*
** Determine of GPG is available by checking for the options file, and the
** gpg binary itself.
*/

int has_gpg(const char *gpgdir)
{
	char *p=optionfile(gpgdir);
	struct stat stat_buf;
	int rc;

	if (!p)
		return (-1);

	rc=stat(p, &stat_buf);
	free(p);
	if (rc == 0)
		rc=stat(GPG, &stat_buf);

	return (rc);
}
