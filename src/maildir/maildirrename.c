/*
** Copyright 2002-2003 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<stdlib.h>
#include	<time.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<ctype.h>
#include	<errno.h>
#include	<stdio.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#include	"maildirmisc.h"

/* Hierarchical rename */

static int chk_rename(const char *, const char *, void *);
static int do_rename(const char *, const char *, void *);

static int scan_maildir_rename(const char *, const char *, const char *,
			       int (*)(const char *, const char *, void *),
			       int, void *void_arg);

static int validrename(const char *oldname, const char *newname)
{
	if (strchr(oldname, '/') ||
	    strchr(newname, '/') ||
	    newname[0] != '.' ||
	    strcmp(newname, ".") == 0)
		return (-1);

	while (*newname)
	{
		if (*newname == '.')
		{
			if (newname[1] == '.' || newname[1] == 0)
				return -1;
		}
		++newname;
	}
	return 0;
}

int maildir_rename(const char *maildir,
		   const char *oldname, const char *newname, int flags,
		   void (*rename_func)(const char *old_path,
				       const char *new_path))
{
	if (validrename(oldname, newname))
	{
		errno=EINVAL;
		return (-1);
	}

	if (scan_maildir_rename(maildir, oldname, newname, chk_rename, flags,
				NULL))
		return (-1);

	scan_maildir_rename(maildir, oldname, newname, do_rename, flags,
			    rename_func);
	return (0);
}

static int scan_maildir_rename(const char *maildir,
			       const char *oldname,
			       const char *newname,
			       int (*func)(const char *, const char *, void *),
			       int flags,
			       void *void_arg)
{
	char *new_p;

	int oldname_l=strlen(oldname);
	DIR *dirp;
	struct dirent *de;

	dirp=opendir(maildir);
	while (dirp && (de=readdir(dirp)) != 0)
	{
		char *tst_cur;

		if (de->d_name[0] != '.')
			continue;
		if (strcmp(de->d_name, "..") == 0)
			continue;

		if ((tst_cur=malloc(strlen(maildir) + strlen(de->d_name)
				    + sizeof("//cur")))
		    == NULL)
		{
			closedir(dirp);
			return (-1);
		}
	 	strcat(strcat(strcat(strcpy(tst_cur, maildir), "/"),
			      de->d_name), "/cur");
		if (access(tst_cur, 0))
		{
			free(tst_cur);
			continue;
		}
		if (strncmp(de->d_name, oldname, oldname_l))
		{
			free(tst_cur);
			continue;
		}

		if (de->d_name[oldname_l] == 0)
		{
			if (!(flags & MAILDIR_RENAME_FOLDER))
			{
				free(tst_cur);
				continue;	/* Only the hierarchy */
			}

			strcat(strcat(strcpy(tst_cur, maildir), "/"),
			       de->d_name);

			new_p=malloc(strlen(maildir)+sizeof("/")
				     +strlen(newname));

			if (!new_p)
			{
				free(tst_cur);
				closedir(dirp);
				return (-1);
			}

			strcat(strcat(strcpy(new_p, maildir), "/"), newname);

			if ( (*func)(tst_cur, new_p, void_arg))
			{
				free(new_p);
				free(tst_cur);
				closedir(dirp);
				return (-1);
			}
			free(new_p);
			free(tst_cur);
			continue;
		}
		free(tst_cur);

	        if (de->d_name[oldname_l] != '.')
			continue;

		if (!(flags & MAILDIR_RENAME_SUBFOLDERS))
			continue;

		tst_cur=malloc(strlen(maildir) + 
			       strlen(newname) + strlen(de->d_name+oldname_l)
			       + sizeof("/"));

		if (!tst_cur)
		{
			closedir(dirp);
			return (-1);
		}

		strcat(strcat(strcat(strcpy(tst_cur, maildir),
				     "/"), newname), de->d_name+oldname_l);

		new_p=malloc(strlen(maildir) + strlen(de->d_name)
			     + sizeof("/"));

		if (!new_p)
		{
			free(tst_cur);
			closedir(dirp);
			return (-1);
		}

		strcat(strcat(strcpy(new_p, maildir), "/"),
		       de->d_name);

		if ( (*func)(new_p, tst_cur, void_arg))
		{
			free(new_p);
			free(tst_cur);
			closedir(dirp);
			return (-1);
		}
		free(new_p);
		free(tst_cur);
	}
	closedir(dirp);
	return (0);
}

static int chk_rename(const char *on, const char *nn, void *void_arg)
{
	if (access(nn, 0) == 0)
	{
		errno=EEXIST;
		return (-1);	/* Destination folder exists */
	}
	return (0);
}

static int do_rename(const char *on, const char *nn, void *void_arg)
{
	void (*void_func)(const char *, const char *)=
		(void  (*)(const char *, const char *))void_arg;

	if (void_func)
		(*void_func)(on, nn);

	return (rename(on, nn));
}
