/*
** Copyright 2001 Double Precision, Inc.
** See COPYING for distribution information.
*/

#include	"config.h"
#include	"sha1.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

static const char rcsid[]="$Id: testsuite.c,v 1.1.1.1 2003/05/07 02:13:23 lfan Exp $";

int main()
{
SHA1_DIGEST	digest;
unsigned i, n;

static char foo[1000001];
static char *testcases[]={"abc",
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", foo};

	memset(foo, 'a', 1000000);
	for (n=0; n<sizeof(testcases)/sizeof(testcases[0]); n++)
	{
		i=strlen(testcases[n]);
		sha1_digest(testcases[n], i, digest);
		printf( (i < 200 ? "SHA1(%s)=":
			"SHA1(%-1.20s...)="), testcases[n]);

		for (i=0; i<20; i++)
		{
			if (i && (i & 3) == 0)	putchar(' ');
			printf("%02X", digest[i]);
		}
		printf("\n");
	}
	exit (0);
	return (0);
}
