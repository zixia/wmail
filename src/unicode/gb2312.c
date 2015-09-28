/*
** Copyright 2000-2002 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: gb2312.c,v 1.1.1.1 2003/05/07 02:13:50 lfan Exp $
*/

#include "gb2312.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unicode_char * const gb2312[]= {
	gb2312_b0,
	gb2312_b1,
	gb2312_b2,
	gb2312_b3,
	gb2312_b4,
	gb2312_b5,
	gb2312_b6,
	gb2312_b7,
	gb2312_b8,
	gb2312_b9,
	gb2312_ba,
	gb2312_bb,
	gb2312_bc,
	gb2312_bd,
	gb2312_be,
	gb2312_bf,
	gb2312_c0,
	gb2312_c1,
	gb2312_c2,
	gb2312_c3,
	gb2312_c4,
	gb2312_c5,
	gb2312_c6,
	gb2312_c7,
	gb2312_c8,
	gb2312_c9,
	gb2312_ca,
	gb2312_cb,
	gb2312_cc,
	gb2312_cd,
	gb2312_ce,
	gb2312_cf,
	gb2312_d0,
	gb2312_d1,
	gb2312_d2,
	gb2312_d3,
	gb2312_d4,
	gb2312_d5,
	gb2312_d6,
	gb2312_d7,
	gb2312_d8,
	gb2312_d9,
	gb2312_da,
	gb2312_db,
	gb2312_dc,
	gb2312_dd,
	gb2312_de,
	gb2312_df,
	gb2312_e0,
	gb2312_e1,
	gb2312_e2,
	gb2312_e3,
	gb2312_e4,
	gb2312_e5,
	gb2312_e6,
	gb2312_e7,
	gb2312_e8,
	gb2312_e9,
	gb2312_ea,
	gb2312_eb,
	gb2312_ec,
	gb2312_ed,
	gb2312_ee,
	gb2312_ef,
	gb2312_f0,
	gb2312_f1,
	gb2312_f2,
	gb2312_f3,
	gb2312_f4,
	gb2312_f5,
	gb2312_f6,
	gb2312_f7};

static unicode_char *c2u(const struct unicode_info *u,
			 const char *cp, int *err)
{
	size_t i, cnt;
	unicode_char *uc;

	if (err)
		*err= -1;

	/*
	** Count the number of potential unicode characters first.
	*/

	for (i=cnt=0; cp[i]; i++)
	{
		if ( (int)(unsigned char)cp[i] < 0xB0 ||
		     (int)(unsigned char)cp[i] > 0xF7 ||
		     cp[i+1] == 0)
		{
			++cnt;
			continue;
		}

		++i;
		++cnt;
	}

	uc=malloc((cnt+1)*sizeof(unicode_char));
	if (!uc)
		return (NULL);

	cnt=0;

	for (i=cnt=0; cp[i]; i++)
	{
		int a=(int)(unsigned char)cp[i], b;

		if ( a >= 0xB0 && a <= 0xF7 && cp[i+1])
		{
			b=(int)(unsigned char)cp[i+1];
			++i;

			if (b >= 0xa1 && b < 0xFF)
			{
				unicode_char ucv=gb2312[a-0xb0][b-0xa1];

				if (ucv)
				{
					uc[cnt++]= ucv;
					continue;
				}
			}
		}
		else
		{
			uc[cnt++]=a;
			continue;
		}

		if (err)
		{
			*err=i;
			free(uc);
			return (NULL);
		}
		uc[cnt++]= 0xFFFD;
	}
	uc[cnt]=0;

	return (uc);
}

static unsigned revlookup(unicode_char c)
{
	unsigned j;
	unsigned bucket;
	unsigned uc;

	bucket=c % gb2312_revhash_size;
	uc=0;

	for (j=gb2312_revtable_index[ bucket ];
	     j < sizeof(gb2312_revtable_uc)/sizeof(gb2312_revtable_uc[0]);
	     ++j)
	{
		unicode_char uuc=gb2312_revtable_uc[j];

		if (uuc == c)
			return (gb2312_revtable_octets[j]);

		if ((uuc % gb2312_revhash_size) != bucket)
			break;
	}
	return (0);
}

static char *u2c(const struct unicode_info *u,
		 const unicode_char *cp, int *err)
{
	size_t cnt, i;
	char *s;

	if (err)
		*err= -1;
	/*
	** Figure out the size of the octet string.  Unicodes < 0x7f will
	** map to a single byte, unicodes >= 0x80 will map to two bytes.
	*/

	for (i=cnt=0; cp[i]; i++)
	{
		if (cp[i] > 0x7f)
			++cnt;
		++cnt;
	}

	s=malloc(cnt+1);
	if (!s)
		return (NULL);
	cnt=0;

	for (i=0; cp[i]; i++)
	{
		unsigned uc;

		if (cp[i] < 0x80)
		{
			s[cnt++]= (char)cp[i];
			continue;
		}

		uc=revlookup(cp[i]);

		if (!uc)
		{
			if (err)
			{
				*err=i;
				free(s);
				return (NULL);
			}
			uc=65535;
		}
		s[cnt++]= (char)(uc / 256);
		s[cnt++]= (char)uc;
	}
	s[cnt]=0;
	return (s);
}

