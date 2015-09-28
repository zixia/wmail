/*
** Copyright 1998 - 2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: maildir.c,v 1.4 2004/01/03 15:28:49 roy Exp $
*/
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<errno.h>
#include       <syslog.h>
#include	"config.h"
#include	"sqwebmail.h"
#include	"maildir.h"
#include	"folder.h"
#include	"rfc822/rfc822.h"
#include	"rfc822/rfc2047.h"
#include	"pref.h"
#include	"sqconfig.h"
#include	"dbobj.h"
#include	"maildir/maildirquota.h"
#include	"maildir/maildirrequota.h"
#include	"maildir/maildirgetquota.h"
#include	"maildir/maildirmisc.h"
#include	"maildir/maildircreate.h"

#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif

#if	HAVE_DIRENT_H
#include	<dirent.h>
#define	NAMLEN(dirent)	strlen(dirent->d_name)
#else
#define	dirent	direct
#define	NAMLEN(dirent)	((dirent)->d_namlen)
#if	HAVE_SYS_NDIR_H
#include	<sys/ndir.h>
#endif
#if	HAVE_SYS_DIR_H
#include	<sys/dir.h>
#endif
#if	HAVE_NDIR_H
#include	<ndir.h>
#endif
#endif

#include	<sys/types.h>
#include	<sys/stat.h>
#if	HAVE_UTIME_H
#include	<utime.h>
#endif

#if UTF7_FOLDER_ENCODING
#include	"unicode/unicode.h"
#endif

static time_t	current_time;

extern time_t rfc822_parsedt(const char *);
extern const char *sqwebmail_content_charset;

static char *folderdatname=0;	/* Which folder has been cached */
static struct dbobj folderdat;
static const char *folderdatmode;	/* "R" or "W" */

static time_t cachemtime;	/* Modification time of the cache file */
static unsigned long new_cnt, all_cnt;
static int isoldestfirst;
static char sortorder;

static void createmdcache(const char *);

static void maildir_getfoldermsgs(const char *);

/* Change timestamp on a file */

static const char *parse_ul(const char *p, unsigned long *ul)
{
int	err=1;

	while (p && isspace((int)(unsigned char)*p))	++p;
	*ul=0;
	while (p && *p >= '0' && *p <= '9')
	{
		*ul *= 10;
		*ul += (*p-'0');
		err=0;
		++p;
	}
	if (err)	return (0);
	return (p);
}

static void change_timestamp(const char *filename, time_t t)
{
#if	HAVE_UTIME
struct utimbuf	ut;

	ut.actime=ut.modtime=t;
	utime(filename, &ut);
#else
#if	HAVE_UTIMES
struct timeval tv;

	tv.tv_sec=t;
	tv.tv_usec=0;
	utimes(filename, &tv);
#else
#error	You do not have utime or utimes function.  Upgrade your operating system.
#endif
#endif
}

/* Translate folder name into directory name */

// by lfan, remove static
char *xlate_mdir(const char *foldername)
{
char	*p;

	if (strchr(foldername, ':'))	enomem();

	p=maildir_folderdir(0, foldername);

	if (!p)	enomem();
	return (p);
}

static char *xlate_shmdir(const char *foldername)
{
	if (*foldername == ':')
	{
	char	*p;

		p=maildir_shareddir(0, foldername+1);
		if (!p)	enomem();
		return (p);
	}
	return (xlate_mdir(foldername));
}

/* Display message size in meaningfull form */

static void cat_n(char *buf, unsigned long n)
{
char	bb[MAXLONGSIZE+1];
char	*p=bb+sizeof(bb)-1;

	*p=0;
	do
	{
		*--p = "0123456789"[n % 10];
		n=n/10;
	} while (n);
	strcat(buf, p);
}

const char *showsize(unsigned long n)
{
static char	sizebuf[MAXLONGSIZE+10];

	/* If size is less than 1K bytes, display it as 0.xK */

	if (n < 1024)
	{
		strcpy(sizebuf, "0.");
		cat_n(sizebuf, (int)(10 * n / 1024 ));
		strcat(sizebuf, "K");
	}
	/* If size is less than 1 meg, display is as xK */

	else if (n < 1024 * 1024)
	{
		*sizebuf=0;
		cat_n(sizebuf, (unsigned long)(n+512)/1024);
		strcat(sizebuf, "K");
	}

	/* Otherwise, display in megabytes */

	else
	{
	unsigned long nm=(double)n / (1024.0 * 1024.0) * 10;

		*sizebuf=0;
		cat_n( sizebuf, nm / 10);
		strcat(sizebuf, ".");
		cat_n( sizebuf, nm % 10);
		strcat(sizebuf, "M");
	}
	return (sizebuf);
}

/* Put together a filename from up to three parts */

char *alloc_filename(const char *dir1, const char *dir2, const char *filename)
{
char	*p;

	if (!dir1)	dir1="";
	if (!dir2)	dir2="";

	p=malloc(strlen(dir1)+strlen(dir2)+strlen(filename)+3);

	if (!p)	enomem();

	strcpy(p, dir1);
	if (*dir2)
	{
		if (*p)	strcat(p, "/");
		strcat(p, dir2);
	}
	if (*filename)
	{
		if (*p)	strcat(p, "/");
		strcat(p, filename);
	}

	return (p);
}

/*
** char *maildir_find(const char *maildir, const char *filename)
**	- find a message in a maildir
**
** Return the full path to the indicated message.  If the message flags
** in filename have changed, we search for the given message.
*/

char *maildir_find(const char *folder, const char *filename)
{
char	*p;
char	*d=xlate_shmdir(folder);
int	fd;

	if (!d)	return (0);
	p=maildir_filename(d, 0, filename);
	free(d);

	if (!p)	enomem();

	if ((fd=open(p, O_RDONLY)) >= 0)
	{
		close(fd);
		return (p);
	}
	free(p);
	return (0);
}

/*
** char *maildir_basename(const char *filename)
**
** - return base name of the file (strip off cur or new, strip of trailing :)
*/

char *maildir_basename(const char *filename)
{
const char *q=strrchr(filename, '/');
char	*p, *r;

	if (q)	++q;
	else	q=filename;
	p=alloc_filename("", "", q);
	if ((r=strchr(p, ':')) != 0)	*r='\0';
	return (p);
}

/* Display message creation time.  If less than one week old (more or less)
** show day of the week, and time of day, otherwise show day, month, year
*/

static char *displaydate(time_t t)
{
struct tm *tmp=localtime(&t);
static char	datebuf[40];
const char *date_yfmt;
const char *date_wfmt;

                      
	date_yfmt = getarg ("DSPFMT_YDATE");
	if (*date_yfmt == 0)
		date_yfmt = "%d %b %Y";

	date_wfmt = getarg ("DSPFMT_WDATE");
	if (*date_wfmt == 0)
		date_wfmt = "%a %I:%M %p";

	datebuf[0]='\0';
	if (tmp)
	{
		strftime(datebuf, sizeof(datebuf)-1,
			(t < current_time - 6 * 24 * 60 * 60 ||
			t > current_time + 12 * 60 * 60
			? date_yfmt : date_wfmt), tmp);
		datebuf[sizeof(datebuf)-1]=0;
	}
	return (datebuf);
}

/*
** Add a flag to a maildir filename
*/

static char *maildir_addflagfilename(const char *filename, char flag)
{
char	*new_filename=malloc(strlen(filename)+5);
		/* We can possibly add as many as four character */
char	*p;
char	*q;

	strcpy(new_filename, filename);
	p=strrchr(new_filename, '/');
	if (!p)	p=new_filename;
	if ((q=strchr(p, ':')) == 0)
		strcat(new_filename, ":2,");
	else if (q[1] != '2' && q[2] != ',')
		strcpy(p, ":2,");
	p=strchr(p, ':');
	if (strchr(p, flag))
	{
		free(new_filename);
		return (0);		/* Already set */
	}

	p += 2;
	while (*p && *p < flag)	p++;
	q=p+strlen(p);
	while ((q[1]=*q) != *p)
		--q;
	*p=flag;
	return (new_filename);
}

static void closedb()
{
	if (folderdatname)
	{
		dbobj_close(&folderdat);
		free(folderdatname);
		folderdatname=0;
	}
}

