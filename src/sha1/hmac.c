/*
** Copyright 2001 Double Precision, Inc.
** See COPYING for distribution information.
*/
#define	SHA1_INTERNAL
#include	"sha1.h"
#include	"../libhmac/hmac.h"

static const char rcsid[]="$Id: hmac.c,v 1.1.1.1 2003/05/07 02:13:23 lfan Exp $";

static void alloc_context( void (*func)(void *, void *), void *arg)
{
struct	SHA1_CONTEXT c;

	(*func)((void *)&c, arg);
}

static void alloc_hash( void (*func)(unsigned char *, void *), void *arg)
{
unsigned char c[SHA1_DIGEST_SIZE];

	(*func)(c, arg);
}

struct hmac_hashinfo hmac_sha1 = {
	"sha1",
	SHA1_BLOCK_SIZE,
	SHA1_DIGEST_SIZE,
	sizeof(struct SHA1_CONTEXT),
	(void (*)(void *))sha1_context_init,
	(void (*)(void *, const void *, unsigned))sha1_context_hashstream,
	(void (*)(void *, unsigned long))sha1_context_endstream,
	(void (*)(void *, unsigned char *))sha1_context_digest,
	(void (*)(void *, const unsigned char *))sha1_context_restore,
        alloc_context,
	alloc_hash};
