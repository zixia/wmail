/*
** Copyright 2002 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include "config.h"
#include "maildirwatch.h"

#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const char rcsid[]="$Id: maildirwatch.c,v 1.1.1.1 2003/05/07 02:14:20 lfan Exp $";

struct maildirwatch *maildirwatch_alloc(const char *maildir)
{
	char wd[PATH_MAX];
	struct maildirwatch *w;

	if (maildir == 0 || *maildir == 0)
		maildir=".";

	if (getcwd(wd, sizeof(wd)-1) < 0)
		return NULL;

	if (*maildir == '/')
		wd[0]=0;
	else
		strcat(wd, "/");

	if ((w=malloc(sizeof(struct maildirwatch))) == NULL)
		return NULL;

	if ((w->maildir=malloc(strlen(wd)+strlen(maildir)+1)) == NULL)
	{
		free(w);
		return NULL;
	}

	strcat(strcpy(w->maildir, wd), maildir);

#if HAVE_FAM

	if (FAMOpen(&w->fc) < 0)
	{
		errno=EIO;

		free(w->maildir);
		free(w);
		return NULL;
	}
	w->broken=0;
#endif
	return w;
}

void maildirwatch_free(struct maildirwatch *w)
{
#if HAVE_FAM
	FAMClose(&w->fc);
#endif

	free(w->maildir);
	free(w);
}

#if HAVE_FAM

static int waitEvent(struct maildirwatch *w)
{
	int fd;
	fd_set r;
	struct timeval tv;
	time_t now2;

	int rc;

	while ((rc=FAMPending(&w->fc)) == 0)
	{
		if (w->now >= w->timeout)
			return 0;

		fd=FAMCONNECTION_GETFD(&w->fc);

		FD_ZERO(&r);
		FD_SET(fd, &r);

		tv.tv_sec= w->timeout - w->now;
		tv.tv_usec=0;

		select(fd+1, &r, NULL, NULL, &tv);
		now2=time(NULL);

		if (now2 < w->now)
			return 0; /* System clock changed */

		w->now=now2;
	}

	return rc;
}
#endif


int maildirwatch_unlock(struct maildirwatch *w, int nseconds)
{
#if HAVE_FAM
	FAMRequest fr;
	FAMEvent fe;
	int cancelled=0;
	char *p;

	if (w->broken)
	{
		errno=EIO;
		return -1;
	}

	p=malloc(strlen(w->maildir)+ sizeof("/" WATCHDOTLOCK));

	if (!p)
		return -1;

	strcat(strcpy(p, w->maildir), "/" WATCHDOTLOCK);

	if (FAMMonitorFile(&w->fc, p, &fr, NULL) < 0)
	{
		free(p);
		fprintf(stderr, "ERR:FAMMonitorFile: %s\n",
			strerror(errno));
		return -1;
	}
	free(p);

	if (nseconds < 0)
		nseconds=0;

	time(&w->now);

	w->timeout=w->now + nseconds;

	for (;;)
	{
		if (waitEvent(w) != 1)
		{
			if (!cancelled && FAMCancelMonitor(&w->fc, &fr) == 0)
			{
				w->timeout=w->now+15;
				cancelled=1;
				continue;
			}

			if (!cancelled)
				fprintf(stderr, "ERR:FAMCancelMonitor: %s\n",
					strerror(errno));

			w->broken=1;
			break;
		}

		if (FAMNextEvent(&w->fc, &fe) != 1)
		{
			fprintf(stderr, "ERR:FAMNextEvent: %s\n",
				strerror(errno));
			w->broken=1;
			break;
		}

		if (fe.fr.reqnum != fr.reqnum)
			continue;

		if (fe.code == FAMDeleted && !cancelled)
		{
			if (FAMCancelMonitor(&w->fc, &fr) == 0)
			{
				w->timeout=w->now+15;
				cancelled=1;
				continue;
			}
			fprintf(stderr, "ERR:FAMCancelMonitor: %s\n",
				strerror(errno));
			w->broken=1;
			break;
		}

		if (fe.code == FAMAcknowledge)
			break;
	}

	if (w->broken)
		return -1;

	return 0;
#else
	return -1;
#endif
}

int maildirwatch_start(struct maildirwatch *p,
		       struct maildirwatch_contents *w)
{
	w->w=p;

	time(&p->now);
	p->timeout = p->now + 60;

#if HAVE_FAM

	if (p->broken)
	{
		errno=EIO;
		return (1);
	}

