/*
** Copyright 1998 - 2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/


#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>

#include	"rfc822.h"
#include	"rfc2047.h"

static const char rcsid[]="$Id: rfc2047.c,v 1.1.1.1 2003/05/07 02:14:05 lfan Exp $";

static const char xdigit[]="0123456789ABCDEF";

static char *rfc2047_search_quote(const char **ptr)
{
const char *p= *ptr;
char	*s;

	while (**ptr && **ptr != '?')
		++ *ptr;
	if ((s=malloc( *ptr - p + 1)) == 0)
		return (0);
	memcpy(s, p, *ptr-p);
	s[*ptr - p]=0;
	return (s);
}

static int nyb(int c)
{
const char	*p;

	c=toupper( (int)(unsigned char)c );
	p=strchr(xdigit, c);
	return (p ? p-xdigit:0);
}

static unsigned char decode64tab[256];
static int decode64tab_init=0;

static size_t decodebase64(char *ptr, size_t cnt)
{
size_t  i, j;
char    a,b,c;
size_t  k;

	if (!decode64tab_init)
	{
		for (i=0; i<256; i++)   decode64tab[i]=0;
		for (i=0; i<64; i++)
			decode64tab[ (int)
				("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i])]=i;
		decode64tab[ (int)'=' ] = 99;
	}

	i=cnt / 4;
	i=i*4;
	k=0;
	for (j=0; j<i; j += 4)
	{
	int     w=decode64tab[(int)(unsigned char)ptr[j]];
	int     x=decode64tab[(int)(unsigned char)ptr[j+1]];
	int     y=decode64tab[(int)(unsigned char)ptr[j+2]];
	int     z=decode64tab[(int)(unsigned char)ptr[j+3]];

		a= (w << 2) | (x >> 4);
		b= (x << 4) | (y >> 2);
		c= (y << 6) | z;
		ptr[k++]=a;
		if ( ptr[j+2] != '=')
			ptr[k++]=b;
		if ( ptr[j+3] != '=')
			ptr[k++]=c;
	}
	return (k);
}

/*
**	This is the main rfc2047 decoding function.  It receives rfc2047-encoded
**	text, and a callback function.  The callback function is repeatedly
**	called, each time receiving a piece of decoded text.  The decoded
**	info includes a text fragment - string, string length arg - followed
**	by the character set, followed by a context pointer that is received
**	from the caller.  If the callback function returns non-zero, rfc2047
**	decoding terminates, returning the result code.  Otherwise,
**	rfc2047_decode returns 0 after a successfull decoding (-1 if malloc
**	failed).
*/

int rfc2047_decode(const char *text, int (*func)(const char *, int,
						 const char *,
						 const char *, void *),
		void *arg)
{
int	rc;
int	had_last_word=0;
const char *p;
char	*chset, *lang;
char	*encoding;
char	*enctext;

	while (text && *text)
	{
		if (text[0] != '=' || text[1] != '?')
		{
			p=text;
			while (*text)
			{
				if (text[0] == '=' && text[1] == '?')
					break;
				if (!isspace((int)(unsigned char)*text))
					had_last_word=0;
				++text;
			}
			if (text > p && !had_last_word)
			{
				rc=(*func)(p, text-p, 0, 0, arg);
				if (rc)	return (rc);
			}
			continue;
		}

		text += 2;
		if ((chset=rfc2047_search_quote( &text )) == 0)
			return (-1);
		if (*text)	++text;
		if ((encoding=rfc2047_search_quote( &text )) == 0)
		{
			free(chset);
			return (-1);
		}
		if (*text)	++text;
		if ((enctext=rfc2047_search_quote( &text )) == 0)
		{
			free(encoding);
			free(chset);
			return (-1);
		}
		if (*text == '?' && text[1] == '=')
			text += 2;
		if (strcmp(encoding, "Q") == 0 || strcmp(encoding, "q") == 0)
		{
		char *q, *r;

			for (q=r=enctext; *q; )
			{
				int c;

				if (*q == '=' && q[1] && q[2])
				{
					*r++ = (char)(
						nyb(q[1])*16+nyb(q[2]));
					q += 3;
					continue;
				}

				c=*q++;
				if (c == '_')
					c=' ';
				*r++ = c ;
			}
			*r=0;
		}
		else if (strcmp(encoding, "B") == 0 || strcmp(encoding, "b")==0)
		{
			enctext[decodebase64(enctext, strlen(enctext))]=0;
		}

		lang=strrchr(chset, '*'); /* RFC 2231 language */

		if (lang)
			*lang++=0;

		rc=(*func)(enctext, strlen(enctext), chset, lang, arg);
		free(enctext);
		free(chset);
		free(encoding);
		if (rc)	return (rc);

		had_last_word=1;	/* Ignore blanks between enc words */
	}
	return (0);
}

