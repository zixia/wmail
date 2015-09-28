/*
** Copyright 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: testgpg.c,v 1.1.1.1 2003/05/07 02:14:58 lfan Exp $";


#include	"config.h"
#include	"gpglib.h"

#include	"unicode/unicode.h"
#include	"numlib/numlib.h"

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

static int dump_stdout(const char *p, size_t l, void *dummy)
{
	fwrite(p, l, 1, stdout);
	fflush(stdout);
	return (0);
}

static int poll_wait(void *dummy)
{
	putchar('*');
	fflush(stdout);
	return (0);
}

static void genkey(const char *d)
{
	gpg_genkey(d, &unicode_ISO8859_1,
		   "John Smith",
		   "john@example.com",
		   "Dummy ISO-8859 Tëëst key",
		   1024,
		   2048,
		   12,
		   'm',
		   "",
		   dump_stdout,
		   poll_wait,
		   NULL);
}

static int show_key(const char *fingerprint, const char *shortname,
		    const char *key, int invalid,
		    struct gpg_list_info *dummy)
{
	printf("Fingerprint: %s\nShort: %s\n%s\n\n",
	       fingerprint,
	       shortname,
	       key);
	fflush(stdout);
	return (0);
}

static void genlist(const char *d, int flag)
{
	struct gpg_list_info gli;

	memset(&gli, 0, sizeof(gli));

	gpg_listkeys(d, flag, show_key, dump_stdout, &gli);
}

static int expkey(const char *d, const char *f, int flag)
{
	return (gpg_exportkey(d, flag, f, dump_stdout, dump_stdout, NULL));
}

static int delkey(const char *d, const char *f, int flag)
{
	return (gpg_deletekey(d, flag, f, dump_stdout, NULL));
}

static int signkey(const char *d, const char *signthis, const char *signwith)
{
	return (gpg_signkey(d, signthis, signwith, -1, dump_stdout, 0, NULL));
}

static int checksign(const char *d, const char *stuff, const char *sig)
{
	return (gpg_checksign(d, stuff, sig, dump_stdout, NULL));
}

static int import(const char *filename)
{
	FILE *f=fopen(filename, "r");
	int rc;

	if (!f)
	{
		perror(filename);
		return (1);
	}

	if ((rc=gpg_import_start(NULL, 1)) == 0)
	{
		char buf[BUFSIZ];
		int n;

		while ((n=fread(buf, 1, sizeof(buf), f)) > 0)
		{
			if ((rc=gpg_import_do(buf, n, dump_stdout, NULL)) != 0)
				return (rc);
		}

		rc=gpg_import_finish(dump_stdout, NULL);
	}
	return (rc);
}

int main(int argc, char **argv)
{
	if (argc < 3)
		return (0);

	if (strcmp(argv[2], "gen") == 0)
	{
		genkey(argv[1]);
		return (0);
	}

	if (strcmp(argv[2], "listpub") == 0)
	{
		genlist(argv[1], 0);
		return (0);
	}

	if (strcmp(argv[2], "listsec") == 0)
	{
		genlist(argv[1], 1);
		return (0);
	}

	if (strcmp(argv[2], "exppub") == 0 && argc >= 4)
	{
		exit(expkey(argv[1], argv[3], 0));
		return (0);
	}

	if (strcmp(argv[2], "expsec") == 0 && argc >= 4)
	{
		exit(expkey(argv[1], argv[3], 1));
		return (0);
	}

	if (strcmp(argv[2], "delpub") == 0 && argc >= 4)
	{
		exit (delkey(argv[1], argv[3], 0));
		return (0);
	}

	if (strcmp(argv[2], "delsec") == 0 && argc >= 4)
	{
		exit (delkey(argv[1], argv[3], 1));
		return (0);
	}

	if (strcmp(argv[2], "sign") == 0 && argc >= 5)
	{
		exit(signkey(argv[1], argv[3], argv[4]));
		return (0);
	}

	if (strcmp(argv[2], "checksign") == 0 && argc >= 5)
	{
		exit(checksign(argv[1], argv[3], argv[4]));
		return (0);
	}

	if (strcmp(argv[2], "import") == 0)
		exit(import(argv[1]));

	return (0);
}
