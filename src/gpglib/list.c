/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

static const char rcsid[]="$Id: list.c,v 1.1.1.1 2003/05/07 02:14:57 lfan Exp $";

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
** List keys
*/

static int dolist(int (*)(const char *, const char *, const char *, int,
			  struct gpg_list_info *),
		  int (*)(const char *, size_t, void *),
		  void *);

int gpg_listkeys(const char *gpgdir,
		 int secret,
		 int (*callback_func)(const char *, const char *,
				      const char *,
				      int,
				      struct gpg_list_info *),
		 int (*err_func)(const char *, size_t, void *),
		 struct gpg_list_info *voidarg)
{
	char *argvec[7];
	int rc;

	argvec[0]="gpg";
	argvec[1]= secret ? "--list-secret-keys":"--list-sigs";
	argvec[2]="--with-colons";
	argvec[3]="--fingerprint";
	argvec[4]="-q";
	argvec[5]="--no-tty";
	argvec[6]=0;

#define DEFINIT(c, a) if (!(c)) (c)=(a)

	DEFINIT(voidarg->charset, &unicode_ISO8859_1);
	DEFINIT(voidarg->disabled_msg, "[ This key is disabled ]");
	DEFINIT(voidarg->revoked_msg, "[ This key is revoked ]");
	DEFINIT(voidarg->expired_msg, "[ This key is expired ]");

	if (gpg_fork(&gpg_stdin, &gpg_stdout, &gpg_stderr, gpgdir, argvec) < 0)
		rc= -1;
	else
	{
		int rc2;

		rc=dolist(callback_func, err_func, voidarg);

		rc2=gpg_cleanup();
		if (rc2)
			rc=rc2;
	}
	return (rc);
}

/*
** Parse output of gpg into physical lines.
*/

struct gpg_callbackinfo {
	int (*err_func)(const char *, size_t, void *);
	/* stderr passthrough */

	struct gpg_list_info *voidarg;
	/* passthrough arg */

	/* Line buffer: */
	char *linebuffer;
	size_t linebufsize;
	size_t linebufcnt;

	/* Function that receives collected lines, + passthrough arg */
	int (*line_func)(char *, void *, struct gpg_list_info *);
	void *line_callback_arg;
} ;

static int gpg_line_stderr(const char *l, size_t c, void *vp)
{
	return (0);
}

static int gpg_line_stdout(const char *l, size_t c, void *vp)
{
	struct gpg_callbackinfo *gci=(struct gpg_callbackinfo *)vp;
	size_t i, j;

	if (c + gci->linebufcnt >= gci->linebufsize)
	{
		/* Need bigger line buffer */

		size_t news= c+gci->linebufcnt+256;

		char *newp= gci->linebuffer ? realloc(gci->linebuffer, news)
			: malloc(news);

		if (!newp)
			return (-1);
		gci->linebuffer=newp;
		gci->linebufsize=news;
	}

	memcpy(gci->linebuffer + gci->linebufcnt, l, c);
	gci->linebufcnt += c;

	/* Search for collected newlines, extract complete lines,
	** invoke the callback function.
	*/

	for (;;)
	{
		int rc;

		for (i=0; i<gci->linebufcnt; i++)
			if (gci->linebuffer[i] == '\n')
				break;
		if (i >= gci->linebufcnt)
			break;
		gci->linebuffer[i++]=0;

		rc= (*gci->line_func)(gci->linebuffer, gci->line_callback_arg,
				      gci->voidarg);
		j=0;
		while (i < gci->linebufcnt)
		{
			gci->linebuffer[j]=gci->linebuffer[i];
			++i;
			++j;
		}
		gci->linebufcnt=j;
		if (rc)
			return (rc);
	}
	return (0);
}

/*
** Parse list output of gpg into distrinct keys.
*/

struct gpg_callbacklistinfo {

	int (*callback_func)(const char *, const char *, const char *, int,
			     struct gpg_list_info *);
	int invalid_flag;
	char fingerprint[128];
	char shortname[256];
	char *keybuffer;
	size_t keybufsize;
	size_t keybuflen;

	int seen_startofkey;
} ;

static int dolist_callback(char *, void *, struct gpg_list_info *);

static int dolist(int (*callback_func)(const char *, const char *,
				       const char *, int,
				       struct gpg_list_info *),
		  int (*err_func)(const char *, size_t, void *),
		  void *voidarg)
{
	struct gpg_callbackinfo gci;
	struct gpg_callbacklistinfo gcli;
	int rc, rc2;

	close(gpg_stdin);
	gpg_stdin= -1;

	memset(&gci, 0, sizeof(gci));
	gci.err_func=err_func;
	gci.voidarg=voidarg;

	gci.line_func= &dolist_callback;
	gci.line_callback_arg= &gcli;

	memset(&gcli, 0, sizeof(gcli));
	gcli.callback_func=callback_func;

	rc=gpg_read( gpg_line_stdout,
		     gpg_line_stderr,
		     NULL,
		     0,
		     &gci);


	rc2= (*gci.line_func)(NULL, gci.line_callback_arg,
					  gci.voidarg);
	if (rc2)
		rc=rc2;

	if (gcli.keybuffer)
		free(gcli.keybuffer);

	if (gci.linebuffer)
		free(gci.linebuffer);
	return (rc);
}

