/*
** Copyright 2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/

/*
** $Id: rfc2045encode.c,v 1.1.1.1 2003/05/07 02:14:10 lfan Exp $
*/
#include	"rfc2045.h"


static int quoted_printable(struct rfc2045_encode_info *,
			    const char *, size_t);
static int base64(struct rfc2045_encode_info *,
		  const char *, size_t);
static int eflush(struct rfc2045_encode_info *,
		 const char *, size_t);

const char *rfc2045_encode_autodetect_fp(FILE *fp, int okQp)
{
	const char *encoding="7bit";
	int	l=0;
	int	longline=0;
	int c;

	while ((c=getc(fp)) != EOF)
	{
		unsigned char ch= (unsigned char)c;

		if (ch >= 0x80)
			encoding="8bit";

		if (ch == 0)
			return "base64";

		if (ch == '\n')	l=0;
		else if (++l > 990)
		{
			longline=1;
			if (!okQp)
				return "base64";
		}

	}

	if (longline && okQp)
		encoding="quoted-printable";

	return encoding;
}

void rfc2045_encode_start(struct rfc2045_encode_info *info,
			  const char *transfer_encoding,
			  int (*callback_func)(const char *, size_t, void *),
			  void *callback_arg)
{
	info->output_buf_cnt=0;
	info->input_buf_cnt=0;

	switch (*transfer_encoding) {
	case 'q':
	case 'Q':
		info->encoding_func=quoted_printable;
		break;
	case 'b':
	case 'B':
		info->encoding_func=base64;
		break;
	default:
		info->encoding_func=eflush;
		break;
	}
	info->callback_func=callback_func;
	info->callback_arg=callback_arg;
}

int rfc2045_encode(struct rfc2045_encode_info *info,
		   const char *ptr,
		   size_t cnt)
{
	return ((*info->encoding_func)(info, ptr, cnt));
}

int rfc2045_encode_end(struct rfc2045_encode_info *info)
{
	int rc=(*info->encoding_func)(info, NULL, 0);

	if (rc == 0 && info->output_buf_cnt > 0)
	{
		rc= (*info->callback_func)(info->output_buffer,
					   info->output_buf_cnt,
					   info->callback_arg);
		info->output_buf_cnt=0;
	}

	return rc;
}

static int eflush(struct rfc2045_encode_info *info, const char *ptr, size_t n)
{
	while (n > 0)
	{
		size_t i;

		if (info->output_buf_cnt == sizeof(info->output_buffer))
		{
			int rc= (*info->callback_func)(info->output_buffer,
						       info->output_buf_cnt,
						       info->callback_arg);

			info->output_buf_cnt=0;
			if (rc)
				return rc;
		}

		i=n;

		if (i > sizeof(info->output_buffer) - info->output_buf_cnt)
			i=sizeof(info->output_buffer) - info->output_buf_cnt;

		memcpy(info->output_buffer + info->output_buf_cnt, ptr, i);
		info->output_buf_cnt += i;
		ptr += i;
		n -= i;
	}
	return 0;
}

static int base64_flush(struct rfc2045_encode_info *);

static int base64(struct rfc2045_encode_info *info,
		  const char *buf, size_t n)
{
	if (!buf)
	{
		int rc=0;

		if (info->input_buf_cnt > 0)
			rc=base64_flush(info);

		return rc;
	}

	while (n)
	{
		size_t	i;

		if (info->input_buf_cnt == sizeof(info->input_buffer))
		{
			int rc=base64_flush(info);

			if (rc != 0)
				return rc;
		}

		i=n;
		if (i > sizeof(info->input_buffer) - info->input_buf_cnt)
			i=sizeof(info->input_buffer) - info->input_buf_cnt;

		memcpy(info->input_buffer + info->input_buf_cnt,
		       buf, i);
		info->input_buf_cnt += i;
		buf += i;
		n -= i;
	}
	return 0;
}

static const char base64tab[]=
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int base64_flush(struct rfc2045_encode_info *info)
{
	int	a=0,b=0,c=0;
	int	i, j;
	int	d, e, f, g;
	char	output_buf[ sizeof(info->input_buffer) / 3 * 4+1];

	for (j=i=0; i<info->input_buf_cnt; i += 3)
	{
		a=(unsigned char)info->input_buffer[i];
		b= i+1 < info->input_buf_cnt ?
			(unsigned char)info->input_buffer[i+1]:0;
		c= i+2 < info->input_buf_cnt ?
			(unsigned char)info->input_buffer[i+2]:0;

		d=base64tab[ a >> 2 ];
		e=base64tab[ ((a & 3 ) << 4) | (b >> 4)];
		f=base64tab[ ((b & 15) << 2) | (c >> 6)];
		g=base64tab[ c & 63 ];
		if (i + 1 >= info->input_buf_cnt)	f='=';
		if (i + 2 >= info->input_buf_cnt) g='=';
		output_buf[j++]=d;
		output_buf[j++]=e;
		output_buf[j++]=f;
		output_buf[j++]=g;
	}

	info->input_buf_cnt=0;

	output_buf[j++]='\n';
	return eflush(info, output_buf, j);
}

static const char xdigit[]="0123456789ABCDEF";

static int quoted_printable(struct rfc2045_encode_info *info,
			    const char *p, size_t n)
{
	char local_buf[256];
	int local_buf_cnt=0;

#define QPUT(c) do { if (local_buf_cnt == sizeof(local_buf)) \
                     { int rc=eflush(info, local_buf, local_buf_cnt); \
			local_buf_cnt=0; if (rc) return (rc); } \
			local_buf[local_buf_cnt]=(c); ++local_buf_cnt; } while(0)

	if (!p)
		return (0);

	while (n)
	{
		if (info->input_buf_cnt > 72 && *p != '\n')
		{
			QPUT('=');
			QPUT('\n');
			info->input_buf_cnt=0;
		}

		if ( *p == '\n')
			info->input_buf_cnt=0;
		else if (*p < ' ' || *p == '=' || *p >= 0x7F)
		{
			QPUT('=');
			QPUT(xdigit[ (*p >> 4) & 15]);
			QPUT(xdigit[ *p & 15 ]);
			info->input_buf_cnt += 3;
			p++;
			--n;
			continue;
		}
		else info->input_buf_cnt++;

		QPUT( *p);
		p++;
		--n;
	}

	if (local_buf_cnt > 0)
		return eflush(info, local_buf, local_buf_cnt);

	return 0;
}
