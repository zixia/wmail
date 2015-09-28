/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: mimegpgstack.c,v 1.1.1.1 2003/05/07 02:14:58 lfan Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "mimegpgstack.h"

void push_mimestack(struct mimestack **s, const char *b)
{
	struct mimestack *ss=(struct mimestack *)
		malloc(sizeof(**s));

	if (!ss)
	{
		perror("malloc");
		exit(1);
	}

	if ((ss->boundary=strdup(b)) == NULL)
	{
		free(ss);
		perror("strdup");
		exit(1);
	}

	ss->next= *s;
	*s=ss;
}

void pop_mimestack(struct mimestack **p)
{
	struct mimestack *pp= *p;

	if (pp)
	{
		*p=pp->next;
		free(pp->boundary);
		free(pp);
	}
}

void pop_mimestack_to(struct mimestack **p, struct mimestack *s)
{
	while (*p)
	{
		int last=strcmp( (*p)->boundary, s->boundary) == 0;
		pop_mimestack(p);
		if (last)
			break;
	}
}

struct mimestack *search_mimestack(struct mimestack *p, const char *c)
{
	int l=strlen(c);

	while (p)
	{
		int ll=strlen(p->boundary);

		if (l >= ll && strncasecmp(p->boundary, c, ll) == 0)
			break;
		p=p->next;
	}
	return (p);
}