/*
** rfc2047_decode_simple just strips out the rfc2047 decoding, throwing away
** the character set.  This is done by calling rfc2047_decode twice, once
** to count the number of characters in the decoded text, the second time to
** actually do it.
*/

struct simple_info {
	char *string;
	int index;
	const char *mychset;
	} ;

static int count_simple(const char *txt, int len, const char *chset,
			const char *lang, void *arg)
{
struct simple_info *iarg= (struct simple_info *)arg;

	iarg->index += len;

	return (0);
}

static int save_simple(const char *txt, int len, const char *chset,
		       const char *lang,
		       void *arg)
{
struct simple_info *iarg= (struct simple_info *)arg;

	memcpy(iarg->string+iarg->index, txt, len);
	iarg->index += len;
	return (0);
}

char *rfc2047_decode_simple(const char *text)
{
struct	simple_info info;

	info.index=1;
	if (rfc2047_decode(text, &count_simple, &info))
		return (0);

	if ((info.string=malloc(info.index)) == 0)	return (0);
	info.index=0;
	if (rfc2047_decode(text, &save_simple, &info))
	{
		free(info.string);
		return (0);
	}
	info.string[info.index]=0;
	return (info.string);
}

/*
** rfc2047_decode_enhanced is like simply, but prefixes the character set
** name before the text, in brackets.
*/

static int do_enhanced(const char *txt, int len, const char *chset,
		       const char *lang,
		       void *arg,
		       int (*func)(const char *, int, const char *,
				   const char *, void *)
		       )
{
int	rc=0;
struct	simple_info *info=(struct simple_info *)arg;

	if (chset && info->mychset && strcasecmp(chset, info->mychset) == 0)
		chset=0;

	if (chset)
	{
		rc= (*func)(" [", 2, 0, 0, arg);
		if (rc == 0)
			rc= (*func)(chset, strlen(chset), 0, 0, arg);
		if (rc == 0 && lang)
			rc= (*func)("*", 1, 0, 0, arg);
		if (rc == 0 && lang)
			rc= (*func)(lang, strlen(lang), 0, 0, arg);
		if (rc == 0)
			rc= (*func)("] ", 2, 0, 0, arg);
	}

	if (rc == 0)
		rc= (*func)(txt, len, 0, 0, arg);
	return (rc);
}

static int count_enhanced(const char *txt, int len, const char *chset,
			  const char *lang,
			  void *arg)
{
	return (do_enhanced(txt, len, chset, lang, arg, &count_simple));
}

static int save_enhanced(const char *txt, int len, const char *chset,
			 const char *lang,
			 void *arg)
{
	return (do_enhanced(txt, len, chset, lang, arg, &save_simple));
}

char *rfc2047_decode_enhanced(const char *text, const char *mychset)
{
struct	simple_info info;

	info.mychset=mychset;
	info.index=1;
	if (rfc2047_decode(text, &count_enhanced, &info))
		return (0);

	if ((info.string=malloc(info.index)) == 0)	return (0);
	info.index=0;
	if (rfc2047_decode(text, &save_enhanced, &info))
	{
		free(info.string);
		return (0);
	}
	info.string[info.index]=0;
	return (info.string);
}

void rfc2047_print(const struct rfc822a *a,
	const char *charset,
	void (*print_func)(char, void *),
	void (*print_separator)(const char *, void *), void *ptr)
{
	rfc822_print_common(a, &rfc2047_decode_enhanced, charset,
		print_func, print_separator, ptr);
}

static char *a_rfc2047_encode_str(const char *str, const char *charset);

static void rfc2047_encode_header_do(const struct rfc822a *a,
	const char *charset,
	void (*print_func)(char, void *),
	void (*print_separator)(const char *, void *), void *ptr)
{
	rfc822_print_common(a, &a_rfc2047_encode_str, charset,
		print_func, print_separator, ptr);
}

/*
** When MIMEifying names from an RFC822 list of addresses, strip quotes
** before MIMEifying them, and add them afterwards.
*/

static char *a_rfc2047_encode_str(const char *str, const char *charset)
{
	size_t	l=strlen(str);
	char	*p, *s;
	char foo, bar[2];

	if (l == 0)
		return (strdup(str));

	if (*str == '"' && str[l-1] == '"')
		;
	else if (*str == '(' && str[l-1] == ')')
		;
	else
		return (rfc2047_encode_str(str, charset));

	foo= *str;
	bar[0]= str[l-1];
	bar[1]=0;

	p=malloc(l);
	if (!p)	return (0);
	memcpy(p, str+1, l-2);
	p[l-2]=0;
	s=rfc2047_encode_str(p, charset);
	free(p);
	if (!s)	return (0);
	p=malloc(strlen(s)+3);
	if (!p)
	{
		free(s);
		return (0);
	}
	p[0]=foo;
	strcpy(p+1, s);
	strcat(p, bar);
	free(s);
	return (p);
}




