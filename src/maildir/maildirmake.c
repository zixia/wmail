/*
** Copyright 1998 - 2003 Double Precision, Inc.
** See COPYING for distribution information.
*/

/* $Id: maildirmake.c,v 1.1.1.1 2003/05/07 02:14:21 lfan Exp $ */

#if	HAVE_CONFIG_H
#include	"config.h"
#endif
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#if	HAVE_FCNTL_H
#include	<fcntl.h>
#endif
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<sys/types.h>
#if	HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif
#include	<ctype.h>
#include	<errno.h>

#include	"maildircreate.h"
#include	"maildirmisc.h"
#include	"maildirsharedrc.h"
#include	"maildirquota.h"

static void usage()
{
	printf("Usage: maildirmake [ options ] maildir\n");
	exit(1);
}

extern FILE *maildir_shared_fopen(const char *, const char *);
extern void maildir_shared_fparse(char *, char **, char **);

static void add(const char *dir, const char *name)
{
char	*c=malloc(strlen(name)+1);
char	*s;
FILE	*fp;
char	buf[BUFSIZ];
char	*ptr, *dirp;

	if (!c)
	{
		perror("malloc");
		exit(1);
	}
	strcpy(c, name);

	if ((s=strchr(c, '=')) == 0)
		usage();
	*s++=0;
	if (*s != '/')
		usage();
	if (access(s, R_OK))
	{
		perror(s);
		exit(1);
	}
	if (strchr(c, '.') || strchr(c, '/'))
	{
		fprintf(stderr, "%s: invalid name\n", c);
		exit(1);
	}

	if (chdir(dir))
	{
		perror(dir);
		exit(1);
	}

	if ((fp=fopen(MAILDIRSHAREDRC, "r")) != 0)
	{
		while ((ptr=fgets(buf, sizeof(buf), fp)) != 0)
		{
		char	*namep;

			maildir_shared_fparse(buf, &namep, &dirp);
			if (!namep)	continue;
			if (strcmp(namep, c) == 0)
			{
				fclose(fp);
				fprintf(stderr,
		"%s: already defined as a sharable maildir in %s.\n",
					namep, MAILDIRSHAREDRC);
				exit(2);
			}
		}
		fclose(fp);
	}

	if ((fp=maildir_shared_fopen(".", "a+")) == 0)
	{
		perror(dir);
		exit(1);
	}

	if (fseek(fp, 0L, SEEK_SET) < 0)
	{
		perror(dir);
		exit(1);
	}

	while ((ptr=fgets(buf, sizeof(buf), fp)) != 0)
	{
	char	*namep;

		maildir_shared_fparse(buf, &namep, &dirp);
		if (!namep)	continue;
		if (strcmp(namep, c) == 0)
		{
			fclose(fp);
			fprintf(stderr, "%s: already defined as a sharable maildir.\n",
				namep);
			exit(2);
		}
	}
	fprintf(fp, "%s\t%s\n", c, s);
	if (fflush(fp) || ferror(fp) || fclose(fp))
	{
		perror("dir");
		exit(1);
	}
	exit(0);
}

static void del(const char *dir, const char *n)
{
FILE	*fp;
char	buf[BUFSIZ];
char	buf2[BUFSIZ];

FILE	*fp2;
int	found;
 struct maildir_tmpcreate_info createInfo;

	if (chdir(dir))
	{
		perror(dir);
		exit(1);
	}
	if ((fp=maildir_shared_fopen(".", "r")) == 0)
	{
		perror(dir);
		exit(1);
	}

	maildir_tmpcreate_init(&createInfo);

	createInfo.uniq="shared";
	createInfo.msgsize=0;
	createInfo.doordie=1;

	if ((fp2=maildir_tmpcreate_fp(&createInfo)) == NULL)
	{
		perror(dir);
		exit(1);
	}

	found=0;
	while (fgets(buf, sizeof(buf), fp))
	{
	char	*name, *dirp;

		strcpy(buf2, buf);
		maildir_shared_fparse(buf2, &name, &dirp);
		if (name && strcmp(name, n) == 0)
		{
			found=1;
			continue;
		}
		fprintf(fp2, "%s", buf);
	}
	fclose(fp);
	if (fflush(fp2) || ferror(fp2) || fclose(fp2)
	    || rename(createInfo.tmpname, "shared-maildirs"))
	{
		perror(createInfo.tmpname);
		unlink(createInfo.tmpname);
		exit(1);
	}
	maildir_tmpcreate_free(&createInfo);
	if (!found)
	{
		fprintf(stderr, "%s: not found.\n", n);
		exit(1);
	}
	exit(0);
}

