#ifndef	gpglib_h
#define	gpglib_h
/*
** Copyright 2001-2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char gpglib_h_rcsid[]="$Id: gpglib.h,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

#ifdef  __cplusplus
extern "C" {
#endif

#include	"config.h"
#include	<sys/types.h>
#include	<stdlib.h>

struct unicode_info;

int gpg_cleanup();
int has_gpg(const char *gpgdir);

int gpg_genkey(const char *gpgdir,
	       const struct unicode_info *charset,
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
	       void *voidarg);

struct gpg_list_info {
	const struct unicode_info *charset;
	const char *disabled_msg;
	const char *revoked_msg;
	const char *expired_msg;
	void *voidarg;
} ;

int gpg_listkeys(const char *gpgdir,
		 int secret,
		 int (*callback_func)(const char *, const char *,
				      const char *, int,
				      struct gpg_list_info *),
		 int (*err_func)(const char *, size_t, void *),
		 struct gpg_list_info *);

int gpg_exportkey(const char *gpgdir,
		  int secret,
		  const char *fingerprint,
		  int (*out_func)(const char *, size_t, void *),
		  int (*err_func)(const char *, size_t, void *),
		  void *voidarg);

int gpg_deletekey(const char *gpgdir, int secret, const char *fingerprint,
		  int (*dump_func)(const char *, size_t, void *),
		  void *voidarg);

int gpg_signkey(const char *gpgdir, const char *signthis, const char *signwith,
		int passphrase_fd,
		int (*dump_func)(const char *, size_t, void *),
		int trustlevel,
		void *voidarg);

int gpg_checksign(const char *gpgdir,
		  const char *content,	/* Filename, for now */
		  const char *signature, /* Filename, for now */
		  int (*dump_func)(const char *, size_t, void *),
		  void *voidarg);

	/* IMPORT A KEY */

int gpg_import_start(const char *gpgdir, int issecret);

int gpg_import_do(const char *p, size_t n,	/* Part of the key */
		  int (*dump_func)(const char *, size_t, void *),
		  /* gpg output callback */

		  void *voidarg);

int gpg_import_finish(int (*dump_func)(const char *, size_t, void *),
		      void *voidarg);



	     /* INTERNAL: */

pid_t gpg_fork(int *, int *, int *, const char *, char **);

#define GPGARGV_PASSPHRASE_FD(argv,i,fd,buf) \
	((argv)[(i)++]="--passphrase-fd", \
	 (argv)[(i)++]=libmail_str_size_t((fd),(buf)))

int gpg_write(const char *, size_t,
	      int (*)(const char *, size_t, void *),
	      int (*)(const char *, size_t, void *),
	      int (*)(void *),
	      unsigned,
	      void *);

int gpg_read(int (*)(const char *, size_t, void *),
	     int (*)(const char *, size_t, void *),
	     int (*)(void *),
	     unsigned,
	     void *);



struct rfc2045 *gpgmime_is_multipart_signed(const struct rfc2045 *);
	/*
	** Return ptr to signed content if ptr is a multipart/signed.
	*/

struct rfc2045 *gpgmime_is_multipart_encrypted(const struct rfc2045 *);
	/*
	** Return ptr to encrypted content if ptr is a multipart/encrypted.
	*/

int gpgmime_has_mimegpg(const struct rfc2045 *);
	/*
	** Return non-zero if MIME content has any signed or encrypted
	** content.
	*/

int gpgmime_is_decoded(const struct rfc2045 *, int *);
	/*
	** Return non-zero if this is a multipart/mixed section generated
	** by mimegpg, and return the GnuPG return code.
	*/

struct rfc2045 *gpgmime_decoded_content(const struct rfc2045 *);
	/*
	** If is_decoded, then return the ptr to the decoded content.
	** (note - if decryption failed, NULL is returned).
	*/

struct rfc2045 *gpgmime_signed_content(const struct rfc2045 *);
	/*
	** If is_multipart_signed, return ptr to the signed content.
	*/

#ifdef  __cplusplus
} ;
#endif
#endif
