#ifndef pcpdtimer_h
#define pcpdtimer_h

/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char pcpdtimer_h_rcsid[]="$Id: pcpdtimer.h,v 1.1.1.1 2003/05/07 02:15:10 lfan Exp $";

#include "config.h"
#include <time.h>

struct PCP;

struct pcpdtimer {
	struct pcpdtimer *next, *prev;

	time_t alarm;
	void (*handler)(struct PCP *, void *);
	void *voidarg;
} ;

extern struct pcpdtimer *first_timer, *last_timer;

#define pcpdtimer_init(p) memset((p), 0, sizeof(*p))

void pcpdtimer_install(struct pcpdtimer *, time_t);
void pcpdtimer_triggered(struct pcpdtimer *);

#endif
