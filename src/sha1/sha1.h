#ifndef	sha1_h
#define	sha1_h

/*
** Copyright 2001 Double Precision, Inc.
** See COPYING for distribution information.
*/

static const char sha1_h_rcsid[]="$Id: sha1.h,v 1.1.1.1 2003/05/07 02:13:23 lfan Exp $";

#if	HAVE_CONFIG_H
#include	"sha1/config.h"
#endif

#if	HAVE_SYS_TYPES_H
#include	<sys/types.h>
#endif

#define	SHA1_DIGEST_SIZE	20
#define	SHA1_BLOCK_SIZE		64

#ifdef	__cplusplus
extern "C" {
#endif

typedef	unsigned char SHA1_DIGEST[20];

#ifdef	SHA1_INTERNAL

struct SHA1_CONTEXT {

	SHA1_WORD	H[5];

	unsigned char blk[SHA1_BLOCK_SIZE];
	unsigned blk_ptr;
	} ;

void sha1_context_init(struct SHA1_CONTEXT *);
void sha1_context_hash(struct SHA1_CONTEXT *,
		const unsigned char[SHA1_BLOCK_SIZE]);
void sha1_context_hashstream(struct SHA1_CONTEXT *, const void *, unsigned);
void sha1_context_endstream(struct SHA1_CONTEXT *, unsigned long);
void sha1_context_digest(struct SHA1_CONTEXT *, SHA1_DIGEST);
void sha1_context_restore(struct SHA1_CONTEXT *, const SHA1_DIGEST);

#endif

void sha1_digest(const void *, unsigned, SHA1_DIGEST);
const char *sha1_hash(const char *);

#ifdef	__cplusplus
 } ;
#endif

#endif
