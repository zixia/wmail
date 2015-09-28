/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: genkey.c,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

#include	"config.h"
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/time.h>

#include	"gpg.h"
#include	"gpglib.h"

#include	"unicode/unicode.h"
#include	"numlib/numlib.h"

extern int gpg_stdin, gpg_stdout, gpg_stderr;
extern pid_t gpg_pid;


/*
** Generate a new key.
*/

static char *toutf8(const struct unicode_info *, const char *);

static int dogenkey(const char *, const char *, const char *,
		    int,
		    int,
		    unsigned,
		    char,
		    const char *passphrase,
		    int (*)(const char *, size_t, void *),
		    int (*)(void *),
		    void *);

int gpg_genkey(const char *gpgdir, const struct unicode_info *charset,
	       const char *name,
	       const char *addr,
	       const char *comment,
	       int skeylen,
	       int ekeylen,
	       unsigned expire,
	       char expire_unit,
	       const char *passphrase,
	       int (*dump_func)(const char *, size_t, void *),
	       int (*timeout_func)(void *),
	       void *voidarg)
{
	char *name_u, *addr_u, *comment_u;
	char *argvec[4];
	int rc;

	name_u=toutf8(charset, name);
	if (!name_u)
		return (-1);

	addr_u=toutf8(charset, addr);
	if (!addr_u)
	{
		free(name_u);
		return (-1);
	}

	comment_u=toutf8(charset, comment);
	if (!comment_u)
		return (-1);


	argvec[0]="gpg";
	argvec[1]="--gen-key";
	argvec[2]="--batch";
	argvec[3]=NULL;

	if (gpg_fork(&gpg_stdin, &gpg_stdout, NULL, gpgdir, argvec) < 0)
		rc= -1;
	else
	{
		int rc2;

		rc=dogenkey(name_u, addr_u, comment_u,
			    skeylen, ekeylen, expire, expire_unit,
			    passphrase,
			    dump_func, timeout_func, voidarg);
		rc2=gpg_cleanup();
		if (rc2)
			rc=rc2;
	}
	free(comment_u);
	free(name_u);
	free(addr_u);
	return (rc);
}

static char *toutf8(const struct unicode_info *u, const char *ch)
{
	unicode_char *uc=(*u->c2u)(u, ch, NULL);
	char *p;

	if (!uc)
		return (NULL);

	p=unicode_toutf8(uc);
	free(uc);
	return (p);
}

static char *mkcmdbuf(const char *, const char *, const char *,
		      int,
		      int,
		      unsigned,
		      char, const char *);

static int dogenkey(const char *name, const char *addr, const char *comment,
		    int skeylen,
		    int ekeylen,
		    unsigned expire,
		    char expire_unit,
		    const char *passphrase,
		    int (*dump_func)(const char *, size_t, void *),
		    int (*timeout_func)(void *),
		    void *voidarg)
{
	char *cmd_buf=mkcmdbuf(name, addr, comment, skeylen, ekeylen,
			       expire, expire_unit, passphrase);

	int rc=gpg_write(cmd_buf, strlen(cmd_buf), dump_func, NULL,
			 timeout_func, 5, voidarg);
	int rc2;

	free(cmd_buf);
	if (rc == 0)
		rc=gpg_read(dump_func, NULL, timeout_func, 5, voidarg);
	rc2=gpg_cleanup();
	if (rc == 0)
		rc=rc2;
	return (rc);
}

static char *mkcmdbuf(const char *name, const char *addr, const char *comment,
		      int skeylen,
		      int ekeylen,
		      unsigned expire,
		      char expire_unit,
		      const char *passphrase)
{
	static const char genkey_cmd[]=
		"Key-Type: DSA\n"
		"Key-Length: %s\n"
		"Subkey-Type: ELG-E\n"
		"Subkey-Length: %s\n"
		"Name-Real: %s\n"
		"%s%s%s"
		"%s%s%s"
		"Name-Email: %s\n"
		"Expire-Date: %s\n"
		"%%commit\n";

	static const char comment1[]="Name-Comment: ";
	static const char comment2[]="\n";
	static const char passphrase1[]="Passphrase: ";
	static const char passphrase2[]="\n";

	char skl_buf[NUMBUFSIZE];
	char kl_buf[NUMBUFSIZE];
	char exp_buf[NUMBUFSIZE+1];

	char *p;

	libmail_str_size_t(skeylen, skl_buf);
	libmail_str_size_t(ekeylen, kl_buf);
	if (expire == 0)
	{
		strcpy(exp_buf, "0");
	}
	else
	{
		char b[2];

		b[0]=expire_unit;
		b[1]=0;

		strcat(libmail_str_size_t(expire, exp_buf), b);
	}

	p=malloc(256+strlen(kl_buf)+strlen(skl_buf)+strlen(exp_buf)
		 +strlen(name)+strlen(addr)+strlen(comment)
		 +strlen(passphrase));

	if (!p)
		return (NULL);

	while (*comment == ' ' || *comment == '\t')
		++comment;

	sprintf(p, genkey_cmd, skl_buf, kl_buf, name,
		*comment ? comment1:"",
		comment,
		*comment ? comment2:"",

		*passphrase ? passphrase1:"",
		passphrase,
		*passphrase ? passphrase2:"",

		addr, exp_buf);
	return (p);
}
