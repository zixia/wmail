/*
** Copyright 2000-2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include	"config.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/socket.h>
#include	<sys/un.h>
#include	<sys/time.h>
#include	<sys/wait.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<errno.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<syslog.h>
#include	"numlib/numlib.h"
#include	"liblock/config.h"
#include	"liblock/liblock.h"
#include	"auth.h"
#include	"authmod.h"
#include	"authdaemonrc.h"

static const char rcsid[]="$Id: authdaemond.c,v 1.1.1.1 2003/05/07 02:14:40 lfan Exp $";

#ifndef	SOMAXCONN
#define	SOMAXCONN	5
#endif

#if HAVE_HMACLIB
#include        "../libhmac/hmac.h"
#include        "cramlib.h"
#endif

#include	"authstaticlist.h"

static char *authmodulelist;
static unsigned ndaemons;

static int mksocket()
{
int	fd=socket(PF_UNIX, SOCK_STREAM, 0);
struct	sockaddr_un skun;

	if (fd < 0)	return (-1);
	skun.sun_family=AF_UNIX;
	strcpy(skun.sun_path, AUTHDAEMONSOCK);
	strcat(skun.sun_path, ".tmp");
	unlink(skun.sun_path);
	if (bind(fd, (const struct sockaddr *)&skun, sizeof(skun)) ||
		listen(fd, SOMAXCONN) ||
		chmod(skun.sun_path, 0777) ||
		rename(skun.sun_path, AUTHDAEMONSOCK) ||
		fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		perror(AUTHDAEMONSOCK);
		close(fd);
		return (-1);
	}
	return (fd);
}

static int readconfig()
{
char	buf[BUFSIZ];
FILE	*fp;
char	*modlist=0;
unsigned daemons=1;

	if ((fp=fopen(AUTHDAEMONRC, "r")) == NULL)
	{
		perror(AUTHDAEMONRC);
		return (-1);
	}

	while (fgets(buf, sizeof(buf), fp))
	{
	char	*p=strchr(buf, '\n'), *q;

		if (!p)
		{
		int	c;

			while ((c=getc(fp)) >= 0 && c != '\n')
				;
		}
		else	*p=0;
		if ((p=strchr(buf, '#')) != 0)	*p=0;

		for (p=buf; *p; p++)
			if (!isspace((int)(unsigned char)*p))
				break;
		if (*p == 0)	continue;

		if ((p=strchr(buf, '=')) == 0)
		{
			syslog(LOG_ERR, "authdaemon: Bad line in %s: %s\n",
				AUTHDAEMONRC, buf);
			fclose(fp);
			if (modlist)
				free(modlist);
			return (-1);
		}
		*p++=0;
		while (*p && isspace((int)(unsigned char)*p))
			++p;
		if (*p == '"')
		{
			++p;
			q=strchr(p, '"');
			if (q)	*q=0;
		}
		if (strcmp(buf, "authmodulelist") == 0)
		{
			if (modlist)
				free(modlist);
			modlist=strdup(p);
			if (!modlist)
			{
				perror("malloc");
				fclose(fp);
				return (-1);
			}
			continue;
		}
		if (strcmp(buf, "daemons") == 0)
		{
			daemons=atoi(p);
			continue;
		}
	}
	fclose(fp);

	if (daemons <= 0)
		daemons=1;
	syslog(LOG_INFO, "authdaemon: modules=\"%s\", daemons=%u\n",
		modlist ? modlist:"(none)",
		daemons);
	ndaemons=daemons;
	if (authmodulelist)	free(authmodulelist);
	authmodulelist=modlist;
	return (0);
}


static char buf[BUFSIZ];
static char *readptr;
static int readleft;
static char *writeptr;
static int writeleft;

static int getauthc(int fd)
{
fd_set	fds;
struct	timeval	tv;

	if (readleft--)
		return ( (int)(unsigned char)*readptr++ );

	readleft=0;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec=10;
	tv.tv_usec=0;
	if (select(fd+1, &fds, 0, 0, &tv) <= 0 ||
		!FD_ISSET(fd, &fds))
		return (EOF);
	readleft=read(fd, buf, sizeof(buf));
	readptr=buf;
	if (readleft <= 0)
	{
		readleft=0;
		return (EOF);
	}
	--readleft;
	return ( (int)(unsigned char)*readptr++ );
}

static int writeauth(int fd, const char *p, unsigned pl)
{
fd_set	fds;
struct	timeval	tv;

	while (pl)
	{
	int	n;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec=10;
		tv.tv_usec=0;
		if (select(fd+1, 0, &fds, 0, &tv) <= 0 ||
			!FD_ISSET(fd, &fds))
			return (-1);
		n=write(fd, p, pl);
		if (n <= 0)	return (-1);
		p += n;
		pl -= n;
	}
	return (0);
}

static int writeauthflush(int fd)
{
	if (writeptr > buf)
	{
		if (writeauth(fd, buf, writeptr - buf))
			return (-1);
	}
	writeptr=buf;
	writeleft=sizeof(buf);
	return (0);
}

static int writeauthbuf(int fd, const char *p, unsigned pl)
{
unsigned n;

	while (pl)
	{
		if (pl < writeleft)
		{
			memcpy(writeptr, p, pl);
			writeptr += pl;
			writeleft -= pl;
			return (0);
		}

		if (writeauthflush(fd))	return (-1);

		n=pl;
		if (n > writeleft)	n=writeleft;
		memcpy(writeptr, p, n);
		p += n;
		writeptr += n;
		writeleft -= n;
		pl -= n;
	}
	return (0);
}

static int writeenvval(int fd, const char *env, const char *val)
{
	if (writeauthbuf(fd, env, strlen(env)) ||
		writeauthbuf(fd, "=", 1) ||
		writeauthbuf(fd, val, strlen(val)) ||
		writeauthbuf(fd, "\n", 1))
		return (-1);
	return (0);
}

static int printauth(struct authinfo *authinfo, void *vp)
{
int	fd= *(int *)vp;
char	buf2[NUMBUFSIZE];

	writeptr=buf;
	writeleft=sizeof(buf);

	if (authinfo->sysusername)
		if (writeenvval(fd, "USERNAME", authinfo->sysusername))
			return (1);
	if (authinfo->sysuserid)
		if (writeenvval(fd, "UID", libmail_str_uid_t(*authinfo->sysuserid,
			buf2)))
			return (1);
	if (writeenvval(fd, "GID", libmail_str_uid_t(authinfo->sysgroupid, buf2)))
			return (1);

	if (writeenvval(fd, "HOME", authinfo->homedir))
			return (1);
	if (authinfo->address)
		if (writeenvval(fd, "ADDRESS", authinfo->address))
			return (1);
	if (authinfo->fullname)
		if (writeenvval(fd, "NAME", authinfo->fullname))
			return (1);
	if (authinfo->maildir)
		if (writeenvval(fd, "MAILDIR", authinfo->maildir))
			return (1);
	if (authinfo->quota)
		if (writeenvval(fd, "QUOTA", authinfo->quota))
			return (1);
	if (authinfo->passwd)
		if (writeenvval(fd, "PASSWD", authinfo->passwd))
			return (1);
	if (authinfo->clearpasswd)
		if (writeenvval(fd, "PASSWD2", authinfo->clearpasswd))
			return (1);
	if (writeauthbuf(fd, ".\n", 2) || writeauthflush(fd))
		return (1);
	return (0);
}

static void printauth_void(struct authinfo *authinfo, void *vp)
{
	printauth(authinfo, vp);
}

static const char *authnextptr;
static int authnexti;

static void authbegin()
{
	authnextptr=authmodulelist;
	authnexti=0;
}

static int authnext()
{
	if (authmodulelist == 0)
	{
		if (authstaticmodulelist[authnexti] == 0)
			return (-1);
		return (authnexti++);
	}

	for (;;)
	{
	int	i;

		while (*authnextptr &&
			isspace((int)(unsigned char)*authnextptr))
			++authnextptr;
		if (*authnextptr == 0)	return (-1);
		for (i=0; authnextptr[i] &&
			!isspace((int)(unsigned char)authnextptr[i]); i++)
			;

		for (authnexti=0; authstaticmodulelist[authnexti];
				authnexti++)
			if (strncmp(authstaticmodulelist[authnexti]->auth_name,
				    authnextptr, i) == 0 &&
			    authstaticmodulelist[authnexti]->auth_name[i]
			    == 0)
			{
				authnextptr += i;
				return (authnexti);
			}
		authnextptr += i;
	}
}

static void pre(int fd, char *prebuf)
{
char	*p=strchr(prebuf, ' ');
char	*service;
int	i;

	if (!p)	return;
	*p++=0;
	while (*p == ' ')	++p;
	service=p;
	p=strchr(p, ' ');
	if (!p) return;
        *p++=0;
        while (*p == ' ')     ++p;

	for (authbegin(); (i=authnext()) >= 0; )
	{
	int	rc;

		if (strcmp(prebuf, ".") && strcmp(prebuf,
			authstaticmodulelist[i]->auth_name))
			continue;

		rc=(*authstaticmodulelist[i]->auth_prefunc)(p, service,
                        &printauth, &fd);
		if (rc >= 0)	return;
	}
	writeauth(fd, "FAIL\n", 5);
}

static void dopasswd(int, const char *, const char *, const char *,
		     const char *);

static void passwd(int fd, char *prebuf)
{
	char *p;
	const char *service;
	const char *userid;
	const char *opwd;
	const char *npwd;

	for (p=prebuf; *p; p++)
	{
		if (*p == '\t')
			continue;
		if ((int)(unsigned char)*p < ' '
		    || *p == '\'' || *p == '"')
		{
			writeauth(fd, "FAIL\n", 5);
			return;
		}
	}

	service=prebuf;

	if ((p=strchr(service, '\t')) != 0)
	{
		*p++=0;
		userid=p;
		if ((p=strchr(p, '\t')) != 0)
		{
			*p++=0;
			opwd=p;
			if ((p=strchr(p, '\t')) != 0)
			{
				*p++=0;
				npwd=p;
				if ((p=strchr(p, '\t')) != 0)
					*p=0;
				dopasswd(fd, service, userid, opwd, npwd);
				return;
			}
		}
	}
}

static void dopasswd(int fd,
		     const char *service,
		     const char *userid,
		     const char *opwd,
		     const char *npwd)
{
	int	i;

	for (authbegin(); (i=authnext()) >= 0; )
	{
		int	rc;

		int (*f)(const char *, const char *, const char *,
			 const char *)=
			authstaticmodulelist[i]->auth_changepwd;

		if (!f)
			continue;

		rc= (*f)(service, userid, opwd, npwd);

		if (rc == 0)
		{
			writeauth(fd, "OK\n", 3);
			return;
		}
		if (errno == EPERM)
			break;
	}
	writeauth(fd, "FAIL\n", 5);
}

static void auth(int fd, char *p)
{
char *service;
char *authtype;
int	i;
char	*pp;

	service=p;
	if ((p=strchr(p, '\n')) == 0)	return;
	*p++=0;
	authtype=p;
	if ((p=strchr(p, '\n')) == 0)	return;
	*p++=0;

	pp=malloc(strlen(p)+1);
	if (!pp)
	{
		syslog(LOG_CRIT, "authdaemon: malloc() failed: %m");
		return;
	}

	for (authbegin(); (i=authnext()) >= 0; )
	{
	char	*q=(*authstaticmodulelist[i]->auth_func)(service, authtype,
			strcpy(pp, p), 1, &printauth_void, &fd);
		if (q)
		{
			free(q);
			free(pp);
			return;
		}
		if (errno != EPERM)
		{
			free(pp);
			return;	/* Temporary error */
		}
	}
	writeauth(fd, "FAIL\n", 5);
	free(pp);
}

