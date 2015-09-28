/*
** Copyright 2000-2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/

#include "rfc2045_config.h"
#include	"rfc2646.h"
#include	<stdlib.h>
#include	<string.h>

static const char rcsid[]="$Id: rfc2646create.c,v 1.2 2003/05/08 02:08:48 lfan Exp $";

struct rfc2646create *rfc2646create_alloc( int (*f)(const char *, size_t,
						    void *),
					   void *vp)
{
	struct rfc2646create *p=calloc(1, sizeof(struct rfc2646create));

	if (!p)
		return (NULL);

	p->handler=f;
	p->voidarg=vp;

	p->linesize=76;
	p->sent_firsttime=1;
	return (p);
}

static int wordwrap_line(const char *, size_t, size_t,
			  struct rfc2646create *);

static int wordwrap_sent(const char *buf, size_t cnt,
			 struct rfc2646create *rfcptr)
{
	size_t	i;
	int quote_depth=0;
	int rc;

	for (i=0; i<cnt; i++)
	{
		if (buf[i] != '>')
			break;
		++quote_depth;
	}

	if (i < cnt && buf[i] == ' ')
		++i;

	rc=0;

	/* A flowed line, followed by empty unflowed line, is a paragraph
	** break.
	*/
	if (rfcptr->has_sent_paragraph && i >= cnt &&
	    rfcptr->last_sent_quotelevel == quote_depth)
	{
		rc=(*rfcptr->handler)(" \n", 2, rfcptr->voidarg);
		rfcptr->has_sent_paragraph=0;
	}
	else
	{
		if (!rfcptr->sent_firsttime)
			rc=(*rfcptr->handler)("\n", 1, rfcptr->voidarg);
		rfcptr->has_sent_paragraph=1;
	}

	rfcptr->sent_firsttime=0;
	rfcptr->last_sent_quotelevel=quote_depth;

	if (rc)
		return (rc);

	if (quote_depth)	/* Already wrapped */
	{
		return ((*rfcptr->handler)(buf, cnt, rfcptr->voidarg));
	}

	while (cnt > i && buf[cnt-1] == ' ')
	{
		if (cnt - i == 3 && strncmp(buf+i, "-- ", 3) == 0)
			break;
		--cnt;
	}

	while (i < cnt)
	{
		size_t j;

		if (cnt - i <= rfcptr->linesize)
		{
			rc=wordwrap_line(buf, cnt, i, rfcptr);
			break;
		}

		for (j=i+rfcptr->linesize; j > i; --j)
			if (buf[j] == ' ')
			{
				++j;
				break;
			}

		if (j > i)
		{
			rc=wordwrap_line(buf, j-1, i, rfcptr);
			if (rc)
				break;

			i=j;
			while (i < cnt && buf[i] == ' ')
				++i;
			if (i == cnt)
				break;
			rc=(*rfcptr->handler)(" \n", 2, rfcptr->voidarg);
			if (rc)
				break;
			continue;
		}
		j=i+rfcptr->linesize;

                // by lfan
		{
			size_t k;
			for(k = j-1; k >= i; --k)
				if( buf[k] >= 0 )
					break;
			if( (j-1-k)%2 == 1 ) // half char!
				j++;
		}

		rc=wordwrap_line(buf, j, i, rfcptr);
		if (rc || (rc=(*rfcptr->handler)(" \n", 2, rfcptr->voidarg)))
			break;
		i=j;
	}
	return (rc);
}

static int wordwrap_line(const char *buf,
			 size_t cnt, size_t i,
			 struct rfc2646create *rfcptr)
{
	int rc=0;

	if ((cnt - i >= 5 && strncmp(buf+i, "From ", 5) == 0) ||
	    (cnt > i && buf[i] == '-'
	     && (cnt - i != 3 || strncmp(buf+i, "-- ", 3))))
		rc=(*rfcptr->handler)(" ", 1, rfcptr->voidarg);

	if (rc == 0)
		rc=(*rfcptr->handler)(buf+i, cnt-i, rfcptr->voidarg);
	return (rc);
}

int rfc2646create_parse(struct rfc2646create *rfcptr,
			const char *str, size_t strcnt)
{
	char *ptr, *q;
	size_t cnt;
	int rc;

	if (strcnt + rfcptr->buflen > rfcptr->bufsize)
	{
		size_t l=strcnt + rfcptr->buflen + 256;
		char *newbuf= rfcptr->buffer ? realloc(rfcptr->buffer, l)
			: malloc(l);

		if (!newbuf)
			return (-1);

		rfcptr->buffer=newbuf;
		rfcptr->bufsize=l;
	}

	if (strcnt)
		memcpy(rfcptr->buffer + rfcptr->buflen, str, strcnt);

	rfcptr->buflen += strcnt;

	ptr=rfcptr->buffer;
	cnt=rfcptr->buflen;

	rc=0;
	for (;;)
	{
		size_t i;

		for (i=0; i<cnt; i++)
			if (ptr[i] == '\n')
				break;
		if (i >= cnt)	break;
		rc=wordwrap_sent(ptr, i, rfcptr);
		if (rc)
			break;
		++i;
		ptr += i;
		cnt -= i;
	}
	q=rfcptr->buffer;
	rfcptr->buflen=cnt;
	while (cnt)
	{
		*q++ = *ptr++;
		--cnt;
	}
	return (rc);
}

int rfc2646create_free(struct rfc2646create *rfcptr)
{
	int rc=0;

	if (rfcptr->buflen)
		rc=wordwrap_sent(rfcptr->buffer, rfcptr->buflen, rfcptr);

	if (rc == 0)
		rc=(*rfcptr->handler)("\n", 1, rfcptr->voidarg);

	if (rfcptr->buffer)
		free(rfcptr->buffer);
	free(rfcptr);
	return (rc);
}