static char *nextword(char *);

static int nyb(int c)
{
	static const char xdigits[]="0123456789ABCDEFabcdef";

	char *p=strchr(xdigits, c);

	if (!p)
		return (0);

	c= p-xdigits;

	if (c >= 16)
		c -= 6;
	return (c);
}

static int append_key(struct gpg_callbacklistinfo *, const char *);

static int dolist_callback(char *line, void *vp1,
			   struct gpg_list_info *vp)
{
	struct gpg_callbacklistinfo *gci=(struct gpg_callbacklistinfo *)vp1;

	char *rectype;
	char *trust;
	char *length;
	int algo;
	char *keyid;
	char *crdate;
	char *expdate;
	char *localid;
	char *ownertrust;
	char *userid;
	char *p;
	const char *stat;

	if (!line || strncmp(line, "pub", 3) == 0
	    || strncmp(line, "sec", 3) == 0)
	{
		if (gci->seen_startofkey)
		{
			int rc=(*gci->callback_func)(gci->fingerprint,
						     gci->shortname,
						     gci->keybuflen ?
						     gci->keybuffer:"",
						     gci->invalid_flag,
						     vp);
			gci->keybuflen=0;
			gci->fingerprint[0]=0;
			gci->shortname[0]=0;
			if (rc)
				return (rc);
		}

		if (!line)
			return (0);

		gci->seen_startofkey=1;
	}

	if (!gci->seen_startofkey)
		return (0);

	p=line;
	rectype=p; p=nextword(p);
	trust=p; p=nextword(p);
	length=p; p=nextword(p);
	algo=atoi(p); p=nextword(p);
	keyid=p; p=nextword(p);
	crdate=p; p=nextword(p);
	expdate=p; p=nextword(p);
	localid=p; p=nextword(p);
	ownertrust=p; p=nextword(p);
	userid=p; p=nextword(p);

	{
		char *q;

		for (p=q=userid; *p; )
		{
			int n;

			if (*p != '\\')
			{
				*q=*p;
				++p;
				++q;
				continue;
			}
			++p;
			if (*p != 'x')
			{
				*q=*p;
				if (*p)
					++p;
				++q;
				continue;
			}
			++p;
			n=nyb(*p);
			if (*p)
				++p;
			n=n * 16 + nyb(*p);
			if (*p)
				++p;
			*q=n;
			++q;
		}
		*q=0;
	}

	stat=0;

	if (strcmp(rectype, "fpr") == 0)
	{
		gci->fingerprint[0]=0;
		strncat(gci->fingerprint, userid,
			sizeof(gci->fingerprint)-2);
		return (0);
	}

	if (strcmp(rectype, "pub") == 0 ||
	    strcmp(rectype, "sec") == 0)
	{
		stat= *trust == 'd' ? vp->disabled_msg:
			*trust == 'r' ? vp->revoked_msg:
			*trust == 'e' ? vp->expired_msg:0;

	}
	else if (strcmp(rectype, "sub") &&
		 strcmp(rectype, "ssb") &&
		 strcmp(rectype, "uid") &&
		 strcmp(rectype, "sig"))
		return (0);

	if (append_key(gci, rectype) ||
	    append_key(gci, " "))
		return (-1);


	gci->invalid_flag= stat ? 1:0;

	{
		char buf[60];

		sprintf(buf, "%4.4s/%-8.8s", length,
			algo == 1 ? "RSA":
			algo == 16 ? "ElGamal":
			algo == 17 ? "DSA":
			algo == 20 ? "DSA":"???");

		if (algo == 0 || *length == 0)
			sprintf(buf, "%13s", "");

		if (append_key(gci, buf))
			return (-1);
	}

	userid=unicode_cfromutf8(vp->charset, userid, NULL);
	if (!userid)
		return (-1);

	if (strcmp(rectype, "pub") == 0 ||
	    strcmp(rectype, "sec") == 0)
	{
		gci->shortname[0]=0;
		strncat(gci->shortname, userid,
			sizeof(gci->shortname)-2);

	}

	if (append_key(gci, " ")
	    || append_key(gci, atoi(crdate) ?  crdate :"          ")
	    || append_key(gci, " ")
	    || append_key(gci, atoi(expdate) ? expdate:"          ")
	    || append_key(gci, " ")
	    || append_key(gci, userid)
	    || append_key(gci, "\n"))
	{
		free(userid);
		return (-1);
	}
	free(userid);

	if (stat)
	{
		append_key(gci, "                  ");
		append_key(gci, stat);
		append_key(gci, "\n");
	}

	return (0);
}

static char *nextword(char *p)
{
	while (*p && *p != ':')
		++p;

	if (*p)
		*p++=0;
	return (p);
}

static int append_key(struct gpg_callbacklistinfo *gci, const char *l)
{
	int ll=strlen(l);

	if (ll + gci->keybuflen >= gci->keybufsize)
	{
		int n=ll + gci->keybuflen + 256;

		char *p= gci->keybuffer ? realloc(gci->keybuffer, n)
			: malloc(n);
		if (!p)
			return (-1);
		gci->keybuffer=p;
		gci->keybufsize=n;
	}
	strcpy(gci->keybuffer + gci->keybuflen, l);
	gci->keybuflen += ll;
	return (0);
}

