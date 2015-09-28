/*
** Copyright 2000-2002 Double Precision, Inc.
** See COPYING for distribution information.
**
** $Id: big5.c,v 1.1.1.1 2003/05/07 02:13:46 lfan Exp $
*/

#include "big5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const unicode_char * const big5fwdlo[]= {
	NULL,
	big5_a2_lo,
	NULL,
	big5_a4_lo,
	big5_a5_lo,
	big5_a6_lo,
	big5_a7_lo,
	big5_a8_lo,
	big5_a9_lo,
	big5_aa_lo,
	big5_ab_lo,
	big5_ac_lo,
	big5_ad_lo,
	big5_ae_lo,
	big5_af_lo,
	big5_b0_lo,
	big5_b1_lo,
	big5_b2_lo,
	big5_b3_lo,
	big5_b4_lo,
	big5_b5_lo,
	big5_b6_lo,
	big5_b7_lo,
	big5_b8_lo,
	big5_b9_lo,
	big5_ba_lo,
	big5_bb_lo,
	big5_bc_lo,
	big5_bd_lo,
	big5_be_lo,
	big5_bf_lo,
	big5_c0_lo,
	big5_c1_lo,
	big5_c2_lo,
	big5_c3_lo,
	big5_c4_lo,
	big5_c5_lo,
	big5_c6_lo,
	NULL,
	NULL,
	big5_c9_lo,
	big5_ca_lo,
	big5_cb_lo,
	big5_cc_lo,
	big5_cd_lo,
	big5_ce_lo,
	big5_cf_lo,
	big5_d0_lo,
	big5_d1_lo,
	big5_d2_lo,
	big5_d3_lo,
	big5_d4_lo,
	big5_d5_lo,
	big5_d6_lo,
	big5_d7_lo,
	big5_d8_lo,
	big5_d9_lo,
	big5_da_lo,
	big5_db_lo,
	big5_dc_lo,
	big5_dd_lo,
	big5_de_lo,
	big5_df_lo,
	big5_e0_lo,
	big5_e1_lo,
	big5_e2_lo,
	big5_e3_lo,
	big5_e4_lo,
	big5_e5_lo,
	big5_e6_lo,
	big5_e7_lo,
	big5_e8_lo,
	big5_e9_lo,
	big5_ea_lo,
	big5_eb_lo,
	big5_ec_lo,
	big5_ed_lo,
	big5_ee_lo,
	big5_ef_lo,
	big5_f0_lo,
	big5_f1_lo,
	big5_f2_lo,
	big5_f3_lo,
	big5_f4_lo,
	big5_f5_lo,
	big5_f6_lo,
	big5_f7_lo,
	big5_f8_lo,
	big5_f9_lo};

static const unicode_char * const big5fwdhi[]= {
	NULL,
	big5_a2_hi,
	NULL,
	big5_a4_hi,
	big5_a5_hi,
	big5_a6_hi,
	big5_a7_hi,
	big5_a8_hi,
	big5_a9_hi,
	big5_aa_hi,
	big5_ab_hi,
	big5_ac_hi,
	big5_ad_hi,
	big5_ae_hi,
	big5_af_hi,
	big5_b0_hi,
	big5_b1_hi,
	big5_b2_hi,
	big5_b3_hi,
	big5_b4_hi,
	big5_b5_hi,
	big5_b6_hi,
	big5_b7_hi,
	big5_b8_hi,
	big5_b9_hi,
	big5_ba_hi,
	big5_bb_hi,
	big5_bc_hi,
	big5_bd_hi,
	big5_be_hi,
	big5_bf_hi,
	big5_c0_hi,
	big5_c1_hi,
	big5_c2_hi,
	big5_c3_hi,
	big5_c4_hi,
	big5_c5_hi,
	big5_c6_hi,
	NULL,
	NULL,
	big5_c9_hi,
	big5_ca_hi,
	big5_cb_hi,
	big5_cc_hi,
	big5_cd_hi,
	big5_ce_hi,
	big5_cf_hi,
	big5_d0_hi,
	big5_d1_hi,
	big5_d2_hi,
	big5_d3_hi,
	big5_d4_hi,
	big5_d5_hi,
	big5_d6_hi,
	big5_d7_hi,
	big5_d8_hi,
	big5_d9_hi,
	big5_da_hi,
	big5_db_hi,
	big5_dc_hi,
	big5_dd_hi,
	big5_de_hi,
	big5_df_hi,
	big5_e0_hi,
	big5_e1_hi,
	big5_e2_hi,
	big5_e3_hi,
	big5_e4_hi,
	big5_e5_hi,
	big5_e6_hi,
	big5_e7_hi,
	big5_e8_hi,
	big5_e9_hi,
	big5_ea_hi,
	big5_eb_hi,
	big5_ec_hi,
	big5_ed_hi,
	big5_ee_hi,
	big5_ef_hi,
	big5_f0_hi,
	big5_f1_hi,
	big5_f2_hi,
	big5_f3_hi,
	big5_f4_hi,
	big5_f5_hi,
	big5_f6_hi,
	big5_f7_hi,
	big5_f8_hi,
	big5_f9_hi};

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
		if ( (int)(unsigned char)cp[i] <= 0xA1 ||
		     (int)(unsigned char)cp[i] == 0xA3 ||
		     (int)(unsigned char)cp[i] > 0xF9 ||
		     (int)(unsigned char)cp[i] == 0xC7 ||
		     (int)(unsigned char)cp[i] == 0xC8 ||
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

		if ( a > 0xA1 && a <= 0xF9 && a != 0xA3
		     && a != 0xC7 && a != 0xC8 && cp[i+1])
		{
			b=(int)(unsigned char)cp[i+1];

			++i;
			if (b >= 64 && b < 128 && big5fwdlo[a-0xa1])
			{
				unicode_char ucv=big5fwdlo[a-0xa1][b-64];

				if (ucv)
				{
					uc[cnt++]= ucv;
					continue;
				}
			}
			else if (b >= 161 && b < 256 && big5fwdhi[a-0xa1])
			{
				unicode_char ucv=big5fwdhi[a-0xa1][b-161];

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

	bucket=c % big5_revhash_size;
	uc=0;

	for (j=big5_revtable_index[ bucket ];
	     j < sizeof(big5_revtable_uc)/sizeof(big5_revtable_uc[0]);
	     ++j)
	{
		unicode_char uuc=big5_revtable_uc[j];

		if (uuc == c)
			return (big5_revtable_octets[j]);

		if ((uuc % big5_revhash_size) != bucket)
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

const struct unicode_info unicode_BIG5 = {
	"BIG5",
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

		if (sscanf(buf, "U+%4x kBigFive %4x", &b, &a) != 2)
			continue;
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
		if (s == NULL && uc[0] == 0xfffd)
		{
			free(uc);
			printf("Ok\n");
			continue;	/* Unmapped */
		}
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
		printf("us-ascii u2c failure\n");
		return (1);
	}
	free(s);

	buf[0]=0xA2;
	buf[1]=0x40;
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

	buf[0]=0xA2;
	buf[1]=0x40;
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
