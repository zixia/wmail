/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: fork.c,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

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

extern int gpg_stdin, gpg_stdout, gpg_stderr;
extern pid_t gpg_pid;


/*
** Helper function: for and run pgp, with the given file descriptors and
** options.
*/

pid_t gpg_fork(int *gpg_stdin, int *gpg_stdout, int *gpg_stderr,
	       const char *gpgdir,
	       char **argvec)
{
	int pipein[2], pipeout[2], pipeerr[2];
	pid_t p;
	char *s;

	if (gpg_stdin && pipe(pipein) < 0)
		return (-1);

	if (gpg_stdout && pipe(pipeout) < 0)
	{
		if (gpg_stdin)
		{
			close(pipein[0]);
			close(pipein[1]);
		}
		return (-1);
	}

	if (gpg_stderr && pipe(pipeerr) < 0)
	{
		if (gpg_stdout)
		{
			close(pipeout[0]);
			close(pipeout[1]);
		}

		if (gpg_stdin)
		{
			close(pipein[0]);
			close(pipein[1]);
		}
		return (-1);
	}

	signal(SIGCHLD, SIG_DFL);
	p=gpg_pid=fork();
	if (p < 0)
	{
		if (gpg_stderr)
		{
			close(pipeerr[0]);
			close(pipeerr[1]);
		}

		if (gpg_stdout)
		{
			close(pipeout[0]);
			close(pipeout[1]);
		}
		if (gpg_stdin)
		{
			close(pipein[0]);
			close(pipein[1]);
		}

		return (-1);
	}

	if (p)
	{
		signal(SIGPIPE, SIG_IGN);

		if (gpg_stderr)
		{
			close(pipeerr[1]);
			*gpg_stderr=pipeerr[0];
		}

		if (gpg_stdout)
		{
			close(pipeout[1]);
			*gpg_stdout=pipeout[0];
		}

		if (gpg_stdin)
		{
			close(pipein[0]);
			*gpg_stdin=pipein[1];
		}
		return (0);
	}

	if (gpg_stderr)
	{
		close(2);
		dup(pipeerr[1]);
		close(pipeerr[0]);
		close(pipeerr[1]);
	}
	else if (gpg_stdout)
	{
		close(2);
		dup(pipeout[1]);
	}

	if (gpg_stdout)
	{
		close(1);
		dup(pipeout[1]);
		close(pipeout[0]);
		close(pipeout[1]);
	}

	if (gpg_stdin)
	{
		close(0);
		dup(pipein[0]);
		close(pipein[0]);
		close(pipein[1]);
	}

	if (gpgdir)
	{
		s=malloc(sizeof("GNUPGHOME=")+strlen(gpgdir));
		if (!s)
		{
			perror("malloc");
			exit(1);
		}
		strcat(strcpy(s, "GNUPGHOME="), gpgdir);
		if (putenv(s) < 0)
		{
			perror("putenv");
			exit(1);
		}
	}

	{
		const char *gpg=getenv("GPG");
		if (!gpg || !*gpg)
			gpg=GPG;

		execv(gpg, argvec);
		perror(gpg);
	}
	exit(1);
	return (0);
}

int gpg_write(const char *p, size_t cnt,
	      int (*stdout_func)(const char *, size_t, void *),
	      int (*stderr_func)(const char *, size_t, void *),
	      int (*timeout_func)(void *),
	      unsigned timeout,
	      void *voidarg)
{
	char buf[BUFSIZ];

	fd_set fdr, fdw;
	struct timeval tv;

	if (!timeout_func)
		timeout=0;
	while (cnt)
	{
		int maxfd=0;
		int n;

		FD_ZERO(&fdr);
		FD_ZERO(&fdw);

		FD_SET(gpg_stdin, &fdw);

		if (gpg_stdout >= 0)
		{
			FD_SET(gpg_stdout, &fdr);
			if (gpg_stdout > maxfd)
				maxfd=gpg_stdout;
		}

		if (gpg_stderr >= 0)
		{
			FD_SET(gpg_stderr, &fdr);
			if (gpg_stderr > maxfd)
				maxfd=gpg_stderr;
		}

		tv.tv_usec=0;
		tv.tv_sec=timeout;
		n=select(maxfd+1, &fdr, &fdw, NULL, timeout ? &tv:NULL);
		if (n == 0)
		{
			n=(*timeout_func)(voidarg);
			if (n)
				return(n);
			continue;
		}
		if (n < 0)
			continue;

		if (FD_ISSET(gpg_stdin, &fdw))
		{
			int n=write(gpg_stdin, p, cnt);

			if (n <= 0)
				return (-1);

			p += n;
			cnt -= n;
		}

		if (gpg_stdout >= 0 && FD_ISSET(gpg_stdout, &fdr))
		{
			int n=read(gpg_stdout, buf, sizeof(buf));

			if (n <= 0)
			{
				close(gpg_stdout);
				gpg_stdout= -1;
			}
			else if (stdout_func &&
				 (n=(*stdout_func)(buf, n, voidarg)) != 0)
				return (n);
		}

		if (gpg_stderr >= 0 && FD_ISSET(gpg_stderr, &fdr))
		{
			int n=read(gpg_stderr, buf, sizeof(buf));

			if (n <= 0)
			{
				close(gpg_stderr);
				gpg_stderr= -1;
			}
			else if (stderr_func &&
				 (n=(*stderr_func)(buf, n, voidarg)) != 0)
				return (n);
		}
	}
	return (0);
}

int gpg_read(int (*stdout_func)(const char *, size_t, void *),
	     int (*stderr_func)(const char *, size_t, void *),
	     int (*timeout_func)(void *),
	     unsigned timeout,
	     void *voidarg)
{
	char buf[BUFSIZ];

	fd_set fdr;
	struct timeval tv;

	if (gpg_stdin >= 0)
	{
		close(gpg_stdin);
		gpg_stdin= -1;
	}

	if (!timeout_func)
		timeout=0;

	while ( gpg_stdout >= 0 || gpg_stderr >= 0)
	{
		int maxfd=0;
		int n;

		FD_ZERO(&fdr);

		if (gpg_stdout >= 0)
		{
			FD_SET(gpg_stdout, &fdr);
			if (gpg_stdout > maxfd)
				maxfd=gpg_stdout;
		}

		if (gpg_stderr >= 0)
		{
			FD_SET(gpg_stderr, &fdr);
			if (gpg_stderr > maxfd)
				maxfd=gpg_stderr;
		}

		tv.tv_usec=0;
		tv.tv_sec=timeout;
		n=select(maxfd+1, &fdr, NULL, NULL, timeout ? &tv:NULL);

		if (n == 0)
		{
			n=(*timeout_func)(voidarg);
			if (n)
				return(n);
			continue;
		}
		if (n < 0)
			continue;

		if (gpg_stdout >= 0 && FD_ISSET(gpg_stdout, &fdr))
		{
			int n=read(gpg_stdout, buf, sizeof(buf));

			if (n <= 0)
			{
				close(gpg_stdout);
				gpg_stdout= -1;
			}
			else if (stdout_func &&
				 (n=(*stdout_func)(buf, n, voidarg)) != 0)
				return (n);
		}

		if (gpg_stderr >= 0 && FD_ISSET(gpg_stderr, &fdr))
		{
			int n=read(gpg_stderr, buf, sizeof(buf));

			if (n <= 0)
			{
				close(gpg_stderr);
				gpg_stderr= -1;
			}
			else if (stderr_func &&
				 (n=(*stderr_func)(buf, n, voidarg)) != 0)
				return (n);
		}
	}
	return (0);
}
