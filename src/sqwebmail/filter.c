/*
** Copyright 1998 - 2000 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: filter.c,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/
#include	"filter.h"
#include	"buf.h"
#include	"sqwebmail.h"
#include	<string.h>

static void (*filter_func)(const char *, size_t);

/*static void filter_for_saving(const char *, size_t);*/
static void filter_for_display(const char *, size_t);

static void (*handler_func)(const char *, size_t);

static struct buf disp_buf;
static int linesize;

void	filter_start(int mode, void (*handler)(const char *, size_t))
{
	buf_init(&disp_buf);
	switch (mode)	{
	case FILTER_FOR_SAVING:
		filter_func=handler;	/* No-op, for now. */
		break;
	case FILTER_FOR_DISPLAY:
		filter_func= &filter_for_display;
		linesize=EXTLINESIZE;
		break;
	case FILTER_FOR_PREVIEW:
		filter_func= &filter_for_display;
		linesize=MYLINESIZE;
		break;
	}

	handler_func=handler;
}
	
void	filter(const char *ptr, size_t cnt)
{
	(*filter_func)(ptr, cnt);
}

void	filter_end(void)
{
	(*filter_func)(0, 0);
	buf_free(&disp_buf);
}

static void wordwrap(const char *buf, size_t cnt,
			void (*func)(const char *, size_t))
{
size_t	i;

	if (cnt == 0 || *buf == '>')	/* Do not wordwrap quoted lines */
	{
		(*func)(buf, cnt);
		(*func)("\n", 1);
		return;
	}
	for (i=0; i<cnt;)
	{
	size_t	j;
	size_t	spacepos, restartpos;

		spacepos=restartpos=cnt;
		for (j=i; ; j++)
		{
			if (j >= i+linesize && spacepos <= j)
				break;
			if (j >= cnt)
			{
				spacepos=restartpos=j;
				break;
			}
			if (buf[j] == ' ')
			{
				spacepos=j;
				restartpos=j+1;
			}
		}
		(*func)(buf+i, spacepos-i);
		(*func)("\n", 1);
		i=restartpos;
	}
}

/***************************************************************************/

static void dodisplay(const char *, size_t);

static void filter_for_display(const char *ptr, size_t cnt)
{
size_t	i;

	if (!ptr)
	{
		if (disp_buf.cnt)
			wordwrap(disp_buf.ptr, disp_buf.cnt, dodisplay);
		disp_buf.cnt=0;
		return;
	}

	buf_memcat(&disp_buf, ptr, cnt);
	for (;;)
	{
		for (i=0; i<disp_buf.cnt; i++)
			if (disp_buf.ptr[i] == '\n')
				break;
		if (i >= disp_buf.cnt)	break;
		wordwrap(disp_buf.ptr, i, dodisplay);
		buf_trimleft(&disp_buf, i+1);
	}
}

static void dodisplay(const char *buf, size_t cnt)
{
size_t	i,j;
const char *p=0;
char	charbuf[6];

	for (i=j=0; i<cnt; i++)
	{
		switch (buf[i])	{
		case '<':
			p="&lt;";	break;
		case '>':
			p="&gt;";	break;
		case '&':
			p="&amp;";	break;
		case '"':
			p="&quot;";	break;
		default:
			if (ISCTRL(buf[i]) && buf[i] != '\n')
			{
				sprintf(charbuf, "&#%d;",
					(int)(unsigned char)buf[i]);
				p=charbuf;
			}
			else
				continue;
		}
		if (i > j)
			(*handler_func)(buf+j, i-j);
		(*handler_func)(p, strlen(p));
		j=i+1;
	}
	if (i > j)
		(*handler_func)(buf+j, i-j);
}