static int opencache(const char *folder, const char *mode)
{
char	*maildir=xlate_shmdir(folder);
char	*cachename;
size_t	l;
char	*p;
char	*q;
char	*r;
unsigned long ul;

	if (!maildir)	return (-1);

	cachename=alloc_filename(maildir, "", MAILDIRCURCACHE "." DBNAME);

	if (folderdatname && strcmp(folderdatname, cachename) == 0)
	{
		if (strcmp(mode, "W") == 0 &&
			strcmp(folderdatmode, "W"))
			;
			/*
			** We want to open for write, folder is open for
			** read
			*/
		else
		{
			free(cachename);
			return (0);
				/* We already have this folder cache open */
		}
	}
	closedb();
	folderdatmode=mode;

	dbobj_init(&folderdat);
	if (dbobj_open(&folderdat, cachename, mode))	return (-1);
	folderdatname=cachename;

	if ((p=dbobj_fetch(&folderdat, "HEADER", 6, &l, "")) == 0)
		return (0);
	q=malloc(l+1);
	if (!q)	enomem();
	memcpy(q, p, l);
	q[l]=0;
	free(p);

	cachemtime=0;
	new_cnt=0;
	all_cnt=0;
	isoldestfirst=0;
	sortorder=0;

	for (p=q; (p=strtok(p, "\n")) != 0; p=0)
	{
		if ((r=strchr(p, '=')) == 0)	continue;
		*r++=0;
		if (strcmp(p, "SAVETIME") == 0 &&
			parse_ul(r, &ul))
			cachemtime=ul;
		else if (strcmp(p, "COUNT") == 0)
			parse_ul(r, &all_cnt);
		else if (strcmp(p, "NEWCOUNT") == 0)
			parse_ul(r, &new_cnt);
		else if (strcmp(p, "SORT") == 0)
		{
		unsigned long ul;
		const char *s;

			if ((s=parse_ul(r, &ul)) != 0)
			{
				isoldestfirst=ul;
				sortorder= *s;
			}
		}
	}
	free(q);
	return (0);
}

/* And, remove a flag */

static void maildir_remflagname(char *filename, char flag)
{
char	*p;

	p=strrchr(filename, '/');
	if (!p)	p=filename;
	if ((p=strchr(p, ':')) == 0)	return;
	else if (p[1] != '2' && p[2] != ',')
		return;

	p=strchr(p, ':');
	p += 3;

	while (*p && isalpha((int)(unsigned char)*p))
	{
		if (*p == flag)
		{
			while ( (*p=p[1]) != 0)
				p++;
			return;
		}
		p++;
	}
}

static MSGINFO *get_msginfo(unsigned long n)
{
char	namebuf[MAXLONGSIZE+40];
char	*p;
size_t	len;
unsigned long ul;

static	char *buf=0;
size_t	bufsize=0;
static	MSGINFO msginfo_buf;

	sprintf(namebuf, "REC%lu", n);

	p=dbobj_fetch(&folderdat, namebuf, strlen(namebuf), &len, "");
	if (!p)	return (0);

	if (!buf || len > bufsize)
	{
		buf= buf ? realloc(buf, len+1):malloc(len+1);
		if (!buf)	enomem();
		bufsize=len;
	}
	memcpy(buf, p, len);
	buf[len]=0;
	free(p);

	memset(&msginfo_buf, 0, sizeof(msginfo_buf));
	msginfo_buf.filename= msginfo_buf.date_s= msginfo_buf.from_s=
	msginfo_buf.subject_s= msginfo_buf.size_s="";

	for (p=buf; (p=strtok(p, "\n")) != 0; p=0)
	{
	char	*q=strchr(p, '=');

		if (!q)	continue;
		*q++=0;

		if (strcmp(p, "FILENAME") == 0)
			msginfo_buf.filename=q;
		else if (strcmp(p, "FROM") == 0)
			msginfo_buf.from_s=q;
		else if (strcmp(p, "SUBJECT") == 0)
			msginfo_buf.subject_s=q;
		else if (strcmp(p, "SIZES") == 0)
			msginfo_buf.size_s=q;
		else if (strcmp(p, "DATE") == 0 &&
			parse_ul(q, &ul))
		{
			msginfo_buf.date_n=ul;
			msginfo_buf.date_s=displaydate(msginfo_buf.date_n);
		}
		else if (strcmp(p, "SIZEN") == 0 &&
			parse_ul(q, &ul))
			msginfo_buf.size_n=ul;
		else if (strcmp(p, "TIME") == 0 &&
			parse_ul(q, &ul))
			msginfo_buf.mi_mtime=ul;
		else if (strcmp(p, "INODE") == 0 &&
			parse_ul(q, &ul))
			msginfo_buf.mi_ino=ul;
	}
	return (&msginfo_buf);
}

static MSGINFO *get_msginfo_alloc(unsigned long n)
{
MSGINFO	*msginfop=get_msginfo(n);
MSGINFO *p;

	if (!msginfop)	return (0);

	if ((p= (MSGINFO *) malloc(sizeof(*p))) == 0)
		enomem();

	memset(p, 0, sizeof(*p));
	if ((p->filename=strdup(msginfop->filename)) == 0 ||
		(p->date_s=strdup(msginfop->date_s)) == 0 ||
		(p->from_s=strdup(msginfop->from_s)) == 0 ||
		(p->subject_s=strdup(msginfop->subject_s)) == 0 ||
		(p->size_s=strdup(msginfop->size_s)) == 0)
		enomem();
	p->date_n=msginfop->date_n;
	p->size_n=msginfop->size_n;
	p->mi_mtime=msginfop->mi_mtime;
	p->mi_ino=msginfop->mi_ino;
	return (p);
}

static void put_msginfo(MSGINFO *m, unsigned long n)
{
char	namebuf[MAXLONGSIZE+40];
char	*rec;

	sprintf(namebuf, "REC%lu", n);

	rec=malloc(strlen(m->filename)+strlen(m->from_s)+
		strlen(m->subject_s)+strlen(m->size_s)+MAXLONGSIZE*4+
		sizeof("FILENAME=\nFROM=\nSUBJECT=\nSIZES=\nDATE=\n"
			"SIZEN=\nTIME=\nINODE=\n")+100);
	if (!rec)	enomem();

	sprintf(rec, "FILENAME=%s\nFROM=%s\nSUBJECT=%s\nSIZES=%s\n"
		"DATE=%lu\n"
		"SIZEN=%lu\n"
		"TIME=%lu\n"
		"INODE=%lu\n",
		m->filename,
		m->from_s,
		m->subject_s,
		m->size_s,
		(unsigned long)m->date_n,
		(unsigned long)m->size_n,
		(unsigned long)m->mi_mtime,
		(unsigned long)m->mi_ino);

	if (dbobj_store(&folderdat, namebuf, strlen(namebuf),
		rec, strlen(rec), "R"))
		enomem();
	free(rec);
}

static void update_foldermsgs(const char *folder, const char *newname, size_t pos)
{
MSGINFO	*p;
char *n;

	n=strrchr(newname, '/')+1;
	if (opencache(folder, "W") || (p=get_msginfo(pos)) == 0)
	{
		error("Internal error in update_foldermsgs");
		return;
	}

	p->filename=n;

	put_msginfo(p, pos);
}

static void maildir_markflag(const char *folder, size_t pos, char flag)
{
MSGINFO	*p;
char	*filename;
char	*new_filename;

	if (opencache(folder, "W") || (p=get_msginfo(pos)) == 0)
	{
		error("Internal error in maildir_markflag");
		return;
	}

	filename=maildir_find(folder, p->filename);
	if (!filename)	return;

	if ((new_filename=maildir_addflagfilename(filename, flag)) != 0)
	{
		rename(filename, new_filename);
		update_foldermsgs(folder, new_filename, pos);
		free(new_filename);
	}
	free(filename);
}

void maildir_markread(const char *folder, size_t pos)
{
	maildir_markflag(folder, pos, 'S');
}

void maildir_markreplied(const char *folder, const char *message)
{
char	*filename;
char	*new_filename;

	filename=maildir_find(folder, message);

	if (filename &&
		(new_filename=maildir_addflagfilename(filename, 'R')) != 0)
	{
		rename(filename, new_filename);
		free(new_filename);
	}
	if (filename)	free(filename);
}

char *maildir_posfind(const char *folder, size_t *pos)
{
MSGINFO	*p;
char	*filename;

	if (opencache(folder, "R") || (p=get_msginfo( *pos)) == 0)
	{
		error("Internal error in maildir_posfind");
		return (0);
	}

	filename=maildir_find(folder, p->filename);
	return(filename);
}


int maildir_name2pos(const char *folder, const char *filename, size_t *pos)
{
	char *p, *q;
	size_t len;

	maildir_reload(folder);
	if (opencache(folder, "R"))
	{
		error("Internal error in maildir_name2pos");
		return (0);
	}

	p=malloc(strlen(filename)+10);
	if (!p)
		enomem();
	strcat(strcpy(p, "FILE"), filename);
	q=strchr(p, ':');
	if (q)
		*q=0;

	q=dbobj_fetch(&folderdat, p, strlen(p), &len, "");
	free(p);

	if (!q)
		return (-1);

	*pos=0;
	for (p=q; len; --len, p++)
	{
		if (isdigit((int)(unsigned char)*p))
			*pos = *pos * 10 + (*p-'0');
	}
	free(q);
	return (0);
}

void maildir_msgpurge(const char *folder, size_t pos)
{
char *filename=maildir_posfind(folder, &pos);

	if (filename)
	{
		unlink(filename);
		free(filename);
	}
}

void maildir_msgpurgefile(const char *folder, const char *msgid)
{
char *filename=maildir_find(folder, msgid);

	if (filename)
	{
		char *d=xlate_shmdir(folder);

		if (d)
		{
			if (*folder != ':' && maildirquota_countfolder(d) &&
			    maildirquota_countfile(filename))
			{
				unsigned long filesize=0;

				if (maildir_parsequota(filename, &filesize))
				{
					struct stat stat_buf;

					if (stat(filename, &stat_buf) == 0)
						filesize=stat_buf.st_size;
				}

				if (filesize > 0)
					maildir_quota_deleted(".",
							      -(long)filesize,
							      -1);
			}
			free(d);
		}
		unlink(filename);
		free(filename);
	}
}

