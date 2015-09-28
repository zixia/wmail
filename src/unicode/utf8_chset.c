/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: utf8_chset.c,v 1.1.1.1 2003/05/07 02:13:38 lfan Exp $
*/

#include "unicode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** UTF8.toupper/tolower/totitle is implemented by converting UTF8 to
** UCS-4, applying the unicode table lookup, then converting it back to
** UTF8
*/

static char *toupper_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
	unicode_char *uc=unicode_utf8_tou(cp, ip), *p;
	char *s;

	if (!uc) return (0);

	for (p=uc; *p; p++)
		*p=unicode_uc(*p);

	s=unicode_utf8_fromu(uc, ip);
	free(uc);
	return (s);
}

static char *tolower_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
	unicode_char *uc=unicode_utf8_tou(cp, ip), *p;
	char *s;

	if (!uc) return (0);

	for (p=uc; *p; p++)
		*p=unicode_lc(*p);

	s=unicode_utf8_fromu(uc, ip);
	free(uc);
	return (s);
}

static char *totitle_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
	unicode_char *uc=unicode_utf8_tou(cp, ip), *p;
	char *s;

	if (!uc) return (0);

	for (p=uc; *p; p++)
		*p=unicode_tc(*p);

	s=unicode_utf8_fromu(uc, ip);
	free(uc);
	return (s);
}

static unicode_char *tou(const struct unicode_info *i, const char *p,
			 int *err)
{
	return unicode_utf8_tou(p, err);
}

static char *fromu(const struct unicode_info *i, const unicode_char *p,
		   int *err)
{
	return unicode_utf8_fromu(p, err);
}

const struct unicode_info unicode_UTF8 = {
	"UTF-8",
	UNICODE_UTF | UNICODE_MB,
	tou,
	fromu,
	toupper_func,
	tolower_func,
	totitle_func};