static void idlefunc()
{
	int i;

	for (authbegin(); (i=authnext()) >= 0; )
		(*authstaticmodulelist[i]->auth_idle)();
}

static void doauth(int fd)
{
char	buf[BUFSIZ];
int	i, ch;
char	*p;

	readleft=0;
	for (i=0; (ch=getauthc(fd)) != '\n'; i++)
	{
		if (ch < 0 || i >= sizeof(buf)-2)
			return;
		buf[i]=ch;
	}
	buf[i]=0;

	for (p=buf; *p; p++)
	{
		if (*p == ' ')
		{
			*p++=0;
			while (*p == ' ')	++p;
			break;
		}
	}

	if (strcmp(buf, "PRE") == 0)
	{
		pre(fd, p);
		return;
	}

	if (strcmp(buf, "PASSWD") == 0)
	{
		passwd(fd, p);
		return;
	}

	if (strcmp(buf, "AUTH") == 0)
	{
	int j;

		i=atoi(p);
		if (i < 0 || i >= sizeof(buf))	return;
		for (j=0; j<i; j++)
		{
			ch=getauthc(fd);
			if (ch < 0)	return;
			buf[j]=ch;
		}
		buf[j]=0;
		auth(fd, buf);
	}
}

static int sighup_pipe= -1;