/*
** A message is moved to a different folder as follows.
** The message is linked to the destination folder, then marked with a 'T'
** flag in the original folder.  Later, all T-marked messages are deleted.
*/

static int msgcopy(int fromfd, int tofd)
		/* ... Except moving to/from sharable folder actually
		** involves copying.
		*/
{
char	buf[8192];
int	i, j;
char	*p;

	while ((i=read(fromfd, buf, sizeof(buf))) > 0)
	{
		p=buf;
		while (i)
		{
			j=write(tofd, p, i);
			if (j <= 0)	return (-1);
			p += j;
			i -= j;
		}
	}
	return (i);
}

static int do_msgmove(const char *from,
		const char *file, const char *dest, size_t pos)
{
char *destdir, *fromdir;
const char *p;
char *basename;
struct stat stat_buf;
char	*new_filename;
unsigned long	filesize=0;
int	no_link=0;
struct maildirsize quotainfo;
 char *newname;

/* modified by roy 2003.9.29 */
//	if (stat(file, &stat_buf) || stat_buf.st_nlink != 1)
//		return (0);	/* Already moved */

	/* Update quota */

	destdir=xlate_shmdir(dest);
	if (!destdir)	enomem();

	fromdir=xlate_shmdir(from);
	if (!fromdir)	enomem();

	if (strcmp(from, dest))
	{
		if (maildir_parsequota(file, &filesize))
			filesize=stat_buf.st_size;
			/* Recover from possible corruption */

		if ((*dest == ':' || !maildirquota_countfolder(destdir)) &&
		    maildirquota_countfile(file))
		{
			if (*from != ':')
				maildir_quota_deleted(".", -filesize, -1);
		}
		else if (*dest != ':' && maildirquota_countfolder(destdir) &&
			 (*from == ':' || !maildirquota_countfolder(fromdir))
			 )
			/* Moving FROM trash */
		{

			if (maildir_quota_add_start(".", &quotainfo,
						    filesize, 1, NULL))
			{
				free(fromdir);
				return (-1);
			}

			maildir_quota_add_end(&quotainfo, filesize, 1);
		}
	}

	free(fromdir);

	if (*dest == ':' || *from == ':')
	{
		struct maildir_tmpcreate_info createInfo;
		int	fromfd, tofd;
		char	*l;

		if (*dest == ':')	/* Copy to the sharable folder */
		{
		char	*p=malloc(strlen(destdir)+sizeof("/shared"));

			if (!p)
			{
				free(destdir);
				enomem();
			}
			strcat(strcpy(p, destdir), "/shared");
			free(destdir);
			destdir=p;
		}

		maildir_tmpcreate_init(&createInfo);

		createInfo.maildir=destdir;
		createInfo.uniq="copy";
		createInfo.doordie=1;

		if ( *dest == ':' )
			umask (0022);

		if ((tofd=maildir_tmpcreate_fd(&createInfo)) < 0)
		{
			free(destdir);
			error(strerror(errno));
		}

		if (*dest == ':')
		/* We need to copy it directly into /cur of the dest folder */
		{
			memcpy(strrchr(createInfo.newname, '/')-3, "cur", 3);
						/* HACK!!!!!!!!!!!! */
		}

		if ((fromfd=maildir_semisafeopen(file, O_RDONLY, 0)) < 0)
		{
			free(destdir);
			close(tofd);
			unlink(createInfo.tmpname);
			maildir_tmpcreate_free(&createInfo);
			enomem();
		}

		umask (0077);
		if (msgcopy(fromfd, tofd))
		{
			close(fromfd);
			close(tofd);
			free(destdir);
			unlink(createInfo.tmpname);
			maildir_tmpcreate_free(&createInfo);
			enomem();
		}
		close(fromfd);
		close(tofd);

	/*
	** When we attempt to DELETE a message in the sharable folder,
	** attempt to remove the UNDERLYING message
	*/

		if (*from == ':' && (l=maildir_getlink(file)) != 0)
		{
			if (unlink(l))
			{
				/* Not our message */

				if (strcmp(dest, TRASH) == 0)
				{
					free(l);
					unlink(createInfo.tmpname);
					maildir_tmpcreate_free(&createInfo);
					free(destdir);
					return (0);
				}
			}
			free(l);
		}

		if (maildir_movetmpnew(createInfo.tmpname, createInfo.newname))
		{
			free(destdir);
			unlink(createInfo.tmpname);
			maildir_tmpcreate_free(&createInfo);
			error(strerror(errno));
		}
		no_link=1;	/* Don't call link(), below */
		maildir_tmpcreate_free(&createInfo);
	}

	p=strrchr(file, '/');
	if (p)	++p;
	else	p=file;

	if ( (basename=strdup(p)) == NULL)
		enomem();
	maildir_remflagname(basename, 'T');	/* Remove any deleted flag for new name */
	newname=alloc_filename(destdir, "cur", basename);
	free(destdir);
	free(basename);

	/* When DELETE is called for a message in TRASH, from and dest will
	** be the same, so we just mark the file as Trashed, to be removed
	** in checknew.
	*/

	if (no_link == 0 && strcmp(from, dest))
	{
		if (link(file, newname))
		{
			free(newname);
			return (-1);
		}
	}
	free(newname);

	if ((new_filename=maildir_addflagfilename(file, 'T')) != 0)
	{
		rename(file, new_filename);
		update_foldermsgs(from, new_filename, pos);
		free(new_filename);
	}
	return (0);
}

int maildir_msgmove(const char *folder, size_t pos, const char *dest)
{
char *filename=maildir_posfind(folder, &pos);
int	rc;

	if (!filename)	return (0);
	rc=do_msgmove(folder, filename, dest, pos);
	free(filename);
	return (rc);
}

void maildir_msgdeletefile(const char *folder, const char *file, size_t pos)
{
char *filename=maildir_find(folder, file);

	if (filename)
	{
		(void)do_msgmove(folder, filename, TRASH, pos);
		free(filename);
	}
}

int maildir_msgmovefile(const char *folder, const char *file, const char *dest,
	size_t pos)
{
char *filename=maildir_find(folder, file);
int	rc;

	if (!filename)	return (0);
	rc=do_msgmove(folder, filename, dest, pos);
	free(filename);
	return (rc);
}

// detecting attachment, by lfan
int maildir_checkatt(char *fname)
{
	FILE *fp;
	char buf[1024];
	fp = fopen( fname, "r" );
	
	if( fp != NULL )
	{
		while( !feof(fp) ) {
			fgets( buf, 1024, fp );
			if( strncmp( buf, "Content-Disposition: attachment;", 32 ) == 0 )
				return 1;
		}
		fclose(fp);
	}
	
	return 0;
}
/*
** Grab new messages from new.
*/
// by lfan
//static void maildir_checknew(const char *dir)
void maildir_checknew(const char *dir)
{
char	*dirbuf;
struct	stat	stat_buf;
DIR	*dirp;
struct	dirent	*dire;
time_t	cntmtime;

	/* Delete old files in tmp */

	maildir_purgetmp(dir);

	/* Move everything from new to cur */

	dirbuf=alloc_filename(dir, "new", "");

	for (dirp=opendir(dirbuf); dirp && (dire=readdir(dirp)) != 0; )
	{
	char	*oldname, *newname;
	char	*p;
	int	nLen;

		if (dire->d_name[0] == '.')	continue;

		oldname=alloc_filename(dirbuf, dire->d_name, "");
		// by lfan
		if( maildir_checkatt( oldname ) )
			nLen = 1;
		else
			nLen = 0;

		newname=malloc(strlen(oldname)+4+nLen);
		if (!newname)	enomem();

		strcat(strcat(strcpy(newname, dir), "/cur/"), dire->d_name);
		p=strrchr(newname, '/');
		if ((p=strchr(p, ':')) != NULL)	*p=0;	/* Someone screwed up */
		strcat(newname, ":2,");

		// by lfan, adding Attachment flag
		if( nLen ) strcat(newname, "A");

		rename(oldname, newname);
		free(oldname);
		free(newname);
	}
	if (dirp)	closedir(dirp);
	free(dirbuf);

	/* Look for any messages mark as deleted.  When we delete a message
	** we link it into the Trash folder, and mark the original with a T,
	** which we delete when we check for new messages.
	*/

	dirbuf=alloc_filename(dir, "cur", "");
	if (stat(dirbuf, &stat_buf))
	{
		free(dirbuf);
		return;
	}

	/* If the count cache file is still current, the directory hasn't
	** changed, so we don't need to scan it for deleted messages.  When
	** a message is deleted, the rename bumps up the timestamp.
	**
	** This depends on dodirscan() being called after this function,
	** which updates MAILDIRCOUNTCACHE
	*/

	if (read_sqconfig(dir, MAILDIRCOUNTCACHE, &cntmtime) != 0 &&
		cntmtime > stat_buf.st_mtime)
	{
		free(dirbuf);
		return;
	}

	for (dirp=opendir(dirbuf); dirp && (dire=readdir(dirp)) != 0; )
	{
	char	*p;

		if (dire->d_name[0] == '.')	continue;

		if (maildirfile_type(dire->d_name) == MSGTYPE_DELETED)
		{
			p=alloc_filename(dirbuf, "", dire->d_name);


			/*
			** Because of the funky way we do things,
			** if we were compiled with --enable-trashquota,
			** purging files from Trash should decrease the
			** quota
			*/

			if (strcmp(dir, "." TRASH) == 0 &&
			    maildirquota_countfolder(dir))
			{
				struct stat stat_buf;
				unsigned long filesize=0;

				if (maildir_parsequota(dire->d_name,
						       &filesize))
					if (stat(p, &stat_buf) == 0)
						filesize=stat_buf.st_size;

				if (filesize > 0)
					maildir_quota_deleted(".",
							      -(long)filesize,
							      -1);
			}

			maildir_unlinksharedmsg(p);
				/* Does The Right Thing if this is a shared
				** folder
				*/
				
			free(p);
		}
	}
	if (dirp)	closedir(dirp);
	free(dirbuf);
}

