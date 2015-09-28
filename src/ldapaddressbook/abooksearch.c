#include	"config.h"
#include	"ldapaddressbook.h"

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h>
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#define exit(_a_) _exit(_a_)

int ldapabook_search(const struct ldapabook *a,
	const char *script,
	const char *filter,
	int stderr_fd)
{
	int	pipefd[2];
	pid_t	p;
	const char *argv[40];
	int	argn;
	const char *zopt=NULL;
	const char *xopt=NULL;
	const char *uopt=NULL;
	const char *oopt=NULL;
	const char *yopt=NULL;

	struct ldapabook_opts *opts;

	for (opts=a->opts; opts; opts=opts->next)
	{
		switch (opts->options[0]) {
		case SASL_SECURITY_PROPERTIES:
			oopt=opts->options+1;
			break;
		case SASL_AUTHENTICATION_ID:
			uopt=opts->options+1;
			break;
		case SASL_AUTHENTICATION_RID:
			xopt=opts->options+1;
			break;
		case SASL_AUTHENTICATION_MECHANISM:
			yopt=opts->options+1;
			break;
		case SASL_STARTTLS:
			zopt=strcmp(opts->options, "ZZ")
				? "-ZZ":"-Z";
			break;
		}
	}

	if (pipe(pipefd) < 0)	return (-1);
	if ((p=fork()) == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (-1);
	}

	if (p)	/* Parent waits for the first child to exit. */
	{
	int	waitstat;

		close(pipefd[1]);
		while (wait(&waitstat) != p)
			;
		return (pipefd[0]);
	}

	/* Child forks a second child */

	close(pipefd[0]);
	if (stderr_fd)
	{
		close(2);
		dup(stderr_fd);
		close(stderr_fd);
	}

	if ((p=fork()) == -1)
	{
		close(pipefd[1]);
		perror("fork");
		exit(0);
	}

	if (p)	/* Parent immediately exits */
		exit(0);
	close(1);
	dup(pipefd[1]);
	close(pipefd[1]);

	argn=0;
	argv[argn++]=script;
	if (*a->binddn)
	{
		argv[argn++]="-D";
		argv[argn++]=a->binddn;
	}
	if (*a->bindpw)
	{
		argv[argn++]="-w";
		argv[argn++]=a->bindpw;
	}
	if (*a->host)
	{
		argv[argn++]="-h";
		argv[argn++]=a->host;
	}
	if (*a->port)
	{
		argv[argn++]="-p";
		argv[argn++]=a->port;
	}
	if (*a->suffix)
	{
		argv[argn++]="-b";
		argv[argn++]=a->suffix;
	}

	if (zopt)
		argv[argn++]=zopt;

	if (oopt || uopt || xopt || yopt)
	{
		argv[argn++]="-Q";
		if (oopt)
		{
			argv[argn++]="-O";
			argv[argn++]=oopt;
		}

		if (uopt)
		{
			argv[argn++]="-U";
			argv[argn++]=uopt;
		}

		if (xopt)
		{
			argv[argn++]="-X";
			argv[argn++]=xopt;
		}

		if (yopt)
		{
			argv[argn++]="-Y";
			argv[argn++]=yopt;
		}
	}
	else
	{
		argv[argn++]="-x";
	}

	argv[argn++]=filter;
	argv[argn]=0;
	execvp(script, (char **)argv);
	perror(script);
	exit(0);
}