static RETSIGTYPE sighup(int n)
{
	if (sighup_pipe >= 0)
	{
		close(sighup_pipe);
		sighup_pipe= -1;
	}
	signal(SIGHUP, sighup);
#if	RETSIGTYPE != void
	return (1);
#endif
}

static int sigterm_received=0;

static RETSIGTYPE sigterm(int n)
{
	sigterm_received=1;
	if (sighup_pipe >= 0)
	{
		close(sighup_pipe);
		sighup_pipe= -1;
	}
	else
	{
		kill(0, SIGTERM);
		_exit(0);
	}

#if	RETSIGTYPE != void
	return (0);
#endif
}

static int startchildren(int *pipefd)
{
unsigned i;
pid_t	p;

	signal(SIGCHLD, sighup);
	for (i=0; i<ndaemons; i++)
	{
		p=fork();
		while (p == -1)
		{
			syslog(LOG_CRIT, "authdaemon: fork() failed: %m");
			sleep(5);
			p=fork();
		}
		if (p == 0)
		{
			sighup_pipe= -1;
			close(pipefd[1]);
			signal(SIGHUP, SIG_DFL);
			signal(SIGTERM, SIG_DFL);
			signal(SIGCHLD, SIG_DFL);
			return (1);
		}
	}
	return (0);
}