int main(int argc, char *argv[])
{
const char *maildir, *folder=0;
char *p;
int	argn;
int	perm=0700;
int	musthavefolder=0;
int	subdirperm;
char	*addshared=0, *delshared=0;
const char *quota=0;

	for (argn=1; argn<argc; argn++)
	{
		if (argv[argn][0] != '-')	break;
		if (strcmp(argv[argn], "-") == 0)	break;
		if (strncmp(argv[argn], "-f", 2) == 0)
		{
			folder=argv[argn]+2;
			if (*folder == 0 && argn+1 < argc)
				folder=argv[++argn];
			continue;
		}
		if (strcmp(argv[argn], "-S") == 0)
		{
			perm=0755;
			continue;
		}

		if (strncmp(argv[argn], "-s", 2) == 0)
		{
		const char *p=argv[argn]+2;

			if (*p == 0 && argn+1 < argc)
				p=argv[++argn];

			perm=0755;
			for (; *p; ++p)
			{
				if (isspace((int)(unsigned char)*p) ||
					*p == ',')
					continue;
				if (*p == 'r')
					perm=0755;
				else if (*p == 'w')
					perm=0777 | S_ISVTX;
				else if (*p == 'g')
					perm &= ~0007;

				while (*p && !isspace((int)(unsigned char)*p)
					&& *p != ',')
					++p;
				if (!*p)	break;
			}
			musthavefolder=1;
			continue;
		}

		if (strncmp(argv[argn], "-q", 2) == 0)
		{
			const char *p=argv[argn]+2;

			if (*p == 0 && argn+1 < argc)
				p=argv[++argn];

			quota=p;
			continue;
		}

		if (strcmp(argv[argn], "--add") == 0 && argc-argn > 1)
		{
			addshared=argv[++argn];
			continue;
		}

		if (strcmp(argv[argn], "--del") == 0 && argc-argn > 1)
		{
			delshared=argv[++argn];
			continue;
		}

		usage();
	}

	if (argn == argc)	usage();
	maildir=argv[argn];

	if (addshared)
	{
		add(maildir, addshared);
		exit (0);
	}

	if (delshared)
	{
		del(maildir, delshared);
		exit (0);
	}

	if (folder && *folder == '.')
	{
		printf("Invalid folder name: %s\n", folder);
		exit(1);
	}

	if (folder)
	{
		if ((p=(char *)malloc(strlen(folder)+2)) == 0)
		{
			perror("maildirmake");
			exit(1);
		}
		p[0]='.';
		strcpy(p+1, folder);
		folder=p;
	}
	else	if (musthavefolder)
		usage();

	if (quota)
	{
		maildir_quota_set(maildir, quota);
		exit(0);
	}
	subdirperm=perm;
	if (!folder)	subdirperm=0700;
	umask(0);
	if ((folder && chdir(maildir)) ||
		mkdir(folder ? folder:maildir, perm & ~0022) < 0 ||
		chdir(folder ? folder:maildir) < 0 ||
		mkdir("tmp", subdirperm) < 0 ||
		chmod("tmp", subdirperm) < 0 ||
		mkdir("new", subdirperm) < 0 ||
		chmod("new", subdirperm) < 0 ||
		mkdir("cur", subdirperm) < 0 ||
		chmod("cur", subdirperm) < 0 ||
		(folder && open("maildirfolder", O_CREAT|O_WRONLY, 0600) < 0))
	{
		perror("maildirmake");
		exit(1);
	}
	exit(0);
	return (0);
}