static void count(char c, void *p);
static void counts2(const char *c, void *p);
static void save(char c, void *p);
static void saves2(const char *c, void *p);

char *rfc2047_encode_header(const struct rfc822a *a,
        const char *charset)
{
size_t	l;
char	*s, *p;

	l=1;
	rfc2047_encode_header_do(a, charset, &count, &counts2, &l);
	if ((s=malloc(l)) == 0)	return (0);
	p=s;
	rfc2047_encode_header_do(a, charset, &save, &saves2, &p);
	*p=0;
	return (s);
}

static void count(char c, void *p)
{
	++*(size_t *)p;
}

static void counts2(const char *c, void *p)
{
	if (strcmp(c, ", ") == 0)
		c=",\n  ";

	while (*c)	count(*c++, p);
}

static void save(char c, void *p)
{
	**(char **)p=c;
	++*(char **)p;
}

static void saves2(const char *c, void *p)
{
	if (strcmp(c, ", ") == 0)
		c=",\n  ";

	while (*c)	save(*c++, p);
}

int rfc2047_encode_callback(const char *str, const char *charset,
	int (*func)(const char *, size_t, void *), void *arg)
{
int	rc;

#define DOENCODE(i) ((str[i] & 0x80) || str[i] == '"' || str[i] == '=')
#define ISSPACE(i) ( ((i) & 0x80) == 0 && isspace(i))

	while (*str)
	{
	size_t	i, c;

		for (i=0; str[i]; i++)
			if (DOENCODE(i))
				break;
		if (str[i] == 0)
			return ( i ? (*func)(str, i, arg):0);

		/* Find start of word */

		while (i)
		{
			--i;
			if (ISSPACE((int)(unsigned char)str[i]))
			{
				++i;
				break;
			}
		}
		if (i)
		{
			rc= (*func)(str, i, arg);
			if (rc)	return (rc);
			str += i;
		}

		/*
		** Figure out when to stop MIME decoding.  Consecutive
		** MIME-encoded words are MIME-encoded together.
		*/

		i=0;

		for (;;)
		{
			for ( ; str[i]; i++)
				if (ISSPACE((int)(unsigned char)str[i]))
					break;
			if (str[i] == 0)
				break;

			for (c=i; str[c] && ISSPACE((int)(unsigned char)str[c]);
				++c)
				;
			
			for (; str[c]; c++)
				if (ISSPACE((int)(unsigned char)str[c]) ||
				    DOENCODE(c))
					break;

			if (str[c] == 0 || ISSPACE((int)(unsigned char)str[c]))
				break;
			i=c;
		}

		/* Output mimeified text, insert spaces at 70+ character
		** boundaries for line wrapping.
		*/

		c=0;
		while (i)
		{
			if (c == 0)
			{
				if ( (rc=(*func)("=?", 2, arg)) != 0 ||
					(rc=(*func)(charset, strlen(charset),
						arg)) != 0 ||
					(rc=(*func)("?Q?", 3, arg)) != 0)
					return (rc);
				c += strlen(charset)+5;
			}

			if ((*str & 0x80) || *str == '"' || *str == ' ' ||
			    *str == '_' || *str == '=' || *str == '?')
			{
				char	buf[3];

				buf[0]='=';
				buf[1]=xdigit[ ( *str >> 4) & 0x0F ];
				buf[2]=xdigit[ *str & 0x0F ];

				if ( (rc=*str == ' ' ? (*func)("_", 1, arg)
				      : (*func)(buf, 3, arg)) != 0)
					return (rc);
				c += *str == ' ' ? 1:3;
				++str;
				--i;
			}
			else
			{
			size_t	j;

				for (j=0; j < i && !(str[j] & 0x80) &&
					     str[j] != ' ' &&
					     str[j] != '_' &&
					     str[j] != '?' &&
					     str[j] != '=' &&
					     str[j] != '"'; j++)
					if (j + c >= 50)
						break;
				if ( (rc=(*func)(str, j, arg)) != 0)
					return (rc);
				c += j;
				str += j;
				i -= j;
			}

			if (i == 0 || c >= 50)
			{
				if ( (rc=(*func)("?= ", i ? 3:2, arg)) != 0)
					return (rc);
				
				c=0;
			}
		}
	}
	return (0);
}

static int count_char(const char *c, size_t l, void *p)
{
size_t *i=(size_t *)p;

	*i += l;
	return (0);
}

static int save_char(const char *c, size_t l, void *p)
{
char **s=(char **)p;

	memcpy(*s, c, l);
	*s += l;
	return (0);
}

char *rfc2047_encode_str(const char *str, const char *charset)
{
size_t	i=1;
char	*s, *p;

	(void)rfc2047_encode_callback(str, charset, &count_char, &i);
	if ((s=malloc(i)) == 0)	return (0);
	p=s;
	(void)rfc2047_encode_callback(str, charset, &save_char, &p);
	*p=0;
	return (s);
}