static int killchildren(int *pipefd)
{
int	waitstat;

	while (wait(&waitstat) >= 0 || errno != ECHILD)
		;

	if (pipe(pipefd))
	{
		syslog(LOG_CRIT, "authdaemon: pipe() failed: %m");
		return (-1);
	}

	return (0);
}

int start()
{
	int	s;
	int	lockfd;
	int	fd;
	int	pipefd[2];
	int	do_child;

	for (fd=3; fd<256; fd++)
		close(fd);

	if ((lockfd=ll_daemon_start(AUTHDAEMONLOCK)) < 0)
		return (1);

	if (pipe(pipefd))
	{
		perror("pipe");
		return (1);
	}

	if (readconfig())
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (1);
	}

	s=mksocket();
	if (s < 0)
	{
		perror(AUTHDAEMONSOCK);
		close(pipefd[0]);
		close(pipefd[1]);
		return (1);
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, sighup);
	signal(SIGTERM, sigterm);

	ll_daemon_started(AUTHDAEMONPID, lockfd);

	close(0);
	if (open("/dev/null", O_RDWR) != 0)
	{
		perror("open");
		exit(1);
	}
	close(1);
	close(2);
	dup(0);
	dup(1);
	sighup_pipe= pipefd[1];

	do_child=startchildren(pipefd);

	for (;;)
	{
		struct sockaddr	saddr;
		int	saddr_len;
		fd_set	fds;
		struct timeval tv;

		FD_ZERO(&fds);
		FD_SET(pipefd[0], &fds);

		if (do_child)
			FD_SET(s, &fds);

		tv.tv_sec=300;
		tv.tv_usec=0;

		if (select( (s > pipefd[0] ? s:pipefd[0])+1,
			&fds, 0, 0, &tv) <= 0)
			continue;
		if (FD_ISSET(pipefd[0], &fds))
		{
			close(pipefd[0]);
			if (do_child)
				return (0);	/* Parent died */
			readconfig();
			while (killchildren(pipefd))
				sleep(5);
			if (sigterm_received)
				return (0);
			sighup_pipe=pipefd[1];
			do_child=startchildren(pipefd);
			continue;
		}

		if (!FD_ISSET(s, &fds))
		{
			idlefunc();
			continue;
		}

		saddr_len=sizeof(saddr);
		if ((fd=accept(s, &saddr, &saddr_len)) < 0)
			continue;
		if (fcntl(fd, F_SETFL, 0) < 0)
		{
			syslog(LOG_CRIT, "authdaemon: fcntl() failed: %m");
		}
		else
			doauth(fd);
		close(fd);
	}
}

int main(int argc, char **argv)
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "start") == 0)
		{
			exit(start());
		}

		if (strcmp(argv[1], "stop") == 0)
		{
			exit(ll_daemon_stop(AUTHDAEMONLOCK, AUTHDAEMONPID));
		}

		if (strcmp(argv[1], "restart") == 0)
		{
			exit(ll_daemon_restart(AUTHDAEMONLOCK, AUTHDAEMONPID));
		}
	}

	fprintf(stderr, "Usage: %s start|stop|restart\n", argv[0]);
	exit (1);
	return (0);
}
