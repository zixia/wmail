/*
** Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: html.c,v 1.1.1.1 2003/05/07 02:15:27 lfan Exp $
*/
#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>
#include	"config.h"
#include	"cgi/cgi.h"
#include	"sqwebmail.h"
#include	"rfc2045/rfc2045.h"

void decodehtmlchar(char *p)
{
char	*q;

	for (q=p; *p; )
	{
	int	i;

		if (*p != '&')
		{
			*q++=*p++;
			continue;
		}

		if ( p[1] == '#')
		{
		unsigned c=0;

			for (p += 2; isdigit((int)(unsigned char)*p); p++)
				c=c * 10 + (*p++ - '0');
			c=(unsigned char)c;
			if (c)	*q++=c;
			if (*p == ';')	p++;
		}

		for (i=1; p[i]; i++)
			if (!isalpha((int)(unsigned char)p[i]))	break;

		if (p[i] != ';')
		{
			*q++=*p++;
			continue;
		}

		for (i=0; p[i] != ';'; i++)
			p[i]=tolower(p[i]);
		++i;
		if (strncmp(p, "&lt;", 4) == 0)
		{
			*q++ = '<';
		}
		else if ( strncmp(p, "&gt;",4) == 0)
		{
			*q++ = '>';
		}
		else if ( strncmp(p, "&amp;", 5) == 0)
		{
			*q++ = '&';
		}
		else if ( strncmp(p, "&quot;", 6) == 0)
		{
			*q++ = '"';
		}
		p += i;
	}
	*q=0;
}

/*
	HTML sanitization filter.  Transforms HTML as follows:

	The following tags are dropped:

		<SCRIPT>, </SCRIPT>, <APP>, </APP>, <APPLET>, </APPLET>, 
		<SERVER>, </SERVER>, <OBJECT>, </OBJECT>, <HTML>, </HTML>, 
		<HEAD>, </HEAD>, <BODY>, </BODY>, <META>, <TITLE>, </TITLE>,
		<FRAME>, </FRAME>, <LINK> <IFRAME> and </IFRAME>.

	The ONLOAD, ONMOUSEOVER, and all other ON* attributes are removed.

	Attributes TARGET, CODE, ACTION, CODETYPE and LANGUAGE are removed.
	TARGET=_blank is added to all <A> tags.

	HREF, SRC, or LOWSRC attributes that do not specify a URL of http:,
	https:, ftp:, gopher:, wais:, or telnet:, are removed.

*/

char *tagbuf=0;
size_t tagbufsize=0, tagbuflen;

struct tagattrinfo {
	char *tagname;
	size_t tagnamelen;
	char *tagvalue;
	size_t tagvaluelen;

	char *atagstart;	/* Entire tag=value location */
	size_t ataglen;
	} ;

struct tagattrinfo *tagattr=0;
size_t tagattrsize=0, tagattrlen;

static void addtagbuf(int c)
{
	if (tagbufsize >= 1024)	return;	/* DOS attack - get rid of the tag */

	if (tagbuflen >= tagbufsize)
	{
	char	*newtagbuf= tagbuf ? (char *)realloc(tagbuf, tagbufsize+256)
			:(char *)malloc(tagbufsize+256);

		if (!newtagbuf)	enomem();
		tagbuf=newtagbuf;
		tagbufsize += 256;
	}
	tagbuf[tagbuflen++]=c;
}

/* Parse the contents of tagbuf into individual attributes.  If argument is
** NULL, just the count of attributes is returned.  That's the first pass.
** On the second pass the argument points to a struct tagattrinfo array which
** we initialize.
**
** The first attribute is -- obviously -- the actual tag.
*/