/*
** Automatically purge deleted messages.
*/

void maildir_autopurge()
{
char	*dir;
char	*dirbuf;
struct	stat	stat_buf;
DIR	*dirp;
struct	dirent	*dire;
char	*filename;

	/* This is called when logging in.  Version 0.18 supports maildir
	** quotas, so automatically upgrade all folders.
	*/

	for (dirp=opendir("."); dirp && (dire=readdir(dirp)) != 0; )
	{
		if (dire->d_name[0] != '.')	continue;
		if (strcmp(dire->d_name, ".") == 0 ||
			strcmp(dire->d_name, "..") == 0)	continue;
		filename=alloc_filename(dire->d_name, "maildirfolder", "");
		if (!filename)	enomem();
		close(open(filename, O_RDWR|O_CREAT, 0644));
		free(filename);

		/* Eliminate plain text cache starting with version 0.24 */

		filename=alloc_filename(dire->d_name, MAILDIRCURCACHE, "");
		if (!filename)	enomem();
		unlink(filename);
		free(filename);
	}

	/* Version 0.24 top level remove */

	unlink(MAILDIRCURCACHE);

	if (dirp)
		closedir(dirp);

	dir=xlate_mdir(TRASH);

	/* Delete old files in tmp */

	time(&current_time);
	dirbuf=alloc_filename(dir, "cur", "");
	free(dir);

	for (dirp=opendir(dirbuf); dirp && (dire=readdir(dirp)) != 0; )
	{
		if (dire->d_name[0] == '.')	continue;
		filename=alloc_filename(dirbuf, dire->d_name, "");
		if (stat(filename, &stat_buf) == 0 &&
			stat_buf.st_ctime < current_time
					- pref_autopurge * 24 * 60 * 60)
		{
			if (maildirquota_countfolder(dirbuf) &&
			    maildirquota_countfile(filename))
			{
				unsigned long filesize=0;

				if (maildir_parsequota(filename, &filesize))
					filesize=stat_buf.st_size;

				if (filesize > 0)
					maildir_quota_deleted(".",
							      -(long)filesize,
							      -1);
			}

			unlink(filename);
		}

		free(filename);
	}
	if (dirp)	closedir(dirp);
	free(dirbuf);

	maildir_purgemimegpg();
}

/*
** MIME-GPG decoding creates a temporary file in tmp, which is preserved
** (in the event of subsequent multipart/related accesses).  The filenames
** include ':', to mark them as used for this purpose.  Rather than wait
** 36 hours for them to get cleaned up, as part of a normal maildir tmp
** purge, we can blow them off right now.
*/

void maildir_purgemimegpg()
{
	DIR	*dirp;
	struct	dirent	*dire;
	char *p;

	for (dirp=opendir("tmp"); dirp && (dire=readdir(dirp)) != 0; )
	{
		if (strstr(dire->d_name, ":mimegpg:") == 0 &&
		    strstr(dire->d_name, ":calendar:") == 0)	continue;

		p=malloc(sizeof("tmp/")+strlen(dire->d_name));

		if (p)
		{
			strcat(strcpy(p, "tmp/"), dire->d_name);
			unlink(p);
			free(p);
		}
	}

	if (dirp)
		closedir(dirp);
}

static int subjectcmp(const char *a, const char *b)
{
int	aisre;
int	bisre;
int	n;
char	*as;
char	*bs;

	as=rfc822_coresubj(a, &aisre);
	bs=rfc822_coresubj(b, &bisre);

	if (as)
	{
	char *p=rfc2047_decode_simple(as);

		free(as);
		as=p;
	}

	if (bs)
	{
	char *p=rfc2047_decode_simple(bs);

		free(bs);
		bs=p;
	}

	if (!as || !bs)
	{
		if (as)	free(as);
		if (bs)	free(bs);
		enomem();
	}

	n=strcasecmp(as, bs);
	free(as);
	free(bs);

	if (aisre)
		aisre=1;	/* Just to be sure */

	if (bisre)
		bisre=1;	/* Just to be sure */

	if (n == 0)	n=aisre - bisre;
	return (n);
}

/*
** Messages supposed to be arranged in the reverse chronological order of
** arrival.
**
** Instead of stat()ing every file in the directory, we depend on the
** naming convention that are specified for the Maildir.  Therefore, we rely
** on Maildir writers observing the required naming conventions.
*/

static int messagecmp(const MSGINFO **pa, const MSGINFO **pb)
{
int	gt=1, lt=-1;
int	n;
const MSGINFO *a= *pa;
const MSGINFO *b= *pb;

	if (pref_flagisoldest1st)
	{
		gt= -1;
		lt= 1;
	}

	switch (pref_flagsortorder)	{
	case 'F':
		n=strcasecmp(a->from_s, b->from_s);
		if (n)	return (n);
		break;
	case 'S':
		n=subjectcmp(a->subject_s, b->subject_s);
		if (n)	return (n);
		break;
	}
	if (a->date_n < b->date_n)	return (gt);
	if (a->date_n > b->date_n)	return (lt);

	if (a->mi_ino < b->mi_ino)	return (gt);
	if (a->mi_ino > b->mi_ino)	return (lt);
	return (0);
}

/*
** maildirfile_type(directory, filename) - return one of the following:
**
**   MSGTYPE_NEW - new message
**   MSGTYPE_DELETED - trashed message
**   '\0' - all other kinds
*/

char maildirfile_type(const char *p)
{
const char *q=strrchr(p, '/');
int	seen_trash=0, seen_r=0, seen_s=0;

	if (q)	p=q;

	if ( !(p=strchr(p, ':')) || *++p != '2' || *++p != ',')
		return (MSGTYPE_NEW);		/* No :2,info */
				;
	++p;
	while (p && isalpha((int)(unsigned char)*p))
		switch (*p++)	{
		case 'T':
			seen_trash=1;
			break;
		case 'R':
			seen_r=1;
			break;
		case 'S':
			seen_s=1;
			break;
		}

	if (seen_trash)
		return (MSGTYPE_DELETED);	/* Trashed message */
	if (seen_s)
	{
		if (seen_r)	return (MSGTYPE_REPLIED);
		return (0);
	}

	return (MSGTYPE_NEW);
}

static int docount(const char *fn, unsigned *new_cnt, unsigned *other_cnt)
{
const char *filename=strrchr(fn, '/');
char	c;

	if (filename)	++filename;
	else		filename=fn;

	if (*filename == '.')	return (0);	/* We don't want this one */

	c=maildirfile_type(filename);

	if (c == MSGTYPE_NEW)
		++ *new_cnt;
	else
		++ *other_cnt;
	return (1);
}

MSGINFO **maildir_read(const char *dirname, unsigned nfiles,
	size_t *starting_pos,
	int *morebefore, int *moreafter)
{
MSGINFO	**msginfo;
size_t	i;

	if ((msginfo=malloc(sizeof(MSGINFO *)*nfiles)) == 0)
		enomem();
	for (i=0; i<nfiles; i++)
		msginfo[i]=0;

	if (opencache(dirname, "W"))	return (msginfo);

	// by lfan
	//if (nfiles > all_cnt)	nfiles=all_cnt;

	//if (*starting_pos + nfiles > all_cnt)
	//	*starting_pos=all_cnt-nfiles;
	if (*starting_pos > all_cnt)
		*starting_pos = 0;
	if (nfiles > all_cnt - *starting_pos)
		nfiles = all_cnt - *starting_pos;

	*morebefore = *starting_pos > 0;

	for (i=0; i<nfiles; i++)
	{
		if (*starting_pos + i >= all_cnt)	break;
		if ((msginfo[i]= get_msginfo_alloc(*starting_pos + i)) == 0)
			break;
	}
	*moreafter= *starting_pos + i < all_cnt;
	return (msginfo);
}

static void dodirscan(const char *, unsigned *, unsigned *);

void maildir_count(const char *folder,
	unsigned *new_ptr,
	unsigned *other_ptr)
{
char	*dir=xlate_shmdir(folder);

	*new_ptr=0;
	*other_ptr=0;
	if (dir)
	{
		if (*folder == ':')
		{
			maildir_shared_sync(dir);
		}
		maildir_checknew(dir);
		dodirscan(dir, new_ptr, other_ptr);
	}
	free(dir);
}

