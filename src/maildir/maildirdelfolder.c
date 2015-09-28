/*
** Copyright 2000 Double Precision, Inc.
** See COPYING for distribution information.
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
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
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<stdlib.h>
#include	<time.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<stdio.h>
#include	<ctype.h>
#include	<errno.h>

#include	"maildirmisc.h"

static int delsubdir(const char *s)
{
DIR	*dirp;
struct	dirent *de;
char	*p;

	dirp=opendir(s);
	while (dirp && (de=readdir(dirp)) != 0)
	{
		if (strcmp(de->d_name, ".") == 0 ||
			strcmp(de->d_name, "..") == 0)
			continue;

		p=malloc(strlen(s)+strlen(de->d_name)+2);
		if (!p)
		{
			if (dirp)	closedir(dirp);
			return (-1);
		}

		strcat(strcat(strcpy(p, s), "/"), de->d_name);
		unlink(p);
		free(p);
	}
	if (dirp)	closedir(dirp);
	rmdir(s);
	return (0);
}

int maildir_mddelete(const char *s)
{
char	*q;

	q=malloc(strlen(s)+sizeof("/cur"));
	if (!q)	return (-1);
	strcat(strcpy(q, s), "/cur");
	if (delsubdir(q) == 0)
	{
		strcat(strcpy(q, s), "/new");
		if (delsubdir(q) == 0)
		{
			strcat(strcpy(q, s), "/tmp");
			if ( delsubdir(q) == 0 && delsubdir(s) == 0)
			{
				free(q);
				return (0);
			}
		}
	}
	free(q);
	return (-1);
}

int maildir_deletefolder(const char *maildir, const char *folder)
{
char	*s;
int	rc;

	if (*folder == '.')
	{
		errno=EINVAL;
		return (-1);
	}

	s=malloc(strlen(maildir)+strlen(folder)+3);
	if (!s)	return (-1);
	strcat(strcat(strcpy(s, maildir), "/."), folder);

	rc=maildir_mddelete(s);
	free(s);
	return (rc);
}
