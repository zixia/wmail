/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

/*
** $Id: imapsubj.c,v 1.1.1.1 2003/05/07 02:14:05 lfan Exp $
*/
#include	<stdio.h>
#include	<ctype.h>
#include	<stdlib.h>
#include	<string.h>
#include	"rfc822.h"
#include	"config.h"

#if	HAVE_STRCASECMP

#else
#define	strcasecmp	stricmp
#endif

#if	HAVE_STRNCASECMP

#else
#define	strncasecmp	strnicmp
#endif

/* Skip over blobs */

static char *skipblob(char *p)
{
	char *q;

	if (*p == '[')
	{
		for (q= p+1; *q; q++)
			if (*q == '[' || *q == ']')
				break;
		if (*q == ']')
		{
			p=q+1;

			while (isspace((int)(unsigned char)*p))
			{
				++p;
			}

			return (p);
		}
	}
	return (p);
}

static char *skipblobs(char *p)
{
	char *q=p;

	do
	{
		p=q;
		q=skipblob(p);
	} while (q != p);
	return (q);
}

/* Remove artifacts from the subject header */

static void stripsubj(char *s, int *hasrefwd)
{
	char	*p;
	char	*q;
	int doit;

	for (p=q=s; *p; p++)
	{
		if (!isspace((int)(unsigned char)*p))
		{
			*q++=*p;
			continue;
		}
		while (p[1] && isspace((int)(unsigned char)p[1]))
		{
			++p;
		}
		*q++=' ';
	}
	*q=0;

	do
	{
		doit=0;
		/*
		**
		** (2) Remove all trailing text of the subject that matches
		** the subj-trailer ABNF, repeat until no more matches are
		** possible.
		**
		**  subj-trailer    = "(fwd)" / WSP
		*/

		for (p=s; *p; p++)
			;
		while (p > s)
		{
			if ( isspace((int)(unsigned char)p[-1]))
			{
				--p;
				continue;
			}
			if (p-s >= 5 && strncasecmp(p-5, "(FWD)", 5) == 0)
			{
				p -= 5;
				*hasrefwd=1;
				continue;
			}
			break;
		}
		*p=0;

		for (p=s; *p; )
		{
			for (;;)
			{
				/*
				**
				** (3) Remove all prefix text of the subject
				** that matches the subj-leader ABNF.
				**
				**   subj-leader     = (*subj-blob subj-refwd) / WSP
				**
				**   subj-blob       = "[" *BLOBCHAR "]" *WSP
				**
				**   subj-refwd      = ("re" / ("fw" ["d"])) *WSP [subj-blob] ":"
				**
				**   BLOBCHAR        = %x01-5a / %x5c / %x5e-7f
				**                   ; any CHAR except '[' and ']'
				*/

				if (isspace((int)(unsigned char)*p))
				{
					++p;
					continue;
				}

				q=skipblobs(p);

				if (strncasecmp(q, "RE", 2) == 0)
				{
					q += 2;
				}
				else if (strncasecmp(q, "FWD", 3) == 0)
				{
					q += 3;
				}
				else if (strncasecmp(q, "FW", 2) == 0)
				{
					q += 2;
				}
				else q=0;

				if (q)
				{
					q=skipblob(q);
					if (*q == ':')
					{
						p=q+1;
						*hasrefwd=1;
						continue;
					}
				}


				/*
				** (4) If there is prefix text of the subject
				** that matches the subj-blob ABNF, and
				** removing that prefix leaves a non-empty
				** subj-base, then remove the prefix text.
				**
				**   subj-base       = NONWSP *([*WSP] NONWSP)
				**                   ; can be a subj-blob
				*/

				q=skipblob(p);

				if (q != p && *q)
				{
					p=q;
					continue;
				}
				break;
			}

			/*
			**
			** (6) If the resulting text begins with the
			** subj-fwd-hdr ABNF and ends with the subj-fwd-trl
			** ABNF, remove the subj-fwd-hdr and subj-fwd-trl and
			** repeat from step (2).
			**
			**   subj-fwd-hdr    = "[fwd:"
			**
			**   subj-fwd-trl    = "]"
			*/

			if (strncasecmp(p, "[FWD:", 5) == 0)
			{
				q=strrchr(p, ']');
				if (q && q[1] == 0)
				{
					*q=0;
					p += 5;
					*hasrefwd=1;

					for (q=s; (*q++=*p++) != 0; )
						;
					doit=1;
				}
			}
			break;
		}
	} while (doit);

	q=s;
	while ( (*q++ = *p++) != 0)
		;
}

char *rfc822_coresubj(const char *s, int *hasrefwd)
{
	char *q=strdup(s), *r;
	int dummy;

	if (!hasrefwd)
		hasrefwd= &dummy;

	*hasrefwd=0;
	if (!q)	return (0);

	for (r=q; *r; r++)
		if ((*r & 0x80) == 0)	/* Just US-ASCII casing, thanks */
			*r=toupper((int)(unsigned char)*r);
	stripsubj(q, hasrefwd);
	return (q);
}

char *rfc822_coresubj_nouc(const char *s, int *hasrefwd)
{
	char *q=strdup(s);
	int dummy;

	if (!hasrefwd)
		hasrefwd= &dummy;

	*hasrefwd=0;
	if (!q)	return (0);

	stripsubj(q, hasrefwd);
	return (q);
}