unsigned maildir_countof(const char *folder)
{
	maildir_getfoldermsgs(folder);
	return (all_cnt);
}

static void dodirscan(const char *dir, unsigned *new_cnt,
		unsigned *other_cnt)
{
DIR *dirp;
struct dirent *de;
char	*curname;
struct stat cur_stat;
time_t	cntmtime;
const	char *p;
char	cntbuf[MAXLONGSIZE*2+4];

	*new_cnt=0;
	*other_cnt=0;
	curname=alloc_filename(dir, "cur", "");

	if (stat(curname, &cur_stat))
	{
		free(curname);
		return;
	}

	if ((p=read_sqconfig(dir, MAILDIRCOUNTCACHE, &cntmtime)) != 0 &&
		cntmtime > cur_stat.st_mtime)
	{
	unsigned long n;
	unsigned long o;

		if ((p=parse_ul(p, &n)) && (p=parse_ul(p, &o)))
		{
			*new_cnt=n;
			*other_cnt=o;
			free(curname);
			return;	/* Valid cache of count */
		}
	}

	dirp=opendir(curname);
	while (dirp && (de=readdir(dirp)) != NULL)
		docount(de->d_name, new_cnt, other_cnt);
	if (dirp)	closedir(dirp);
	sprintf(cntbuf, "%u %u", *new_cnt, *other_cnt);
	write_sqconfig(dir, MAILDIRCOUNTCACHE, cntbuf);
	if ( read_sqconfig(dir, MAILDIRCOUNTCACHE, &cntmtime) == 0)
		enomem();
	if (cntmtime != cur_stat.st_mtime)
	{
		struct stat stat2;

		if (stat(curname, &stat2)
		    || stat2.st_mtime != cur_stat.st_mtime)
		{
			/* cur directory changed while we were there, punt */

			cntmtime=cur_stat.st_mtime;	/* Reset it below */
		}
	}

	if (cntmtime == cur_stat.st_mtime)	/* Potential race condition */
	{
		free(curname);
		curname=alloc_filename(dir, MAILDIRCOUNTCACHE, "");
		change_timestamp(curname, cntmtime-1);
				/* ... So rebuild it next time */
	}
	free(curname);
}

void maildir_free(MSGINFO **files, unsigned nfiles)
{
unsigned i;

	for (i=0; i<nfiles; i++)
	{
		if ( files[i] )
			maildir_nfreeinfo( files[i] );
	}
	free(files);
}

static char *buf=0;
size_t bufsize=0, buflen=0;

static void addbuf(int c)
{
	if (buflen == bufsize)
	{
	char	*newbuf= buf ? realloc(buf, bufsize+512):malloc(bufsize+512);

		if (!newbuf)	enomem();
		buf=newbuf;
		bufsize += 512;
	}
	buf[buflen++]=c;
}

char *maildir_readline(FILE *fp)
{
int	c;

	buflen=0;
	while ((c=getc(fp)) != '\n' && c >= 0)
		if (buflen < 8192)
			addbuf(c);
	if (c < 0 && buflen == 0)	return (NULL);
	addbuf(0);
	return (buf);
}

char *maildir_readheader_nolc(FILE *fp, char **value)
{
	int c;

	buflen=0;

	while ((c=getc(fp)) != EOF)
	{
		if (c != '\n')
		{
			addbuf(c);
			continue;
		}
		c=getc(fp);
		if (c >= 0) ungetc(c, fp);
		if (c < 0 || c == '\n' || !isspace(c)) break;
		addbuf('\n');
	}
	addbuf(0);

	if (c == EOF && buf[0] == 0) return (0);

	for ( *value=buf; **value; (*value)++)
	{
		if (**value == ':')
		{
			**value='\0';
			++*value;
			break;
		}
	}
	while (**value && isspace((int)(unsigned char)**value))	++*value;
	return(buf);
}

char	*maildir_readheader_mimepart(FILE *fp, char **value, int preserve_nl,
		off_t *mimepos, const off_t *endpos)
{
int	c;

	buflen=0;

	if (mimepos && *mimepos >= *endpos)	return (0);

	while (mimepos == 0 || *mimepos < *endpos)
	{
		if ((c=getc(fp)) != '\n' && c >= 0)
		{
			addbuf(c);
			if (mimepos)	++ *mimepos;
			continue;
		}
		if ( c == '\n' && mimepos)	++ *mimepos;
		if (buflen == 0)	return (0);
		if (c < 0)	break;
		c=getc(fp);
		if (c >= 0)	ungetc(c, fp);
		if (c < 0 || c == '\n' || !isspace(c))	break;
		addbuf(preserve_nl ? '\n':' ');
	}
	addbuf(0);

	for ( *value=buf; **value; (*value)++)
	{
		if (**value == ':')
		{
			**value='\0';
			++*value;
			break;
		}
		**value=tolower(**value);
	}
	while (**value && isspace((int)(unsigned char)**value))	++*value;
	return(buf);
}

char	*maildir_readheader(FILE *fp, char **value, int preserve_nl)
{
	return (maildir_readheader_mimepart(fp, value, preserve_nl, 0, 0));
}

/*****************************************************************************

The MSGINFO structure contains the summary of the headers found in all
messages in the cur directory.

Instead of opening each message every time we need to serve the directory
contents, the messages are scanned once, and a cache file is built
containing the contents.

*****************************************************************************/

/* Deallocate an individual MSGINFO structure */

void maildir_nfreeinfo(MSGINFO *mi)
{
	if (mi->filename)	free(mi->filename);
	if (mi->date_s)	free(mi->date_s);
	if (mi->from_s)	free(mi->from_s);
	if (mi->subject_s)	free(mi->subject_s);
	if (mi->size_s)	free(mi->size_s);
	free(mi);
}

/* Initialize a MSGINFO structure by reading the message headers */

MSGINFO *maildir_ngetinfo(const char *filename)
{
FILE	*fp;
MSGINFO	*mi;
struct stat stat_buf;
char	*hdr, *val;
const char *p;
int	is_sent_header=0;
char	*fromheader=0;
int	fd;

	/* Hack - see if we're reading a message from the Sent or Drafts
		folder */

	p=strrchr(filename, '/');
	if ((p && p - filename >=
		sizeof(SENT) + 5 && strncmp(p - (sizeof(SENT) + 5),
			"/." SENT "/", sizeof(SENT)+2) == 0)
		|| strncmp(filename, "." SENT "/", sizeof(SENT)+1) == 0)
		is_sent_header=1;
	if ((p && p - filename >=
		sizeof(DRAFTS) + 5 && strncmp(p-(sizeof(DRAFTS) + 5),
			"/." DRAFTS "/", sizeof(DRAFTS)+2) == 0)
		|| strncmp(filename, "." DRAFTS "/", sizeof(DRAFTS)+1) == 0)
		is_sent_header=1;

	if ((mi=(MSGINFO *)malloc(sizeof(MSGINFO))) == 0)
		enomem();

	memset(mi, '\0', sizeof(*mi));

	fp=0;
	fd=maildir_semisafeopen(filename, O_RDONLY, 0);
	if (fd >= 0)
		if ((fp=fdopen(fd, "r")) == 0)
			close(fd);

	if (fp == NULL)
	{
		free(mi);
		return (NULL);
	}

	/* mi->filename shall be the base filename, normalized as :2, */

	if ((p=strrchr(filename, '/')) != NULL)
		p++;
	else	p=filename;

	if (!(mi->filename=strdup(p)))
		enomem();

	if (fstat(fileno(fp), &stat_buf) == 0)
	{
		mi->mi_mtime=stat_buf.st_mtime;
		mi->mi_ino=stat_buf.st_ino;
		mi->size_n=stat_buf.st_size;
		mi->size_s=strdup( showsize(stat_buf.st_size));
		mi->date_n=mi->mi_mtime;	/* Default if no Date: */
		if (!mi->size_s)	enomem();
	}
	else
	{
		free(mi->filename);
		fclose(fp);
		free(mi);
		return (0);
	}


	while ((hdr=maildir_readheader(fp, &val, 0)) != 0)
	{
		if (strcmp(hdr, "subject") == 0)
		{
			if (mi->subject_s)	free(mi->subject_s);

			mi->subject_s=strdup(val);
			if (!mi->subject_s)	enomem();
		}

		if (strcmp(hdr, "date") == 0 && mi->date_s == 0)
		{
		time_t	t=rfc822_parsedt(val);

			if (t)
			{
				mi->date_n=t;
				mi->date_s=strdup(displaydate(mi->date_n));
				if (!mi->date_s)	enomem();
			}
		}

		if ((is_sent_header ?
			strcmp(hdr, "to") == 0 || strcmp(hdr, "cc") == 0:
			strcmp(hdr, "from") == 0) && fromheader == 0)
		{
		struct rfc822t *from_addr;
		struct rfc822a *from_addra;
		char	*p;
		int dotflag=0;

			from_addr=rfc822t_alloc_new(val, NULL, NULL);
			if (!from_addr)	enomem();
			from_addra=rfc822a_alloc(from_addr);
			if (!from_addra)	enomem();

			if (from_addra->naddrs > 1)
				dotflag=1;

			if (from_addra->naddrs > 0)
				p=rfc822_getname(from_addra, 0);
			else
				p=val;

			if (fromheader)	free(fromheader);
			if ((fromheader=malloc(strlen(p)+7)) == 0)
				enomem();
			strcpy(fromheader, p);
			if (dotflag)
				strcat(fromheader, "...");

			rfc822a_free(from_addra);
			rfc822t_free(from_addr);
		}

		if (mi->date_s && fromheader && mi->subject_s)
			break;
	}
	fclose(fp);

	mi->from_s=fromheader;
	if (!mi->date_s)
		mi->date_s=strdup(displaydate(mi->date_n));
	if (!mi->date_s)	enomem();
	if (!mi->from_s && !(mi->from_s=strdup("")))	enomem();
	if (!mi->subject_s && !(mi->subject_s=strdup("")))	enomem();
	return (mi);
}

