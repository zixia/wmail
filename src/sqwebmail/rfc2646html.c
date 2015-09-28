/*
** Copyright 2000 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include	"config.h"
#include	"rfc2045/rfc2646.h"
#include	"rfc2646html.h"
#include	<stdlib.h>
#include	<string.h>

static const char rcsid[]="$Id: rfc2646html.c,v 1.1.1.1 2003/05/07 02:15:31 lfan Exp $";

struct rfc2646tohtml *rfc2646tohtml_alloc( int (*f)(const char *, int, void *),
					   void *a)
{
	struct rfc2646tohtml *p=(struct rfc2646tohtml *)
		calloc(1, sizeof(struct rfc2646tohtml));

	if (!p)
		return (0);

	p->handler=f;
	p->voidarg=a;
	p->prev_was_flowed=1;
	return (p);
}

static int endquote(struct rfc2646tohtml *);

int rfc2646tohtml_free(struct rfc2646tohtml *p)
{
	int rc=0;

	while ( p->current_quote_depth )
	{
		rc=endquote(p);
		if (rc)
			break;
	}
	free(p);
	return (rc);
}

static int endquote(struct rfc2646tohtml *p)
{
static const char str[]="</blockquote>\n";

	--p->current_quote_depth;
	p->prev_was_flowed=0;
	p->prev_was_0length=0;
	return ( (*p->handler)(str, sizeof(str)-1, p->voidarg));
}

int rfc2646tohtml_handler(struct rfc2646parser *p, int isflowed, void *vp)
{
	struct rfc2646tohtml *r=(struct rfc2646tohtml *)vp;
	int rc;

	const char *str;
	int i;
	unsigned colcnt;

	while (r->current_quote_depth > p->quote_depth)
	{
		if ((rc=endquote(r)) != 0)
			return (rc);
	}

	while (r->current_quote_depth < p->quote_depth)
	{
		static const char str[]="\n<blockquote type=cite>";

		rc=(*r->handler)(str, sizeof(str)-1, p->voidarg);

		if (rc)
			return (rc);
		++r->current_quote_depth;
		r->prev_was_flowed=1;	/* Prevent <br> below */
		r->prev_was_0length=0;
	}

	str=p->line;

	if (!r->prev_was_flowed)
	{
		if (r->prev_was_0length)
		{
			rc=(*r->handler)("<p>", 3, p->voidarg);
		}
		else
		{
			rc=(*r->handler)("<br>", 4, p->voidarg);
		}
	}
	r->prev_was_flowed=isflowed;
	r->prev_was_0length= *str == 0;
	colcnt=0;

	for (i=0; str[i]; )
	{
		switch (str[i]) {
		case '&':
			rc= i ? (*r->handler)(str, i, r->voidarg):0;
			if (rc == 0)
				rc=(*r->handler)("&amp;", 5,
						 r->voidarg);
			if (rc)
				return (rc);
			++i;
			str += i;
			colcnt += i;
			i=0;
			continue;
		case '<':
			rc= i ? (*r->handler)(str, i, r->voidarg):0;
			if (rc == 0)
				rc=(*r->handler)("&lt;", 4,
						 r->voidarg);
			if (rc)
				return (rc);
			++i;
			str += i;
			colcnt += i;
			i=0;
			continue;
		case '>':
			rc= i ? (*r->handler)(str, i, r->voidarg):0;
			if (rc == 0)
				rc=(*r->handler)("&gt;", 4,
						 r->voidarg);
			++i;
			str += i;
			colcnt += i;
			i=0;
			if (rc)
				return (rc);
			continue;
		case ' ':
			if (str[i+1] != ' ' && str[i+1] != '\t')
				break;
			rc= i ? (*r->handler)(str, i, r->voidarg):0;
			if (rc == 0)
				rc=(*r->handler)("&nbsp;", 6,
						 r->voidarg);
			++i;
			str += i;
			colcnt += i;
			i=0;
			if (rc)
				return (rc);
			continue;
		case '\t':
			rc= i ? (*r->handler)(str, i, r->voidarg):0;
			colcnt += i;

			do
			{
				if (rc == 0)
					rc=(*r->handler)("&nbsp;", 6,
							 r->voidarg);
				++colcnt;
			} while ( (colcnt % 8) != 0);

			++i;
			str += i;
			i=0;
			if (rc)
				return (rc);
			continue;
		default:
			break;
		}
		++i;
	}
	rc= i ? (*r->handler)(str, i, r->voidarg):0;
	if (rc == 0)
		rc=(*r->handler)("\n", 1, r->voidarg);
	return (rc);
}
