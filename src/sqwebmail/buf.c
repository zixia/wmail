/*
** Copyright 1998 - 1999 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: buf.c,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/
#include	<string.h>
#include	"buf.h"
#include	"sqwebmail.h"

void	buf_allocbuf(struct buf *b, size_t n)
{
	if (n > b->size)
	{
	size_t	c=n+64;
	char	*p= b->ptr ? realloc(b->ptr, c):malloc(c);

		if (!p)	enomem();
		b->ptr=p;
		b->size=c;
	}
}

void	buf_cpy(struct buf *b, const char *c)
{
size_t	l=strlen(c);

	buf_allocbuf(b, l+1);
	strcpy(b->ptr, c);
	b->cnt=l;
}

void	buf_cpyn(struct buf *b, const char *c, size_t n)
{
size_t	l;

	for (l=0; l<n; l++)
		if (c[l] == '\0')	break;

	buf_allocbuf(b, l+1);
	memcpy(b->ptr, c, l);
	b->ptr[b->cnt=l]=0;
}

void	buf_cat(struct buf *b, const char *c)
{
size_t	l=strlen(c);

	buf_allocbuf(b, b->cnt+l+1);
	strcpy(b->ptr+b->cnt, c);
	b->cnt += l;
}

void	buf_catn(struct buf *b, const char *c, size_t n)
{
size_t	l;

	for (l=0; l<n; l++)
		if (c[l] == '\0')	break;

	buf_allocbuf(b, b->cnt+l+1);
	memcpy(b->ptr+b->cnt, c, l);
	b->ptr[b->cnt += l]=0;
}

void	buf_memcpy(struct buf *b, const char *c, size_t n)
{
	buf_allocbuf(b, n+1);
	memcpy(b->ptr, c, n);
	b->ptr[b->cnt=n]=0;
}

void	buf_memcat(struct buf *b, const char *c, size_t n)
{
	buf_allocbuf(b, b->cnt+n+1);
	memcpy(b->ptr+b->cnt, c, n);
	b->ptr[b->cnt += n]=0;
}

void	buf_trimleft(struct buf *b, size_t n)
{
	if (n >= b->cnt)
		b->cnt=0;
	else
	{
	size_t	i;

		for (b->cnt -= n, i=0; i<b->cnt; i++)
			b->ptr[i]=b->ptr[i+n];
	}
	buf_allocbuf(b, b->cnt+1);
	b->ptr[b->cnt]=0;
}