/************************************************************************/

/* Save cache file */

static unsigned long save_cnt, savenew_cnt;
static time_t	save_time;

static char	*save_dbname;
static char	*save_tmpdbname;
struct dbobj	tmpdb;

static void maildir_save_start(const char *maildir, time_t t)
{
	int fd;
	struct maildir_tmpcreate_info createInfo;

	save_dbname=alloc_filename(maildir, "", MAILDIRCURCACHE "." DBNAME);

	save_time=t;
#if 1
	{
	  int f = -1;
	  char *tmpfname = alloc_filename(maildir,
					   "", MAILDIRCURCACHE ".nfshack");
	  if (tmpfname) {
	    f = open(tmpfname, O_CREAT|O_WRONLY, 0600);
	    free(tmpfname);
	  }
	  if (f != -1) {
	    struct stat s;
	    write(f, ".", 1);
	    fsync(f);
	    if (fstat(f, &s) == 0)
	      save_time = s.st_mtime;
	    close(f);
	    unlink(tmpfname);
	  }
	}
#endif

	maildir_tmpcreate_init(&createInfo);
	createInfo.maildir=maildir;
	createInfo.uniq="sqwebmail-db";
	createInfo.doordie=1;

	if ((fd=maildir_tmpcreate_fd(&createInfo)) < 0)
	{
		syslog(LOG_ERR, "Can't create cache file %s: %s\n",
		       maildir, strerror(errno));
		error(strerror(errno));
	}
	close(fd);

	save_tmpdbname=createInfo.tmpname;
	createInfo.tmpname=NULL;
	maildir_tmpcreate_free(&createInfo);

	dbobj_init(&tmpdb);

       if (dbobj_open(&tmpdb, save_tmpdbname, "N")) {
               syslog(LOG_ERR, "Can't create cache file |%s|: %s\n", save_tmpdbname, strerror(errno));
		error("Can't create cache file.");
       }

	save_cnt=0;
	savenew_cnt=0;
}

static void maildir_saveinfo(MSGINFO *m)
{
char	*rec, *p;
char	recnamebuf[MAXLONGSIZE+40];

	rec=malloc(strlen(m->filename)+strlen(m->from_s)+
		strlen(m->subject_s)+strlen(m->size_s)+MAXLONGSIZE*4+
		sizeof("FILENAME=\nFROM=\nSUBJECT=\nSIZES=\nDATE=\n"
			"SIZEN=\nTIME=\nINODE=\n")+100);
	if (!rec)	enomem();

	sprintf(rec, "FILENAME=%s\nFROM=%s\nSUBJECT=%s\nSIZES=%s\n"
		"DATE=%lu\n"
		"SIZEN=%lu\n"
		"TIME=%lu\n"
		"INODE=%lu\n",
		m->filename,
		m->from_s,
		m->subject_s,
		m->size_s,
		(unsigned long)m->date_n,
		(unsigned long)m->size_n,
		(unsigned long)m->mi_mtime,
		(unsigned long)m->mi_ino);
	sprintf(recnamebuf, "REC%lu", (unsigned long)save_cnt);
	if (dbobj_store(&tmpdb, recnamebuf, strlen(recnamebuf),
		rec, strlen(rec), "R"))
		enomem();
	free(rec);

	/* Reverse lookup */
	rec=malloc(strlen(m->filename)+10);
	if (!rec)
		enomem();
	strcat(strcpy(rec, "FILE"), m->filename);
	if ((p=strchr(rec, ':')) != 0)
		*p=0;
	sprintf(recnamebuf, "%lu", (unsigned long)save_cnt);
	if (dbobj_store(&tmpdb, rec, strlen(rec),
			recnamebuf, strlen(recnamebuf), "R"))
		enomem();

	save_cnt++;
	if (maildirfile_type(m->filename) == MSGTYPE_NEW)
		savenew_cnt++;
}

static void maildir_save_end(const char *maildir)
{
char	*curname;
char	*rec;

	curname=alloc_filename(maildir, "", "cur");

	rec=malloc(MAXLONGSIZE*4+sizeof(
			"SAVETIME=\n"
			"COUNT=\n"
			"NEWCOUNT=\n"
			"SORT=\n")+100);

	if (!rec)	enomem();
	sprintf(rec,
		"SAVETIME=%lu\nCOUNT=%lu\nNEWCOUNT=%lu\nSORT=%d%c\n",
		(unsigned long)save_time,
		(unsigned long)save_cnt,
		(unsigned long)savenew_cnt,
			pref_flagisoldest1st,
			pref_flagsortorder);
	if (dbobj_store(&tmpdb, "HEADER", 6, rec, strlen(rec), "R"))
		enomem();
	dbobj_close(&tmpdb);
	free(rec);

	rename(save_tmpdbname, save_dbname);
	unlink(save_tmpdbname);

#if 0

	What the fuck was I thinking?
struct stat	new_stat_buf;
struct stat	dir_stat_buf;

	if (!replace && (stat(save_cachename, &new_stat_buf)
				|| stat(curname, &dir_stat_buf)
				)
		)
	{
		unlink(save_cachename);
		free(save_cachename);
		free(save_tmpcachename);
		free(curname);
		return;
	}
#endif

	free(curname);
	free(save_dbname);
	free(save_tmpdbname);
}

void maildir_savefoldermsgs(const char *folder)
{
}

/************************************************************************/

struct	MSGINFO_LIST {
	struct MSGINFO_LIST	*next;
	MSGINFO *minfo;
	} ;

static void createmdcache(const char *maildir)
{
char	*curname;
DIR *dirp;
struct dirent *de;
struct MSGINFO_LIST *milist, *newmi;
MSGINFO	*mi;
unsigned long cnt=0;

	curname=alloc_filename(maildir, "", "cur");

	time(&current_time);

	maildir_save_start(maildir, current_time);

	milist=0;
	dirp=opendir(curname);
	while (dirp && (de=readdir(dirp)) != NULL)
	{
	char	*filename;

		if (de->d_name[0] == '.')
			continue;

		filename=alloc_filename(curname, "", de->d_name);
		mi=maildir_ngetinfo(filename);
		free(filename);
		if (!mi)	continue;

		if (!(newmi=malloc(sizeof(struct MSGINFO_LIST)))) enomem();
		newmi->next= milist;
		milist=newmi;
		newmi->minfo=mi;
		++cnt;
	}
	if (dirp)	closedir(dirp);
	free(curname);

	if (milist)
	{
	MSGINFO **miarray=malloc(sizeof(MSGINFO *) * cnt);
	unsigned long i;

		if (!miarray)	enomem();
		i=0;
		while (milist)
		{
			miarray[i++]=milist->minfo;
			newmi=milist;
			milist=newmi->next;
			free(newmi);
		}

		qsort(miarray, cnt, sizeof(*miarray),
			( int (*)(const void *, const void *)) messagecmp);
		for (i=0; i<cnt; i++)
		{
			maildir_saveinfo(miarray[i]);
			maildir_nfreeinfo(miarray[i]);
		}
		free(miarray);
	}

	maildir_save_end(maildir);
}

static int chkcache(const char *folder)
{
	if (opencache(folder, "W"))	return (-1);

	if (isoldestfirst != pref_flagisoldest1st)	return (-1);
	if (sortorder != pref_flagsortorder)		return (-1);
	return (0);
}

static void	maildir_getfoldermsgs(const char *folder)
{
char	*dir=xlate_shmdir(folder);

	if (!dir)	return;

	while ( chkcache(folder) )
	{
		closedb();
		createmdcache(dir);
	}
	free(dir);
}

void	maildir_remcache(const char *folder)
{
char	*dir=xlate_shmdir(folder);
char	*cachename=alloc_filename(dir, "", MAILDIRCURCACHE "." DBNAME);

	unlink(cachename);
	if (folderdatname && strcmp(folderdatname, cachename) == 0)
		closedb();
	free(cachename);
	free(dir);
}

void	maildir_reload(const char *folder)
{
char	*dir=xlate_shmdir(folder);
char	*curname;
struct	stat	stat_buf;

	if (!dir)	return;

	curname=alloc_filename(dir, "cur", ".");
	time(&current_time);

	/* Remove old cache file when: */

	if (opencache(folder, "W") == 0)
	{
		if ( stat(curname, &stat_buf) != 0 ||
			stat_buf.st_mtime >= cachemtime)
		{
			closedb();
			createmdcache(dir);
		}
	}
	free(dir);
	maildir_getfoldermsgs(folder);
	free(curname);
}

