/*
** Copyright 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

/*
** $Id: rfc2231encode.c,v 1.1.1.1 2003/05/07 02:14:11 lfan Exp $
*/

#if    HAVE_CONFIG_H
#include "rfc2045_config.h"
#endif
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	"rfc2045.h"

static const char xdigit[]="0123456789ABCDEFabcdef";

#define DOENCODE(c) \
	(strchr("'\"\\%:;=", (c)) || (c) <= ' ' || (c) >= 127)

void rfc2231_attrCreate(const char *name, const char *value,
			const char *charset, char *strPtr,
			int *lenPtr, const char *language)
{
	int l;

	*lenPtr=0;

	for (l=0; value[l]; l++)
		if (DOENCODE(value[l]))
			break;

	if (value[l] == 0) /* No need to encode */
	{
		l=strlen(name);

		if (strPtr)
		{
			memcpy(strPtr, name, l);
			strPtr += l;
			*strPtr++ = '=';
		}
		*lenPtr += l+1;

		l=strlen(value);
		if (strPtr)
		{
			memcpy(strPtr, value, l);
			strPtr += l;
		}
		*lenPtr += l;

		if (strPtr)
			*strPtr++ = 0;
		++*lenPtr;
		return;
	}


	l=strlen(name);

	if (strPtr)
	{
		memcpy(strPtr, name, l);
		strPtr += l;
		*strPtr++ = '*';
		*strPtr++ = '=';
	}
	*lenPtr += l+2;

	l=strlen(charset);

	if (strPtr)
	{
		memcpy(strPtr, charset, l);
		strPtr += l;
		*strPtr++ = '\'';
	}
	*lenPtr += l+1;

	if (!language)
		language="";

	l=strlen(language);

	if (strPtr)
	{
		memcpy(strPtr, language, l);
		strPtr += l;
		*strPtr++ = '\'';
	}
	*lenPtr += l+1;

	while (value && *value)
	{
		if (DOENCODE(*value))
		{
			if (strPtr)
			{
				*strPtr++ = '%';
				*strPtr++ = xdigit[ ((unsigned char)*value / 16) & 15];
				*strPtr++ = xdigit[ *value & 15];
			}

			*lenPtr += 3;
		}
		else
		{
			if (strPtr)
				*strPtr++ = *value;
			++*lenPtr;
		}
		++value;
	}

	if (strPtr)
		*strPtr++ = 0;
	++*lenPtr;
}
