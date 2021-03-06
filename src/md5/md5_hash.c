/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include	"md5.h"
#include	<string.h>

static const char rcsid[]="$Id: md5_hash.c,v 1.1.1.1 2003/05/07 02:13:19 lfan Exp $";

static const char base64tab[]=
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const char *md5_hash_courier(const char *passw)
{
MD5_DIGEST md5buf;
static char hash_buffer[1+(sizeof(md5buf)+2)/3*4];
int	a=0,b=0,c=0;
int	i, j;
int	d, e, f, g;

	md5_digest(passw, strlen(passw), md5buf);

	j=0;

	for (i=0; i<sizeof(md5buf); i += 3)
	{
		a=md5buf[i];
		b= i+1 < sizeof(md5buf) ? md5buf[i+1]:0;
		c= i+2 < sizeof(md5buf) ? md5buf[i+2]:0;

		d=base64tab[ a >> 2 ];
		e=base64tab[ ((a & 3 ) << 4) | (b >> 4)];
		f=base64tab[ ((b & 15) << 2) | (c >> 6)];
		g=base64tab[ c & 63 ];
		if (i + 1 >= sizeof(md5buf))	f='=';
		if (i + 2 >= sizeof(md5buf)) g='=';
		hash_buffer[j++]=d;
		hash_buffer[j++]=e;
		hash_buffer[j++]=f;
		hash_buffer[j++]=g;
	}

	hash_buffer[j]=0;
	return (hash_buffer);
}