	{
		char *s=malloc(strlen(p->maildir)+sizeof("/cur"));

		if (!s)
			return (-1);

		strcat(strcpy(s, p->maildir), "/new");

		w->endexists_received=0;
		w->ack_received=0;
		w->cancelled=0;

		if (FAMMonitorDirectory(&p->fc, s, &w->new_req, NULL) < 0)
		{
			fprintf(stderr, "ERR:"
				"FAMMonitorDirectory(%s) failed: %s\n",
				s, strerror(errno));
			free(s);
			errno=EIO;
			return (-1);
		}

		strcat(strcpy(s, p->maildir), "/cur");
		if (FAMMonitorDirectory(&p->fc, s, &w->cur_req, NULL) < 0)
		{
			fprintf(stderr, "ERR:"
				"FAMMonitorDirectory(%s) failed: %s\n",
				s, strerror(errno));

			if (FAMCancelMonitor(&w->w->fc, &w->new_req) < 0)
			{
				free(s);
				p->broken=1;
				fprintf(stderr, "ERR:FAMCancelMonitor: %s\n",
					strerror(errno));
				errno=EIO;
				return (-1);
			}
			w->cancelled=1;
			w->ack_received=1;
		}
		free(s);
	}
	return 0;
#else
	return 1;
#endif
}

int maildirwatch_started(struct maildirwatch_contents *w,
			 int *fdret)
{
#if HAVE_FAM

	if (w->w->broken)
		return (1);

	*fdret=FAMCONNECTION_GETFD(&w->w->fc);

	while (FAMPending(&w->w->fc))
	{
		FAMEvent fe;

		if (FAMNextEvent(&w->w->fc, &fe) != 1)
		{
			fprintf(stderr, "ERR:FAMNextEvent: %s\n",
				strerror(errno));
			w->w->broken=1;
			return (-1);
		}

		switch (fe.code) {
		case FAMDeleted:
			if (!w->cancelled)
			{
				w->cancelled=1;
				if (FAMCancelMonitor(&w->w->fc,
						     &w->new_req) ||
				    FAMCancelMonitor(&w->w->fc,
						     &w->cur_req))
				{
					w->w->broken=1;
					fprintf(stderr,
						"ERR:FAMCancelMonitor: %s\n",
						strerror(errno));
					return (-1);
				}
			}
			break;
		case FAMAcknowledge:
			if (++w->ack_received >= 2)
				return -1;
			break;
		case FAMEndExist:
			++w->endexists_received;
			break;
		default:
			break;
		}
	}

	return (w->endexists_received >= 2 && w->ack_received == 0);
#else
	*fdret= -1;

	return 1;
#endif
}

int maildirwatch_check(struct maildirwatch_contents *w,
		       int *changed,
		       int *fdret,
		       int *timeout)
{
	time_t curTime;

	*changed=0;
	*fdret=-1;

	curTime=time(NULL);

	if (curTime < w->w->now)
		w->w->timeout=curTime; /* System clock changed */
	w->w->now=curTime;

#if HAVE_FAM

	if (!w->w->broken)
	{
		*fdret=FAMCONNECTION_GETFD(&w->w->fc);

		while (FAMPending(&w->w->fc))
		{
			FAMEvent fe;

			if (FAMNextEvent(&w->w->fc, &fe) != 1)
			{
				fprintf(stderr, "ERR:FAMNextEvent: %s\n",
					strerror(errno));
				w->w->broken=1;
				return (-1);
			}

			switch (fe.code) {
			case FAMDeleted:
			case FAMCreated:
			case FAMMoved:
				if (!w->cancelled)
				{
					w->cancelled=1;
					if (FAMCancelMonitor(&w->w->fc,
							     &w->new_req) ||
					    FAMCancelMonitor(&w->w->fc,
							     &w->cur_req))
					{
						w->w->broken=1;
						fprintf(stderr,
							"ERR:FAMCancelMonitor:"
							" %s\n",
							strerror(errno));
						return (-1);
					}
				}
				break;
			case FAMAcknowledge:
				++w->ack_received;
			default:
				break;
			}
		}

		*changed=w->ack_received >= 2;
		*timeout=60 * 60;
		return 0;
	}
#endif
	*timeout=60;

 	if ( (*changed= w->w->now >= w->w->timeout) != 0)
		w->w->timeout = w->w->now + 60;
	return 0;
}

void maildirwatch_end(struct maildirwatch_contents *w)
{
#if HAVE_FAM

	if (!w->w->broken)
	{
		if (!w->cancelled)
		{
			w->cancelled=1;
			if (FAMCancelMonitor(&w->w->fc,
					     &w->new_req) ||
			    FAMCancelMonitor(&w->w->fc,
					     &w->cur_req))
			{
				w->w->broken=1;
				fprintf(stderr,
					"ERR:FAMCancelMonitor: %s\n",
					strerror(errno));
			}
		}
	}

	while (!w->w->broken && w->ack_received < 2)
	{
		FAMEvent fe;

		time(&w->w->now);
		w->w->timeout=w->w->now + 15;

		if (waitEvent(w->w) != 1)
		{
			fprintf(stderr, "ERR:FAMPending: timeout\n");
			w->w->broken=1;
			break;
		}

		if (FAMNextEvent(&w->w->fc, &fe) != 1)
		{
			fprintf(stderr, "ERR:FAMNextEvent: %s\n",
				strerror(errno));
			w->w->broken=1;
			break;
		}

		if (fe.code == FAMAcknowledge)
			++w->ack_received;
	}
#endif
}