/*
	maildir_readfolders(char ***) - read all the folders in the mailbox.
	maildir_freefolders(char ***) - deallocate memory
*/

static void addfolder(const char *name, char ***buf, size_t *size, size_t *cnt)
{
	if (*cnt >= *size)
	{
	char	**newbuf= *buf ? realloc(*buf, (*size + 10) * sizeof(char *))
			: malloc( (*size+10) * sizeof(char *));

		if (!newbuf)	enomem();
		*buf=newbuf;
		*size += 10;
	}

	(*buf)[*cnt]=0;
	if ( name && ((*buf)[*cnt]=strdup(name)) == 0)	enomem();
	++*cnt;
}

/*
**  Return a sorted list of folders.
**
**  Shared folders:
**
**  Shared folders are returned as ":maildirname.foldername".
**
**  : characters are not valid in regular folder names, so we use that
**  to mark shared folders, and they will be manually sorted after
**  regular folders.
**
**  HACK:
**
**  Subscribed folders will be listed as ":maildirname.last.0foldername".
**  Unsubscribed folders will be listed as ":mailfirname.last.1foldername".
**  So the unsubscribed folders will be sorted after the subscribed ones.
**
**  Then, after sorting, remove the 0 or 1 extra character.
*/

static int mdcomparefunc( char **a, char **b)
{
char	*ca= *a, *cb= *b;

	if (*ca == ':' && *cb != ':')	return (1);
	if (*ca != ':' && *cb == ':')	return (-1);
	return (strcasecmp(ca, cb));
}

struct add_shared_info {
	char ***p;
	size_t *s;
	size_t *c;
	} ;

static char *append_suffix(const char *n)
{
char	*name;
const	char *q=strrchr(n, '.');

	if (!q)		return (0);	/* ??? */
	++q;

	name=malloc(strlen(n)+sizeof(":0"));
	if (!name)	return (0);

	*name=':';
	memcpy(name+1, n, q-n);
	strcpy( &name[q-n+1], "0");
	strcat(name, q);
	return (name);
}

static void add_shared(const char *n, void *p)
{
struct add_shared_info *i= (struct add_shared_info *)p;
char	*s;

	if (strchr(n, ':'))	return;

	s=append_suffix(n);
	if (!s)	return;

	addfolder(s, i->p, i->s, i->c);
	free(s);
}

static void add_sharable(const char *n, void *p)
{
struct add_shared_info *i= (struct add_shared_info *)p;
char	*s;
char	**c;
size_t	j;

	if (strchr(n, ':'))	return;

	/* Only add folders that haven't been subscribed to yet */

	s=append_suffix(n);
	if (!s)	return;

	c= *i->p;

	for (j=0; j < *i->c; j++)
		if (strcmp( (*i->p)[j], s) == 0)
		{
			free(s);
			return;
		}

	strrchr(s, '.')[1] = '1';
	addfolder(s, i->p, i->s, i->c);
	free(s);
}


void maildir_readfolders(char ***fp)
{
size_t	fbsize=0;
size_t	fbcnt=0;
DIR *dirp;
struct dirent *dire;
struct stat	stat_buf;
char	*temp_buf;
size_t	i;

	*fp=0;

	addfolder(INBOX, fp, &fbsize, &fbcnt);
	// by lfan, in order to arrange the display order
	addfolder(SENT, fp, &fbsize, &fbcnt);
	addfolder(DRAFTS, fp, &fbsize, &fbcnt);
	addfolder(TRASH, fp, &fbsize, &fbcnt);

	dirp=opendir(".");
	while ( dirp && (dire=readdir(dirp)) != 0)
	{
		if (dire->d_name[0] != '.' ||
			strcmp(dire->d_name, ".") == 0 ||
			strcmp(dire->d_name, "..") == 0 ||
			// by lfan
			strcmp(dire->d_name, ".Sent") == 0 ||
			strcmp(dire->d_name, ".Drafts") == 0 ||
			strcmp(dire->d_name, ".Trash") == 0 ||
			strchr(dire->d_name, ':'))
			continue;

		if ((temp_buf=malloc(strlen(dire->d_name)+5)) == 0)
			enomem();

		if ( stat(
			strcat(strcpy(temp_buf, dire->d_name), "/tmp"),
				&stat_buf) != 0 ||
			!S_ISDIR(stat_buf.st_mode) ||
		     stat(
			strcat(strcpy(temp_buf, dire->d_name), "/new"),
				&stat_buf) != 0 ||
			!S_ISDIR(stat_buf.st_mode) ||
		     stat(
			strcat(strcpy(temp_buf, dire->d_name), "/cur"),
				&stat_buf) != 0 ||
			!S_ISDIR(stat_buf.st_mode) )
		{
			free(temp_buf);
			continue;
		}
		free(temp_buf);
		addfolder(dire->d_name+1, fp, &fbsize, &fbcnt);
	}
	if (dirp)	closedir(dirp);

	{
	struct add_shared_info info;

		info.p=fp;
		info.s= &fbsize;
		info.c= &fbcnt;

		maildir_list_shared(".", &add_shared, &info);
		maildir_list_sharable(".", &add_sharable, &info);
	}
	
	// by lfan
	//qsort( (*fp)+1, fbcnt-1, sizeof(**fp),
	qsort( (*fp)+4, fbcnt-4, sizeof(**fp),
		(int (*)(const void *, const void *))mdcomparefunc);

	for (i=0; i < fbcnt; i++)
	{
	char	*c=(*fp)[i];

		if (*c != ':')	continue;
		c=strrchr(c, '.');
		++c;
		while (*c)
		{
			*c=c[1];
			++c;
		}
	}
	addfolder(NULL, fp, &fbsize, &fbcnt);
}

void maildir_freefolders(char ***fp)
{
size_t	cnt;

	for (cnt=0; (*fp)[cnt]; cnt++)
		free( (*fp)[cnt] );
	free(*fp);
}

int maildir_create(const char *foldername)
{
char	*dir=xlate_mdir(foldername);
int	rc= -1;

	if (mkdir(dir, 0700) == 0)
	{
	char *tmp=alloc_filename(dir, "tmp", "");

		if (mkdir(tmp, 0700) == 0)
		{
		char *tmp2=alloc_filename(dir, "new", "");

			if (mkdir(tmp2, 0700) == 0)
			{
			char *tmp3=alloc_filename(dir, "cur", "");

				if (mkdir(tmp3, 0700) == 0)
				{
				char *tmp4=alloc_filename(dir, "maildirfolder",
					"");

					close(open(tmp4, O_RDWR|O_CREAT, 0600));
					rc=0;
					free(tmp4);
				}
				free(tmp3);
			}
			if (rc)	rmdir(tmp2);
			free (tmp2);
		}
		if (rc)	rmdir(tmp);
		free(tmp);
	}
	if (rc)	rmdir(dir);
	free(dir);
	return (rc);
}

static void rmdirtmp(const char *tmpdir)
{
DIR *dirp;
struct dirent *de;

	dirp=opendir(tmpdir);
	while (dirp && (de=readdir(dirp)) != NULL)
	{
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
		{
		char	*q=alloc_filename(tmpdir, "", de->d_name);

			if (q)	{ unlink(q); free(q); }
		}
	}
	if (dirp)	closedir(dirp);
}

static void rmdirmain(const char *maindir)
{
DIR *dirp;
struct dirent *de;

	dirp=opendir(maindir);
	while (dirp && (de=readdir(dirp)) != NULL)
	{
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
		{
		char	*q=alloc_filename(maindir, "", de->d_name);

			if (q)	{ unlink(q); free(q); }
		}
	}
	if (dirp)	closedir(dirp);
}

int maildir_delete(const char *foldername, int deletecontent)
{
char	*dir, *tmp, *new, *cur;
int	rc=0;

	if (strcmp(foldername, INBOX) == 0 ||
		strcmp(foldername, SENT) == 0 ||
		strcmp(foldername, TRASH) == 0 ||
		strcmp(foldername, DRAFTS) == 0 || 
		strcmp(foldername, SPAM) == 0)	return (-1);

	dir=xlate_mdir(foldername);
	tmp=alloc_filename(dir, "tmp", "");
	cur=alloc_filename(dir, "cur", "");
	new=alloc_filename(dir, "new", "");

	if (deletecontent)
	{
		rmdirtmp(cur);
		rmdirtmp(new);
	}

	if ( rmdir(new) || rmdir(cur)
			|| ( rmdirtmp(tmp), rmdir(tmp))
			|| (rmdirmain(dir), rmdir(dir)) )
	{
		rc= -1;
		mkdir(tmp, 0700);
		mkdir(new, 0700);
		mkdir(cur, 0700);
	}
	free(tmp);
	free(new);
	free(cur);
	free(dir);
	return (rc);
}

int maildir_deleteall(const char *foldername)
{
char	*dir, *tmp, *new, *cur;
int	rc=0;

	if (
		(strcmp(foldername, TRASH) != 0) && 
		(strcmp(foldername, SPAM) != 0)  
	   )	return (-1);

	dir=xlate_mdir(foldername);
	tmp=alloc_filename(dir, "tmp", "");
	cur=alloc_filename(dir, "cur", "");
	new=alloc_filename(dir, "new", "");

	rmdirtmp(cur);

	free(tmp);
	free(new);
	free(cur);
	free(dir);
	return (rc);
}