static size_t parseattr(struct tagattrinfo *tai)
{
size_t	c=0;
char *p;

	for (p=tagbuf; *p; )
	{
		while (*p && isspace((int)(unsigned char)*p))	p++;
		if (!*p)	break;

		++c;
		if (tai)
		{
			tai->tagname=p;
			tai->tagnamelen=0;
			tai->atagstart=p;
		}
		while (*p && !isspace((int)(unsigned char)*p) && *p != '=')
		{
			++p;
			if (tai)	++tai->tagnamelen;
		}
		if (*p != '=')	/* No attribute value */
		{
			if (tai)
			{
				tai->tagvalue=0;
				tai->tagvaluelen=0;
			}
		}
		else
		{
			char c;

			++p;
			if ((c=*p) == '"' || c == '\'')
				/* Attr value in quotes */
			{
				++p;
				if (tai)
				{
					tai->tagvalue=p;
					tai->tagvaluelen=0;
				}
				while (*p && *p != (char)c)
				{
					++p;
					if (tai)	++tai->tagvaluelen;
				}
				if (*p)	p++;
			}
			else
			{
				if (tai)
				{
					tai->tagvalue=p;
					tai->tagvaluelen=0;
				}
				while (*p && !isspace((int)(unsigned char)*p))
				{
					p++;
					if (tai)
					{
						tai->tagvalue=p;
						tai->tagvaluelen=0;
					}
				}
			}
		}
		if (tai)
		{
			tai->ataglen=p-tai->atagstart;
			++tai;
		}
	}
	return (c);
}

static void parsetagbuf()
{
	char *p;

	while ((p=strchr(tagbuf, '<')) != NULL)
		*p=' ';

        tagattrlen=parseattr(0);
        if ( tagattrlen > tagattrsize)
        {
        struct tagattrinfo *newta= tagattr ? (struct tagattrinfo *)
                realloc(tagattr, (tagattrlen+16)*sizeof(*tagattr))
                :(struct tagattrinfo *)
			malloc((tagattrlen+16)*sizeof(*tagattr));

                if (!newta)     enomem();
		tagattrsize=tagattrlen+16;
		tagattr=newta;
        }
        parseattr(tagattr);
}

/* See if this attribute is the one we're looking for */

static int is_attr(struct tagattrinfo *i, const char *l)
{
size_t ll=strlen(l);

	return (i->tagnamelen == ll && strncasecmp(i->tagname, l, ll) == 0);
}

/* If this is the tag we're looking for */

static int is_htmltag(const char *l)
{
	return (tagattrlen ? is_attr(tagattr, l):0);
}

/* See if the attribute value starts with what we're looking for */

static int is_valuestart(const char *v, const char *l)
{
	while (v && isspace((int)(unsigned char)*v))
		++v;

	return (v && strncasecmp(v, l, strlen(l)) == 0);
}

/*
	htmlfilter() is repeatedly called to filter the HTML text.  htmlfilter()
	will call htmlfiltered() with the filtered text, more or less on a
	one to one basis.

	htmlfilter_init() must be called before the first invocation of
	htmlfilter().  Because the HTML can be fed in arbitrary quantities,
	htmlfilter() implements a state machine which htmlfilter_init()
	initializes.
*/

enum htmlstate {
	intext,			/* Initial value.  In plain text */
	seenlt,			/* Seen < */
	seenltbang,		/* Seen <! */
	seenltbangdash,		/* Seen <!- */
	intag,			/* Seen < but not <!, we're collecting the
				tag */

	incomment,		/* <!--, in a comment, have not seen any
				dashes */
	incommentseendash,	/* In a comment, seen - */
	incommentseendashdash	/* In a comment, seen -- */
	} ;

static enum htmlstate cur_state;
static unsigned instyletag;
static void filtered_tag();
static void (*htmlfiltered_func)(const char *, size_t);
static char *(*htmlconvertcid_func)(const char *, void *);
static void *convertcid_arg;

const char *washlink=0;
const char *washlinkmailto=0;
const char *contentbase=0;

void htmlfilter_init( void (*func)(const char *, size_t))
{
	cur_state=intext;
	htmlfiltered_func=func;
	instyletag=0;
}

/* Set prefix to wash HTML links */

void htmlfilter_washlink( const char *p)
{
	washlink=p;
}

void htmlfilter_contentbase( const char *p)
{
	contentbase=p;
}

void htmlfilter_washlinkmailto( const char *p)
{
	washlinkmailto=p;
}

void htmlfilter_convertcid( char *(*cidfunc)(const char *, void *), void *arg)
{
	htmlconvertcid_func=cidfunc;
	convertcid_arg=arg;
}