static char *toupper_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
	unicode_char *uc=c2u(u, cp, ip);
	char *s;

	int dummy;
	unsigned i;

	if (!uc)
		return (NULL);

	for (i=0; uc[i]; i++)
	{
		unicode_char c=unicode_uc(uc[i]);

		if (revlookup(c))
			uc[i]=c;
	}

	s=u2c(u, uc, &dummy);
	free(uc);
	return (s);
}

static char *tolower_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
	unicode_char *uc=c2u(u, cp, ip);
	char *s;

	int dummy;
	unsigned i;

	if (!uc)
		return (NULL);

	for (i=0; uc[i]; i++)
	{
		unicode_char c=unicode_lc(uc[i]);

		if (revlookup(c))
			uc[i]=c;
	}

	s=u2c(u, uc, &dummy);
	free(uc);
	return (s);
}

static char *totitle_func(const struct unicode_info *u,
			  const char *cp, int *ip)
{
	unicode_char *uc=c2u(u, cp, ip);
	char *s;

	int dummy;
	unsigned i;

	if (!uc)
		return (NULL);

	for (i=0; uc[i]; i++)
	{
		unicode_char c=unicode_tc(uc[i]);

		if (revlookup(c))
			uc[i]=c;
	}

	s=u2c(u, uc, &dummy);
	free(uc);
	return (s);
}

const struct unicode_info unicode_GB2312 = {
	"GB2312",
	UNICODE_MB,
	c2u,
	u2c,
	toupper_func,
	tolower_func,
	totitle_func};

#if 0

int main()
{
	FILE *fp=popen("gunzip -cd <Unihan-3.2.0.txt.gz", "r");
	char buf[4000];
	unicode_char *uc;
	char *s, *p;
	int dummyi;

	if (!fp)
		return (0);

	while (fgets(buf, sizeof(buf), fp))
	{
		unsigned a, b, c;
		int dummy;

		if (sscanf(buf, "U+%4x kIRG_GSource 0-%4x", &b, &a) != 2)
			continue;
		a |= 0x8080;

		printf("0x%04x 0x%04x: ", a, b);

		buf[0]= a / 256;
		buf[1]= a % 256;
		buf[2]=0;

		uc=c2u(buf, &dummy);
		if (!uc)
		{
			printf("c2u failure: %d\n", dummy);
			return (1);
		}
		if (uc[0] != b || uc[1])
		{
			printf("c2u failure: got 0x%04x, expected 0x%04x\n",
			       (unsigned)uc[0], (unsigned)b);
			return (1);
		}
		s=u2c(uc, &dummy);
		free(uc);
		if (!s)
		{
			printf("u2c failure: %d\n", dummy);
			return (1);
		}

		c=0;
		if (!s[0] || !s[1] || s[2] ||
		    (c=(int)(unsigned char)s[0] * 256 +
		     (unsigned char)s[1]) != a)
		{
			printf("u2c failure: got 0x%04x, expected 0x%04x\n",
			       c, a);
			return (1);
		}

		p=toupper_func(s, NULL);
		if (!p)
		{
			printf("toupper failure\n");
			return (1);
		}
		if (strcmp(p, s))
			printf("toupper ");
		free(p);

		p=tolower_func(s, NULL);
		if (!p)
		{
			printf("tolower failure\n");
			return (1);
		}
		if (strcmp(p, s))
			printf("tolower ");
		free(p);

		p=totitle_func(s, NULL);
		if (!p)
		{
			printf("totitle failure\n");
			return (1);
		}
		if (strcmp(p, s))
			printf("totitle ");
		free(p);

		free(s);
		printf("ok\n");
	}
	fclose(fp);

	buf[0]=0x40;
	buf[1]=0;
	uc=c2u(buf, NULL);

	if (!uc)
	{
		printf("us-ascii c2u failure\n");
		return (1);
	}
	s=u2c(uc, NULL);
	free(uc);
	if (!s)
	{
		printf("us-asccu u2c failure\n");
		return (1);
	}
	free(s);

	buf[0]=0x40;
	buf[1]=0xF0;
	buf[2]=0;

	uc=c2u(buf, NULL);
	if (!uc)
	{
		printf("fallback failed\n");
		return (1);
	}
	printf("fallback: %04x %04x\n", (unsigned)uc[0],
	       (unsigned)uc[1]);

	s=u2c(uc, NULL);
	free(uc);

	if (!s)
	{
		printf("fallback-reverse failed\n");
		return (1);
	}
	printf("fallback: %02x %02x\n", (int)(unsigned char)s[0],
	       (int)(unsigned char)s[1]);
	free(s);

	buf[0]=0xB2;
	buf[1]=0x20;
	buf[2]=0;

	uc=c2u(buf, &dummyi);

	if (uc)
	{
		printf("abort failed\n");
		return (1);
	}

	printf("aborted at index %d\n", dummyi);

	{
		static unicode_char testing[]={0x0040, 0x1000, 0};

		uc=testing;

		s=u2c(uc, NULL);
		
		if (!s)
		{
			printf("abort-fallback failed\n");
			return (1);
		}
		printf("abort-fallback: %02x %02x\n", (int)(unsigned char)s[0],
		       (int)(unsigned char)s[1]);
		free(s);

		uc=testing;
	}

	s=u2c(uc, &dummyi);

	if (s)
	{
		printf("abort-abort failed\n");
		return (1);
	}

	printf("abort-aborted: index %d\n", dummyi);
	return (0);
}
#endif