int maildir_rename_wrapper(const char *from, const char *to)
{
char	*fromdir, *todir;
int	rc;

	if (strcmp(from, INBOX) == 0 ||
		strcmp(from, SENT) == 0 ||
		strcmp(from, TRASH) == 0 ||
		strcmp(from, DRAFTS) == 0 ||
		strcmp(to, INBOX) == 0)	return (-1);

	fromdir=xlate_mdir(from);
	todir=xlate_mdir(to);
	rc=maildir_rename(".", fromdir, todir,
			  MAILDIR_RENAME_FOLDER, NULL);
	free(fromdir);
	free(todir);
	return (rc);
}
/* ------------------------------------------------------------------- */

/* Here's where we create a new message in a maildir.  First maildir_createmsg
** is called.  Then, the message contents are defined via maildir_writemsg,
** then maildir_closemsg is called. */

static char writebuf[BUFSIZ];
static char *writebufptr;
static int writebufcnt, writebufleft;
static int writeerr;
off_t writebufpos;
int	writebuf8bit;

int	maildir_createmsg(const char *foldername, const char *seq,
		char **retname)
{
	char	*p;
	char	*dir=xlate_mdir(foldername);
	char	*filename;
	int	n;
	struct maildir_tmpcreate_info createInfo;

	/* Create a new file in the tmp directory. */

	maildir_tmpcreate_init(&createInfo);

	createInfo.maildir=dir;
	createInfo.uniq=seq;
	createInfo.doordie=1;

	if ((n=maildir_tmpcreate_fd(&createInfo)) < 0)
	{
		error("maildir_createmsg: cannot create temp file.");
	}

	/*
	** Ok, new maildir semantics: filename in new is different than in tmp.
	** Originally this whole scheme was designed with the filenames being
	** the same.  We can fix it like this:
	*/

	close(n);

	memcpy(strrchr(createInfo.newname, '/')-3, "tmp", 3); /* Hack */

	if (rename(createInfo.tmpname, createInfo.newname) < 0 ||
	    (n=open(createInfo.newname, O_RDWR)) < 0)
	{
		error("maildir_createmsg: cannot create temp file.");
	}


	filename=createInfo.newname;
	createInfo.newname=NULL;

	maildir_tmpcreate_free(&createInfo);

	p=strrchr(filename, '/');
	*retname=strdup(p+1);

	if (*retname == 0)
	{
		close(n);
		free(filename);
		enomem();
	}
	free(filename);

	/* Buffer writes */

	writebufcnt=0;
	writebufpos=0;
	writebuf8bit=0;
	writebufleft=0;
	writeerr=0;
	return (n);
}

/* Like createmsg, except we're rewriting the contents of this message here,
** so we might as well use the same name. */

int maildir_recreatemsg(const char *folder, const char *name, char **baseptr)
{
char	*dir=xlate_mdir(folder);
char	*base;
char	*p;
int	n;

	base=maildir_basename(name);
	p=alloc_filename(dir, "tmp", base);

	free(dir);
	*baseptr=base;
	n=maildir_safeopen(p, O_CREAT|O_RDWR|O_TRUNC, 0644);
	if (n < 0)	free(base);
	free(p);
	writebufcnt=0;
	writebufleft=0;
	writeerr=0;
	writebufpos=0;
	writebuf8bit=0;
	return (n);
}


/* Flush write buffer */

static void writeflush(int n)
{
const char	*q=writebuf;
int	c;

	/* Keep calling write() until there's an error, or we're all done */

	while (!writeerr && writebufcnt)
	{
		c=write(n, q, writebufcnt);
		if ( c <= 0)
			writeerr=1;
		else
		{
			q += c;
			writebufcnt -= c;
		}
	}

	/* We have an empty buffer now */

	writebufcnt=0;
	writebufleft=sizeof(writebuf);
	writebufptr=writebuf;
}

	/* Add whatever we have to the buffer */

/* Write to the message file.  The writes are buffered, and we will set a
** flag if there's error writing to the message file.
*/

void maildir_writemsgstr(int n, const char *p)
{
	maildir_writemsg(n, p, strlen(p));
}

void	maildir_writemsg(int n, const char *p, size_t cnt)
{
int	c;
size_t	i;

	writebufpos += cnt;	/* I'm optimistic */
	for (i=0; i<cnt; i++)
		if (p[i] & 0x80)	writebuf8bit=1;

	while (cnt)
	{
		/* Flush buffer if it's full.  No need to flush if we've
		** already had an error before. */

		if (writebufleft == 0)	writeflush(n);

		c=writebufleft;
		if (c > cnt)	c=cnt;
		memcpy(writebufptr, p, c);
		writebufptr += c;
		p += c;
		cnt -= c;
		writebufcnt += c;
		writebufleft -= c;
	}
}

/* The new message has been written out.  Move the new file from the tmp
** directory to either the new directory (if 'new' is set), or to the
** cur directory.
**
** The caller might have encountered an error condition.  If 'isok' is zero,
** we just delete the file.  If we had a write error, we delete the file
** as well.  We return -1 in both cases, or 0 if the new file has been
** succesfully moved into its final resting place.
*/

int	maildir_writemsg_flush(int n )
{
	writeflush(n);
	return (writeerr);
}

int	maildir_closemsg(int n,	/* File descriptor */
	const char *folder,	/* Folder */
	const char *retname,	/* Filename in folder */
	int isok,	/* 0 - discard it (I changed my mind),
			   1 - keep it
			  -1 - keep it even if we'll exceed the quota
			*/
	unsigned long prevsize	/* Prev size of this msg, used in quota calc */
		)
{
char	*dir=xlate_mdir(folder);
char	*oldname=alloc_filename(dir, "tmp", retname);
char	*newname;
struct	stat	stat_buf;

	writeflush(n);	/* If there's still anything in the buffer */
	if (fstat(n, &stat_buf))
	{
		close(n);
		unlink(oldname);
		enomem();
	}

	newname=maildir_find(folder, retname);
		/* If we called recreatemsg before */

	if (!newname)
	{
		newname=alloc_filename(dir, "cur:2,S", retname);
		/* Hack of the century          ^^^^ */
		strcat(strcat(strcat(strcpy(newname, dir), "/cur/"),
			retname), ":2,S");
	}

	if (writeerr)
	{
		close(n);
		unlink(oldname);
		enomem();
	}

	close(n);

	if (isok)
	{
		if (prevsize < stat_buf.st_size)
		{
			struct maildirsize info;

			if (maildir_quota_add_start(".", &info,
						    stat_buf.st_size-prevsize,
						    prevsize == 0 ? 1:0,
						    NULL))
			{
				if (isok < 0) /* Force it */
				{
					maildir_quota_deleted(".", (long)
							      (stat_buf.st_size
							       -prevsize),
							      prevsize == 0
							      ? 1:0);
					isok= -2;
				}
				else
					isok=0;
			}
			maildir_quota_add_end(&info, stat_buf.st_size-prevsize,
					      prevsize == 0 ? 1:0);
		}
		else if (prevsize != stat_buf.st_size)
		{
			maildir_quota_deleted(".", (long)
					      (stat_buf.st_size-prevsize),
					      prevsize == 0 ? 1:0);
		}
	}

	if (isok)
		rename(oldname, newname);
		
	unlink(oldname);

	if (isok)
	{
	char	*realnewname=maildir_requota(newname, stat_buf.st_size);

		if (strcmp(newname, realnewname))
			rename(newname, realnewname);
		free(realnewname);
	}
	free(dir);
	free(oldname);
	free(newname);
	return (isok && isok != -2? 0:-1);
}

void	maildir_deletenewmsg(int n, const char *folder, const char *retname)
{
char	*dir=xlate_mdir(folder);
char	*oldname=alloc_filename(dir, "tmp", retname);

	close(n);
	unlink(oldname);
	free(oldname);
	free(dir);
}

void maildir_cleanup()
{
	closedb();
}

/*
** Convert folder names to modified-UTF7 encoding.
*/

char *folder_toutf7(const char *foldername)
{
	char *p;

#if UTF7_FOLDER_ENCODING
	unicode_char *uc;
	int errflag;
	const struct unicode_info *uiptr;

	if ((uiptr=unicode_find(sqwebmail_content_charset)) != 0 &&
	    (uc=(*uiptr->c2u)(uiptr, foldername, &errflag)) != 0)
	{
		p=unicode_uctomodutf7(uc);

		free(uc);
		if (!p)
			enomem();
		return (p);
	}
#endif

	p=strdup(foldername);
	if (!p)
		enomem();
	return (p);
}

char *folder_fromutf7(const char *foldername)
{
	char *p;

#if UTF7_FOLDER_ENCODING
	unicode_char *uc;
	int errflag;
	const struct unicode_info *uiptr;

	if ((uiptr=unicode_find(sqwebmail_content_charset)) != 0 &&
	    (uc=unicode_modutf7touc(foldername, &errflag)) != 0)
	{
		p=(*uiptr->u2c)(uiptr, uc, NULL);
		free(uc);
		if (p)
			return (p);
	}
#endif

	p=strdup(foldername);
	if (!p)
		enomem();
	return (p);
}