void htmlfilter(const char *p, size_t s)
{
size_t	l;
size_t	start=0;

	for (l=0; l<s; l++)
	{
		switch (cur_state)	{
		case intext:
			if (p[l] == '>')
			{
				(*htmlfiltered_func)(p+start, l-start);
				(*htmlfiltered_func)("&nbsp;", 6);
				start=l+1;
			}

			if (p[l] != '<')	continue;
			if (!instyletag)
				(*htmlfiltered_func)(p+start, l-start);
					/* Output everything up until the tag */
			cur_state=seenlt;
			tagbuflen=0;
			break;
		case seenlt:
			if (p[l] == '>')
			{
				cur_state=intext;
				start=l+1;
				if (!instyletag)
					(*htmlfiltered_func)("<>", 2);
						/* Eh? */
				continue;
			}
			if (isspace((int)(unsigned char)p[l]))
				break;
			if (p[l] == '!')
				cur_state=seenltbang;
			else if (p[l] != '/'
				 && !isalpha((int)(unsigned char)p[l]))
			{
				start=l+1;
				cur_state=intext;
				break;
			}
			else
				cur_state=intag;
			addtagbuf(p[l]);
			break;
		case intag:
			/* We're in a tag (not a <!-- comment)
			collect the contents in tagbuf, until > is seen */
do_intag:
			cur_state=intag;
			if (p[l] == '>')
			{
				start=l+1;
				cur_state=intext;
				filtered_tag();	/* Filter this tag */
				continue;
			}
			addtagbuf(p[l]);
			continue;

		case seenltbang:
			/* We have <!.  If - is not here, this is a SGML tag */
			if (p[l] != '-')	goto do_intag;
			addtagbuf(p[l]);
			cur_state=seenltbangdash;
			continue;

		case seenltbangdash:

			/* We have <!-. If - is not here, this is a SGML tag,
			otherweise we're in a comment, which we can pass
			along */

			if (p[l] != '-')	goto do_intag;
			if (!instyletag)
				(*htmlfiltered_func)("<!--", 4);
			start=l+1;
			cur_state=incomment;
			continue;

			/* Look for end of comment */

		case incomment:
			if (p[l] == '-')	cur_state=incommentseendash;
			continue;
		case incommentseendash:
			cur_state= p[l] == '-' ? incommentseendashdash
						:incomment;
			continue;
		case incommentseendashdash:
			if (p[l] == '-')	continue;
			if (p[l] != '>')
			{
				cur_state=incomment;
				continue;
			}
			if (!instyletag)
				(*htmlfiltered_func)(p+start, l+1-start);
			cur_state=intext;
			start=l+1;
			continue;
		}
	}

	/* When we're done with this chunk, if we're doing plain text, or if
	** we're in a comment, just pass it along */

	switch (cur_state)	{
	case intext:
	case incomment:
	case incommentseendash:
	case incommentseendashdash:
		if (!instyletag)
			(*htmlfiltered_func)(p+start, l-start);
	default:
		break;
	}
}

/* Ok, wash an HREF link.  The entire A (or whatever) element is in tagbuf.
** tag=value pairs have been parsed into tagattr array.
**
** Our argument is the index of the HREF (or SRC) link, which points back
** into tagbuf.
**
** We build a new element, and then rebuild the tagbuf structure.
*/

static char *redirectencode(const char *, size_t );

/* replacelink takes care of replacing the contents of one tag's value. */

static void replacelink(size_t l, const char *p)
{
struct tagattrinfo *tagattrp=tagattr+l;
char	*newbuf;
size_t	plen=tagattrp->tagvalue - tagbuf;
size_t	i;

	newbuf=malloc(strlen(tagbuf)+strlen(p)+1);
			/* Yes, that's a bit too much.  That's OK */

	if (!newbuf)	enomem();
	memcpy(newbuf, tagbuf, plen);
	strcpy(newbuf+plen, p);
	strcat(newbuf, tagattrp->tagvalue+tagattrp->tagvaluelen);

	tagbuflen=0;
	for (i=0; newbuf[i]; i++)
		addtagbuf(newbuf[i]);
	addtagbuf(0);
	parsetagbuf();
	free(newbuf);
}

static void dowashlink(size_t l, const char *value)
{
char	*url, *p;

	url=redirectencode(value, strlen(value));

	p=malloc(strlen(url)+strlen(washlink)+1);
	if (!p)	enomem();
	strcat(strcpy(p, washlink), url);
	replacelink(l, p);
	free(p);
	free(url);
}

