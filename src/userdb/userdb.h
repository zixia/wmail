#ifndef	userdb_h
#define	userdb_h

/*
** Copyright 1998 - 2001 Double Precision, Inc.
** See COPYING for distribution information.
*/

static const char userdb_rcsid_h[]="$Id: userdb.h,v 1.1.1.1 2003/05/07 02:13:34 lfan Exp $";

#include	<sys/types.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
	Functions to access local/config/userdb.dat
*/

void userdb_init(const char *);
void userdb_close();
char	*userdb(const char *);	/* Fetch the record */
char	*userdbshadow(const char *,
		const char *);	/* Fetch the userdbshadow record */

		/* Extract field from the record */
const char	*userdb_get(const char *,	/* The record */
			const char *,	/* Field name */
			int *);		/* Content length returned */

		/* Extract numerical field from record */

unsigned userdb_getu(const char *,	/* The record */
	const char *,			/* Field name */
	unsigned);			/* Returned if field not found */

		/* Extract string into malloced buffer */
char *userdb_gets(const char *,	/* The record */
	const char *);	/* The field */

struct userdbs {
	char *udb_name;	   /* Account name, ONLY set by userdb_createsuid */
	char *udb_gecos;	/* GECOS */
	char *udb_dir;		/* Home directory */
	char *udb_shell;	/* Shell */
	char *udb_mailbox;	/* Default mailbox */
	char *udb_quota;	/* Maildir quota */
	uid_t	udb_uid;
	gid_t	udb_gid;

	char *udb_source;	/* Non-blank - source file in userdb dir */
	} ;

struct userdbs *userdb_creates(const char *);
struct userdbs *userdb_createsuid(uid_t);
void	userdb_frees(struct userdbs *);
char *userdb_mkmd5pw(const char *);

#ifdef	__cplusplus
} ;
#endif

#endif