static void dowashlinkmailto(size_t l, const char *mailtolink)
{
size_t mailtolinklen=strlen(mailtolink);
char	*newlink=malloc(strlen(washlinkmailto)+1+mailtolinklen);
char	*p;

	if (!newlink)	enomem();
	strcpy(newlink, washlinkmailto);
	strncat(newlink, mailtolink, mailtolinklen);
	if ((p=strchr(newlink+strlen(washlinkmailto), '?')) != 0)
		*p='&';
	replacelink(l, newlink);
	free(newlink);
}

static void dowashcid(size_t l, const char *link)
{
size_t linklen=strlen(link);
char	*p;

	p=malloc(linklen+1);
	if (!p)	enomem();

	memcpy(p, link, linklen);
	p[linklen]=0;

	if (!htmlconvertcid_func)
		*p=0;
	else
	{
	char	*q=(*htmlconvertcid_func)(p+4, convertcid_arg);

		free(p);
		p=q;
	}
	replacelink(l, p);
	free(p);
}

static char *redirectencode(const char *p, size_t l)
{
char	*q=malloc(l+1);
char	*r;

	if (!q)	enomem();
	memcpy(q, p, l);
	q[l]=0;
	decodehtmlchar(q);
	r=cgiurlencode(q);
	free(q);
	return (r);
}


size_t find_tag(const char *tagname)
{
size_t	l;

	for (l=1; l<tagattrlen; l++)
		if (is_attr(tagattr+l, tagname))	return (l);
	return (0);
}

/*
	Decide what to do with this tag
*/

static void filtered_tag()
{
size_t	l;
int	open_window=0;
 
	addtagbuf(0);
	parsetagbuf();
	if (	is_htmltag("TITLE") || is_htmltag("/TITLE") ||
		is_htmltag("SCRIPT") || is_htmltag("/SCRIPT") ||
		is_htmltag("FRAME") || is_htmltag("/FRAME") ||
		is_htmltag("IFRAME") || is_htmltag("/IFRAME") ||
		is_htmltag("APP") || is_htmltag("/APP") ||
		is_htmltag("APPLET") || is_htmltag("/APPLET") ||
		is_htmltag("SERVER") || is_htmltag("/SERVER") ||
		is_htmltag("OBJECT") || is_htmltag("/OBJECT") ||
		is_htmltag("HTML") || is_htmltag("/HTML") ||
		is_htmltag("HEAD") || is_htmltag("/HEAD") ||
		is_htmltag("BODY") || is_htmltag("/BODY") ||
		is_htmltag("LINK") || is_htmltag("META"))
	{
		return;
	}

	if ( is_htmltag("STYLE"))
	{
		++instyletag;
		return;
	}

	if ( is_htmltag("/STYLE"))
	{
		--instyletag;
		return;
	}

	if (instyletag)	return;


	for (l=1; l<tagattrlen; l++)
	{
		if (tagattr[l].tagnamelen > 2 &&
			strncasecmp(tagattr[l].tagname, "ON", 2) == 0)
		{
			memset(tagattr[l].atagstart, ' ',
				tagattr[l].ataglen);
		}
	}

	if (is_htmltag("IMG"))
	{
	size_t	nsrc=find_tag("src");
	size_t	nalt=find_tag("alt");
	int	ignore_img=0;

		/*
		** An IMG with a cid: URL is passed along, untouched, with
		** the URL being processed.  This is handled later.
		*/

		if (nsrc && htmlconvertcid_func)
		{
		const char *p=tagattr[nsrc].tagvalue;
		size_t s=tagattr[nsrc].tagvaluelen;

			while (s)
			{
				if ( !isspace((int)(unsigned char)*p)) break;

				++p;
				--s;
			}
			if (s >= 4 && strncasecmp(p, "cid:", 4) == 0)
			{
				nsrc=0;
				ignore_img=1;
				/* Handle tags below */
			}
		}

		if (nsrc)
		{
		char	*r;
		char	*q;
		char	*alt=0;

			r=malloc(tagattr[nsrc].tagvaluelen+1);
			if (!r)	enomem();
			memcpy(r, tagattr[nsrc].tagvalue,
				tagattr[nsrc].tagvaluelen);
			r[tagattr[nsrc].tagvaluelen]=0;
			q=rfc2045_append_url(contentbase, r);
			free(r);

			for (r=q; *r; r++)
				if (*r == '"' ||
					*r == '<' || *r == '>')	*r=0;
					/* Someone's playing games with us */

			if (nalt)
			{
				alt=malloc(tagattr[nalt].tagvaluelen+1);
				if (!alt)	enomem();
				memcpy(alt, tagattr[nalt].tagvalue,
					tagattr[nalt].tagvaluelen);
				alt[tagattr[nalt].tagvaluelen]=0;
			}

			(*htmlfiltered_func)("<A TARGET=\"_blank\" HREF=\"", 25);

			if (washlink)	dowashlink(nsrc, q);
			else	replacelink(nsrc, q);

			(*htmlfiltered_func)(tagattr[nsrc].tagvalue,
					tagattr[nsrc].tagvaluelen);
			(*htmlfiltered_func)("\">", 2);
			if (alt)
				(*htmlfiltered_func)(alt, strlen(alt));
			else
			{
				(*htmlfiltered_func)("[", 1);
				(*htmlfiltered_func)(q, strlen(q));
				(*htmlfiltered_func)("]", 1);
			}
			(*htmlfiltered_func)("</A>", 4);
			free(q);
			if (alt)	free(alt);
		}

		if (!ignore_img)
			return;
	}

	/* Attempt to automatically plug in any holes */

	for (l=1; l<tagattrlen; l++)
	{
		if (is_attr(tagattr+l, "target") ||
			is_attr(tagattr+l, "code") ||
			is_attr(tagattr+l, "language") ||
			is_attr(tagattr+l, "action") ||
			(is_attr(tagattr+l, "type")
			 && !is_htmltag("BLOCKQUOTE")) ||
			is_attr(tagattr+l, "codetype"))
			memset(tagattr[l].atagstart, ' ',
				tagattr[l].ataglen);

		if (is_attr(tagattr+l, "href")
			|| is_attr(tagattr+l, "src")
			|| is_attr(tagattr+l, "lowsrc"))
		{
		char	*p=malloc(tagattr[l].tagvaluelen+1), *q;
		size_t	n;
		int	goodhref=0;

			if (!p)	enomem();

			memcpy(p, tagattr[l].tagvalue, tagattr[l].tagvaluelen);
			p[tagattr[l].tagvaluelen]=0;

			q=rfc2045_append_url(contentbase, p);
			free(p);

			for (p=q; *p; p++)
				if (*p == '"' ||
					*p == '<' || *p == '>')	*p=0;
					/* Someone's playing games with us */

			for (n=0; q[n]; n++)
				if (q[n] == ':')
				{
					if (is_valuestart(q, "cid:"))
					{
						goodhref=1;
						dowashcid(l, q);
					}
					else if ((is_valuestart(q, "http:") ||
						is_valuestart(q, "https:")) &&
						is_attr(tagattr+l, "href"))
/* block src/lowsrc tags in anything but IMG.
Ex: <input type="image" src="http://"> -- don't render as a redirect
URL
*/
					{
						goodhref=1;
						if (washlink)
							dowashlink(l, q);
						else
							replacelink(l, q);
						if (is_htmltag("A"))
							open_window=1;
						break;
					}
					else if ( is_valuestart(q, "mailto:"))
					{
						goodhref=1;
						dowashlinkmailto(l, strchr(q,
							':')+1);
					}
					else if ( is_valuestart(q, "ftp:") ||
						is_valuestart(q, "gopher:") ||
						is_valuestart(q, "wais:") ||
						is_valuestart(q, "telnet:"))
					{
						goodhref=1;
						if (is_htmltag("A"))
							open_window=1;
					}
					break;
				}
			if (!goodhref)
			{
				memset(tagattr[l].atagstart, ' ',
					tagattr[l].ataglen);
			}
			free(q);
		}
	}

	(*htmlfiltered_func)("<", 1);
	(*htmlfiltered_func)(tagbuf, strlen(tagbuf));
	if (open_window)
		(*htmlfiltered_func)(" target=\"_blank\"", 16);
	(*htmlfiltered_func)(">", 1);

}
