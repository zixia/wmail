/*
** Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: newmsg.c,v 1.4 2003/05/30 09:41:29 lfan Exp $
*/
#include	"config.h"
#include	"sqwebmail.h"
#include	"newmsg.h"
#include	"cgi/cgi.h"
#include	"sqconfig.h"
#include	"auth.h"
#include	"maildir.h"
#include	"token.h"
#include	"pref.h"
#include	"folder.h"
#include	"filter.h"
#include	"gpg.h"
#include	"addressbook.h"
#include	"maildir/maildirmisc.h"
#include	"maildir/maildirquota.h"
#include	"maildir/maildirgetquota.h"
#include	"rfc822/rfc822.h"
#include	"rfc822/rfc2047.h"
#include	"rfc2045/rfc2045.h"
#include	"gpglib/gpglib.h"
#include	<string.h>
#include	<stdio.h>
#include	<signal.h>
#include	<stdlib.h>
#include	<fcntl.h>
#include	<ctype.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<sys/types.h>
#if	HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#include	<errno.h>
#include	"htmllibdir.h"

extern const char *sqwebmail_content_charset;
extern int spell_start(const char *);
extern const char *sqwebmail_mailboxid;
extern const char *sqwebmail_folder;
extern void print_safe_len(const char *, size_t, void (*)(const char *, size_t));
extern void call_print_safe_to_stdout(const char *, size_t);
extern void print_attrencodedlen(const char *, size_t, int, FILE *);
extern void output_attrencoded_nltobr(const char *);
extern void output_attrencoded_oknl(const char *);
extern void output_attrencoded(const char *);
extern void output_scriptptrget();
extern void output_form(const char *);
extern void output_urlencoded(const char *);

extern char *newmsg_newdraft(const char *, const char *, const char *,
				const char *);
extern char *newmsg_createdraft(const char *);
extern char *newmsg_createsentmsg(const char *, int *, int);
extern int ishttps();

static void newmsg_header(const char *label, const char *field, const char *val)
{
	// by lfan
	printf("<TR><TD CLASS=\"new-message-header\" ALIGN=RIGHT WIDTH=60>"
	       //"<FONT CLASS=\"new-message-header-%s\">%s</FONT></TD>"
	       "%s</TD>"
	       "<TD WIDTH=6>&nbsp;</TD>",
	       //field, label);
	       label);

	//printf("<TD><INPUT NAME=%s SIZE=50 MAXLENGTH=512 VALUE=\"",
	printf("<TD><INPUT CLASS=\"myinput\" NAME=%s SIZE=50 MAXLENGTH=512 VALUE=\"",
		field);
	if (val)
	{
	char	*s;

		s=rfc2047_decode_simple(val);
		if (!s)	enomem();
		output_attrencoded(s);
		free(s);
	}
	printf("\"></TD></TR>\n");
}

static const char *ispreviewmsg()
{
const char *p=cgi("previewmsg");

	if (*p == 0)
		p=cgi("addressbook_to");

	if (*p == 0)
		p=cgi("addressbook_cc");

	if (*p == 0)
		p=cgi("addressbook_bcc");

	return (p);
}

void newmsg_hiddenheader(const char *label, const char *value)
{
	printf("<INPUT TYPE=HIDDEN NAME=\"%s\" VALUE=\"", label);
	output_attrencoded(value);
	printf("\">");
}

/* ---------------------------------------------------- */

/* Display message preview */

static void preview_show_func_s(const char *p, size_t n)
{
	fwrite(p, 1, n, stdout);
}

void preview_start()
{
	printf("<TT CLASS=\"message-text-plain\">");
	filter_start(FILTER_FOR_PREVIEW, &preview_show_func_s);
}

int preview_callback(const char *ptr, size_t cnt, void *voidptr)
{
        filter(ptr, cnt);
        return (0);
}

void preview_end()
{
	filter_end();
	printf("</TT>\n");
}

#if 0
static void show_preview(const char *filename)
{
char	*header, *value;
struct	rfc2045	*rfcp;
FILE	*fp;
int	fd;

	fp=0;
	fd=maildir_safeopen(filename, O_RDONLY, 0);
	if (fd >= 0)
		if ((fp=fdopen(fd, "r")) == 0)
			close(fd);

	if (!fp)	return;

	while ((header=maildir_readheader(fp, &value, 1)) != 0)
	{
		/* Don't show X-, From, and Content- headers in preview */

		if (strncmp(header, "x-", 2) == 0)	continue;
		if (strcmp(header, "mime-version") == 0)	continue;
		if (strncmp(header, "content-", 8) == 0)	continue;

		printf("%c", toupper(*header));
		output_attrencoded_oknl(header+1);
		printf(": ");

		value=rfc2047_decode_enhanced(value, sqwebmail_content_charset);
		if (value)
		{
			output_attrencoded_oknl(value);
			free(value);
		}
		printf("\n");
	}
	printf("\n");

	rfcp=rfc2045_fromfp(fp);
	if (!rfcp)	return;

	filter_start(FILTER_FOR_PREVIEW, &preview_show_func_s);
	{
		struct rfc2045 *q=
			rfc2045_searchcontenttype(rfcp, "text/plain");

		if (q)
			rfc2045_decodemimesection(fileno(fp), q,
						  &filter_stub, NULL);
	}
	rfc2045_free(rfcp);
	filter_end();
}
#endif

static int show_textarea(const char *p, size_t l, void *voidptr)
{
	print_attrencodedlen(p, l, 1, stdout);
	return (0);
}

/*
** Return all from/to/cc/bcc addresses in the message.
*/

char *newmsg_alladdrs(FILE *fp)
{
	char	*headers=NULL;
	struct rfc822t *t;
	struct rfc822a *a;
	char *p, *q;
	int l, i;

	if (fp)
	{
		char *header, *value;

		rewind(fp);

		/* First, combine all the headers into one header. */

		while ((header=maildir_readheader(fp, &value, 1)) != 0)
		{
			char *newh;

			if (strcmp(header, "from") &&
			    strcmp(header, "to") &&
			    strcmp(header, "cc") &&
			    strcmp(header, "bcc"))
				continue;

			if (headers)
			{
				newh=realloc(headers, strlen(headers)
					     +strlen(value)+2);
				if (!newh)
					continue;
				strcat(newh, ",");
				headers=newh;
			}
			else
			{
				newh=malloc(strlen(value)+1);
				if (!newh)
					continue;
				*newh=0;
				headers=newh;
			}
			strcat(headers, value);
		}

	}

	/* Now, parse the header, and extract the addresses */

	t=rfc822t_alloc_new(headers ? headers:"", NULL, NULL);
	a= t ? rfc822a_alloc(t):NULL;

	l=1;
	for (i=0; i < (a ? a->naddrs:0); i++)
	{
		p=rfc822_getaddr(a, i);
		if (p)
		{
			++l;
			l +=strlen(p);
			free(p);
		}
	}
	p=malloc(l);
	if (p)
		*p=0;

	for (i=0; i < (a ? a->naddrs:0); i++)
	{
		q=rfc822_getaddr(a, i);
		if (q)
		{
			if (p)
			{
				strcat(strcat(p, q), "\n");
			}
			free(q);
		}
	}

	rfc822a_free(a);
	rfc822t_free(t);
	free(headers);
	return (p);
}

void newmsg_showfp(FILE *fp, int *attachcnt)
{
	struct	rfc2045 *p=rfc2045_fromfp(fp), *q;

	if (!p)	enomem();

	/* Here's a nice opportunity to count all attachments */

	*attachcnt=0;

	for (q=p->firstpart; q; q=q->next)
		if (!q->isdummy)	++*attachcnt;
	if (*attachcnt)	--*attachcnt;
	/* Not counting the 1st MIME part */

	q=rfc2045_searchcontenttype(p, "text/plain");

	if (q)
		rfc2045_decodemimesection(fileno(fp), q,
					  &show_textarea, NULL);
	rfc2045_free(p);
}

void newmsg_preview(const char *p)
{
	size_t pos;

	maildir_remcache(DRAFTS);
	if (maildir_name2pos(DRAFTS, p, &pos) == 0)
	{
		const char *save_folder=sqwebmail_folder;
		cgi_put("showdraft", "1");
		sqwebmail_folder=DRAFTS;
		folder_showmsg(DRAFTS, pos);
		sqwebmail_folder=save_folder;
				/* show_preview(draftmessagefilename); */
	}
}

/* ---------------------------------------------------- */

void newmsg_init(const char *folder, const char *pos)
{
	const char	*tolab=getarg("TOLAB");
	const char	*cclab=getarg("CCLAB");
	const char	*bcclab=getarg("BCCLAB");
	const char	*subjectlab=getarg("SUBJECTLAB");
	const char	*messagelab=getarg("MESSAGELAB");
	const char	*sendlab=getarg("SENDLAB");
	const char	*previewlab=getarg("PREVIEWLAB");
	const char	*forwardsep=getarg("FORWARDLAB");
	const char	*savedraft=getarg("SAVEDRAFT");
	const char	*uploadlab=getarg("UPLOAD");
	const char	*attachedlab=getarg("ATTACHMENTS");
	const char	*replysalutation=getarg("SALUTATION");
	const char	*checkspellingdone=getarg("SPELLCHECKDONE");
	const char	*checkspelling=getarg("CHECKSPELLING");
	const char	*quotaerr=getarg("QUOTAERR");
	const char	*fromlab=getarg("FROMLAB");
	const char	*replytolab=getarg("REPLYTOLAB");
	const char	*addressbooklab=getarg("ADDRESSBOOK");
	char	*draftmessage;
	char	*draftmessagefilename;
	const	char *p;
	FILE	*fp;
	int	attachcnt=0;
	char	*cursubj, *curto, *curcc, *curbcc, *curfrom, *curreplyto;

	/* Picking up an existing draft? */

	p=cgi("draft");
	if (*p)
	{
		CHECKFILENAME(p);
	}

	if (*p)
	{
		draftmessage=strdup(p);
		if (!draftmessage)	enomem();
		p="";
	}
	else
	{
		draftmessage=newmsg_newdraft(folder, pos,
			forwardsep, replysalutation);

		if (!draftmessage)
		{
			if (*ispreviewmsg())
			{
				p=cgi("draftmessage");
				if (*p)
				{
					CHECKFILENAME(p);
				}
				draftmessage=newmsg_createdraft(p);
			}
		}
	}

	draftmessagefilename= draftmessage ?
				 maildir_find(DRAFTS, draftmessage):0;

	if (*(p=cgi("previewmsg")))
	{
#ifdef	ISPELL
		if (strcmp(p, "SPELLCHK") == 0)
			printf("%s<BR><BR>\n", checkspellingdone);
#endif
		printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=1 CLASS=\"box-small-outer\"><TR><TD>\n");
		printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=4 CLASS=\"preview\"><TR><TD>\n");

		if (draftmessagefilename)
		{
			const char *p=strrchr(draftmessagefilename, '/');

			if (p)
				++p;
			else
				p=draftmessagefilename;

			newmsg_preview(p);
		}
		printf("</TD></TR></TABLE>\n");
		printf("</TD></TR></TABLE>\n");

		printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=6><TR><TD><HR WIDTH=\"80%%\"></TD></TR></TABLE>\n");
	}

	printf("<INPUT TYPE=HIDDEN NAME=form VALUE=\"donewmsg\">\n");
	newmsg_hiddenheader("pos", pos);
	newmsg_hiddenheader("focusto",
			    *cgi("newmsg") ? "headers":"text");

	/* Generate unique message token, to detect duplicate SUBMITs */

	tokennew();

	/* Display any error message */

	if (*cgi("foldermsg"))
	{
		printf("<P><FONT CLASS=\"error\" COLOR=\"#FF0000\">");
		output_attrencoded_nltobr(cgi("foldermsg"));
		printf("</FONT><BR>");
	}

	if (strcmp(cgi("error"), "quota") == 0)
		printf("%s", quotaerr);

	/* Read message from the draft file */

	cursubj=0;
	curto=0;
	curfrom=0;
	curreplyto=0;
	curcc=0;
	curbcc=0;
	fp=0;

	if (draftmessagefilename)
	{
	int	x=maildir_safeopen(draftmessagefilename, O_RDONLY, 0);

		if (x >= 0)
			if ((fp=fdopen(x, "r")) == 0)
				close(x);
	}

	if (fp != 0)
	{
	char *header, *value;

		while ((header=maildir_readheader(fp, &value, 0)) != 0)
		{
		char	**rfchp=0;

			if (strcmp(header, "subject") == 0)
			{
				if (!cursubj && !(cursubj=strdup(value)))
					enomem();
				continue;
			}

			if (strcmp(header, "from") == 0)
				rfchp= &curfrom;
			if (strcmp(header, "reply-to") == 0)
				rfchp= &curreplyto;
			if (strcmp(header, "to") == 0)
				rfchp= &curto;
			if (strcmp(header, "cc") == 0)
				rfchp= &curcc;
			if (strcmp(header, "bcc") == 0)
				rfchp= &curbcc;
			if (rfchp)
			{
			char	*newh=malloc ( (*rfchp ? strlen(*rfchp)+2:1)
					+strlen(value));

				if (!newh)	enomem();
				strcpy(newh, value);
				if (*rfchp)
					strcat(strcat(newh, ","), *rfchp);
				if (*rfchp)	free( *rfchp );
				*rfchp=newh;
			}
		}
	}

	//printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=1 CLASS=\"box-small-outer\"><TR><TD>\n");
	printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=4 CLASS=\"new-message-box\"><TR><TD>\n");

	printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1 WIDTH=\"100%%\">\n");
	/* by lfan
	if (access(NOCHANGINGFROM, 0))
		newmsg_header(fromlab, "headerfrom", curfrom ? curfrom:
			*cgi("from") ? cgi("from"):
			pref_from && *pref_from ? pref_from:
			login_fromhdr());

	printf("<TR VALIGN=MIDDLE><TH ALIGN=RIGHT>"
	       "<P CLASS=\"new-message-header\">"
	       "<FONT CLASS=\"new-message-header-addressbook\">"
	       "%s</FONT></TD><TD WIDTH=6>&nbsp;</TH>",
	       addressbooklab);
	*/
	printf("<TR VALIGN=MIDDLE><TD ALIGN=RIGHT CLASS=\"new-message-header\">"
	       "%s</TD><TD WIDTH=6>&nbsp;</TD>",
	       addressbooklab);

	printf("<TD VALIGN=MIDDLE>");
	printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0>");
	//printf("<TR VALIGN=MIDDLE><TD><SELECT NAME=\"nick\" SIZE=4 MULTIPLE>\n");
	printf("<TR VALIGN=MIDDLE><TD><SELECT NAME=\"nick\">\n");
	ab_listselect();
	printf("</select></TD><TD WIDTH=\"100%%\">");
	printf(" &nbsp;<input type=submit class=mybtn name=\"addressbook_to\" value=\"%s\">",
			tolab);
	printf(" <input type=submit class=mybtn name=\"addressbook_cc\" value=\"%s\">",
			cclab);
	printf(" <input type=submit class=mybtn name=\"addressbook_bcc\" value=\"%s\">",
			bcclab);
	printf("</TD></TR></TABLE>");

	printf("</TD></TR>\n");
	newmsg_header(tolab, "headerto", curto ? curto:cgi("to"));
	newmsg_header(cclab, "headercc", curcc ? curcc:cgi("cc"));
	newmsg_header(bcclab, "headerbcc", curbcc ? curbcc:cgi("bcc"));
	// by lfan
	//newmsg_header(replytolab, "headerreply-to", curreplyto ? curreplyto:cgi("replyto"));
	newmsg_header(subjectlab, "headersubject", cursubj ? cursubj:cgi("subject"));

	if (curto)	free(curto);
	if (curfrom)	free(curfrom);
	if (curreplyto)	free(curreplyto);
	if (curcc)	free(curcc);
	if (curbcc)	free(curbcc);
	if (cursubj)	free(cursubj);

	printf("<TR><TD COLSPAN=3><HR WIDTH=\"100%%\"></TD></TR>");
	/* by lfan
	printf("<TR><TH VALIGN=TOP ALIGN=RIGHT>"
			
	       "<P CLASS=\"new-message-header\">"
	       "<FONT CLASS=\"new-message-header-message\">"
	       "%s</FONT></P></TD><TD WIDTH=6>&nbsp;</TD>", messagelab);
	*/
	printf("<TR><TD WIDTH=60 VALIGN=TOP ALIGN=RIGHT CLASS=\"ctype\">"
	       "%s</TD><TD WIDTH=6>&nbsp;</TD>", messagelab);

	printf("<TD><TEXTAREA NAME=message COLS=%d ROWS=15 WRAP=soft>",
		MYLINESIZE);

	if (fp)
	{
		newmsg_showfp(fp, &attachcnt);
	}
	else
	{
		printf("%s", cgi("body"));
		if ((fp=fopen(SIGNATURE, "r")) != NULL)
		{
		char	buf[256];
		int	n;

			printf("\n\n");
			while ((n=fread(buf, 1, sizeof(buf)-1, fp)) > 0)
			{
				buf[n]=0;
				output_attrencoded_oknl(buf);
			}
			fclose(fp);
			fp=NULL;
		}
	}
	printf("</TEXTAREA><BR>\n");

	if (draftmessage && *draftmessage)
	{
		printf("<INPUT TYPE=HIDDEN NAME=draftmessage VALUE=\"");
		output_attrencoded(draftmessage);
		printf("\">");
	}
	if (draftmessage)	free(draftmessage);
	printf("</TD></TR>\n");

	//by lfan
	printf("<TR><TD>&nbsp;</TD><TD>&nbsp;</TD><TD>");

	printf("<INPUT TYPE=CHECKBOX "
	       "NAME=fcc%s><FONT class=ctype>%s</FONT>&nbsp;\n",
	       pref_noarchive ? "":" CHECKED",
	       getarg("PRESERVELAB"));

	if (access(NODSN, R_OK))
		printf("<TR><TD COLSPAN=2 ALIGN=RIGHT><INPUT TYPE=CHECKBOX "
		       "NAME=dsn></TD><TD>%s</TD></TR>\n",
		       getarg("DSN"));

#if HAVE_SQWEBMAIL_UNICODE
	if (has_gpg(GPGDIR) == 0)
	{
		char *all_addr;

		printf("<TR><TD COLSPAN=2 ALIGN=RIGHT><INPUT TYPE=CHECKBOX "
		       "NAME=sign></TD><TD>%s<SELECT NAME=signkey>",
		       getarg("SIGNLAB"));
		gpgselectkey();
		printf("</SELECT></TD></TR>\n");

		all_addr=newmsg_alladdrs(fp);

		printf("<TR VALIGN=MIDDLE><TD COLSPAN=2 ALIGN=RIGHT>"
		       "<INPUT TYPE=CHECKBOX NAME=encrypt></TD>"
		       "<TD><TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 VALIGN=MIDDLE><TR><TD>%s</TD><TD><SELECT SIZE=4 MULTIPLE NAME=encryptkey>",
		       getarg("ENCRYPTLAB"));
		gpgencryptkeys(all_addr);
		printf("</SELECT></TD></TR>\n");
		printf("</TABLE></TD></TR>\n");

		if (ishttps())
			printf("<TR><TD COLSPAN=2 ALIGN>&nbsp;</TD><TD>%s<input type=password name=passphrase></TD></TR>\n",
			       getarg("PASSPHRASE"));

		if (all_addr)
			free(all_addr);
	}
#endif

	if (fp)
		fclose(fp);
	if (draftmessagefilename)
		free(draftmessagefilename);

	//printf("<TR><TD COLSPAN=2>&nbsp;</TD><TD>");
	//printf("<INPUT CLASS=mybtn TYPE=SUBMIT NAME=previewmsg VALUE=\"%s\">",
	//	previewlab);
	printf("<INPUT CLASS=mybtn TYPE=SUBMIT NAME=sendmsg onclick=\"return check();\" VALUE=\"%s\">&nbsp;",
		sendlab);
	printf("<INPUT CLASS=mybtn TYPE=SUBMIT NAME=previewmsg VALUE=\"%s\">&nbsp;",
		previewlab);
	printf("<INPUT CLASS=mybtn TYPE=SUBMIT NAME=savedraft VALUE=\"%s\">&nbsp;",
		savedraft);

        printf("<INPUT CLASS=\"mybtn\" TYPE=SUBMIT NAME=doattachments VALUE=\"");
	printf(uploadlab);
	printf("\">&nbsp;");
	printf(attachedlab, attachcnt);
	
#ifdef	ISPELL
	printf("<INPUT TYPE=SUBMIT NAME=startspellchk VALUE=\"%s\">",
		checkspelling);
#endif
	printf("</TD></TR>\n");
	printf("</TD></TABLE>\n");

	printf("</TD></TR></TABLE>\n");
	//printf("</TD></TR></TABLE>\n");
}

static const char *geterrbuf(int fd)
{
static char errbuf[512];
char	*errbufptr=errbuf;
size_t	errbufleft=sizeof(errbuf)-1;

	while (errbufleft)
	{
	int	l=read(fd, errbufptr, errbufleft);

		if (l <= 0)	break;
		errbufptr += l;
		errbufleft -= l;
	}
	*errbufptr=0;
	return (errbuf);
}

static int waitfor(pid_t pid)
{
pid_t	childpid;
int	wait_stat;

	while ((childpid=wait(&wait_stat)) != pid)
		if (childpid == -1)	return (-1);

	return (wait_stat);
}

void sendmsg_done()
{
	/* by lfan, add send OK function */
	if (*cgi("savedraft")) {
		if ( *cgi("pos"))
			http_redirect_argss("&form=readmsg&pos=%s", cgi("pos"), "");
		else
			http_redirect_argss("&form=folders", "", "");
	}
	else {
		if ( *cgi("pos") )
			http_redirect_argss("&form=sendok&pos=%s", cgi("pos"), "");
		else
			http_redirect_argss("&form=sendok&pos=%s", "-1", "");
	}
}

static int dosendmsg(const char *origdraft)
{
pid_t	pid;
const	char *returnaddr;
int	pipefd1[2];
char	*filename;
const char *line;
char	*draftmessage;
int	isgpgerr;
unsigned long filesize;
struct stat stat_buf;
int dsn;

	if (tokencheck()) /* Duplicate submission - message was already sent */
	{
		sendmsg_done();
		return (1);
	}

	if (strcmp(cgi("form"), "doattach") == 0)
	{
		/* When called from the attachment window, we do NOT create
		** a new draft message */

		draftmessage=strdup(origdraft);
	}
	else
		draftmessage=newmsg_createdraft(origdraft);
	if (!draftmessage)
		enomem();

	filename=newmsg_createsentmsg(draftmessage, &isgpgerr, 1);

	if (!filename)
	{
		char *draftbase=maildir_basename(draftmessage);

		if (isgpgerr)
		{
			cgi_put("draftmessage", draftbase);
			output_form("gpgerr.html");
		}
		else
		{
			http_redirect_argss("&form=newmsg&pos=%s"
					    "&draft=%s&error=quota",
					    cgi("pos"), draftbase);
		}
		free(draftmessage);
		free(draftbase);
		return (1);
	}

	if (pipe(pipefd1) != 0)
	{
		cgi_put("foldermsg", "ERROR: pipe() failed.");
		maildir_msgpurgefile(SENT, filename);
		free(filename);
		free(draftmessage);
		return (0);
	}

	returnaddr=login_returnaddr();

	dsn= *cgi("dsn") != 0;

	pid=fork();
	if (pid < 0)
	{
		cgi_put("foldermsg", "ERROR: fork() failed.");
		close(pipefd1[0]);
		close(pipefd1[1]);
		maildir_msgpurgefile(SENT, filename);
		free(filename);
		free(draftmessage);
		return (0);
	}

	if (pid == 0)
	{
	static const char noexec[]="ERROR: Unable to execute sendit.sh.\n";
	static const char nofile[]="ERROR: Temp file not available - probably exceeded quota.\n";
	char	*tmpfile=maildir_find(SENT, filename);
	int	fd;

		if (!tmpfile)
		{
			fwrite((char*)nofile, 1, sizeof(nofile)-1, stderr);
			_exit(1);
		}

		close(0);

		fd=maildir_safeopen(tmpfile, O_RDONLY, 0);
		close(1);
		close(2);
		dup(pipefd1[1]);
		dup(pipefd1[1]);
		close(pipefd1[0]);
		close(pipefd1[1]);

		if (dsn)
			putenv("DSN=-Nsuccess,delay,fail");
		else
			putenv("DSN=");

		if (fd == 0)
			execl(SENDITSH, "sendit.sh", returnaddr,
				sqwebmail_mailboxid, NULL);

		fwrite(noexec, 1, sizeof(noexec)-1, stderr);
		_exit(1);
	}
	close(pipefd1[1]);

	line=geterrbuf(pipefd1[0]);
	close(pipefd1[0]);

	if (waitfor(pid))
	{
		if (!*line)
			line="Unable to send message.\n";
	}
	else
		line="";

	if (*line == 0)	/* Succesfully sent message */
	{
		if (*draftmessage)
		{
		char	*base=maildir_basename(draftmessage);
		char	*draftfile=maildir_find(DRAFTS, base);

			free(base);

			/* Remove draft file */

			if (draftfile)
			{
			char	*replytofolder=0, *replytomsg=0;
			char	*header, *value;
			FILE	*fp;
			int	x;

				fp=0;
				x=maildir_safeopen(draftfile, O_RDONLY, 0);
				if ( maildir_parsequota(draftfile, &filesize))
				{
					if (x < 0 || fstat(x, &stat_buf))
						stat_buf.st_size=0;
					filesize=stat_buf.st_size;
				}

				if (x >= 0)
					if ((fp=fdopen(x, "r")) == 0)
						close(x);

				/* First, look for a message that we should
				** mark as replied */

				while (fp && (header=maildir_readheader(fp,
						&value, 0)) != 0)
				{
					if (strcmp(header,"x-reply-to-folder")
						== 0 && !replytofolder)
					{
						replytofolder=strdup(value);
						if (!replytofolder)
							enomem();
					}
					if (strcmp(header,"x-reply-to-msg")
						== 0 && !replytomsg)
					{
						replytomsg=strdup(value);
						if (!replytomsg)
							enomem();
					}
					if (replytofolder && replytomsg)
						break;
				}
				if (fp)	fclose(fp);

				if (replytofolder && replytomsg)
					maildir_markreplied(replytofolder,
							replytomsg);
				if (replytofolder)	free(replytofolder);
				if (replytomsg)	free(replytomsg);
				
				maildir_quota_deleted(".",
						      -(long)filesize, -1);

				unlink(draftfile);
				free(draftfile);
			}
		}

		tokensave();

		if (*cgi("fcc") == 0)
		{
			unsigned long filesize=0;
			char	*tmpfile=maildir_find(SENT, filename);

			if (tmpfile)
			{
				maildir_parsequota(tmpfile, &filesize);
				unlink(tmpfile);
				maildir_quota_deleted(".", -(long)filesize,-1);
				free(tmpfile);
			}
		}

		free(filename);
		free(draftmessage);
		sendmsg_done();
		return (1);
	}

	if (stat(filename, &stat_buf) == 0)
		maildir_quota_deleted(".", -(long)stat_buf.st_size, -1);
	maildir_msgpurgefile(SENT, filename);
	free(filename);

	{
	char *draftbase=maildir_basename(draftmessage);

		http_redirect_argsss("&form=newmsg&pos=%s&draft=%s&foldermsg=%s",
			cgi("pos"), draftbase, line);
		free(draftmessage);
		free(draftbase);
	}
	return (1);
}

void newmsg_do(const char *folder)
{
const	char *draftmessage=cgi("draftmessage");

	if (*draftmessage)	/* It's ok if it's blank */
	{
		CHECKFILENAME(draftmessage);
	}

	if (*cgi("savedraft"))
	{
	char	*newdraft=newmsg_createdraft(draftmessage);

		if (!newdraft)	enomem();
		free(newdraft);
		sendmsg_done();
		return;
	}

	if (*cgi("sendmsg") && dosendmsg(draftmessage))
		return;

	if (*cgi("doattachments"))
	{
	char	*newdraft=newmsg_createdraft(draftmessage);
	char	*base;

		if (!newdraft)	enomem();
		if (*cgi("error"))
		{
			cgi_put("previewmsg", "1");
			output_form("newmsg.html");
			return;
		}

		base=maildir_basename(newdraft);
		http_redirect_argss("&form=attachments&pos=%s&draft=%s",
			cgi("pos"), base);
		free(base);
		free(newdraft);
		return;
	}
#ifdef	ISPELL
	if (*cgi("startspellchk"))
	{
	char	*newdraft=newmsg_createdraft(draftmessage);
	char	*base;

		if (!newdraft)	enomem();
		base=maildir_basename(newdraft);
		free(newdraft);
		if (spell_start(base) == 0)
		{
			cgi_put("draftmessage", base);
			output_form("spellchk.html");
		}
		else
		{
			http_redirect_argss("&form=newmsg&pos=%s&draft=%s&previewmsg=SPELLCHK",
				cgi("pos"), base);
		}
		free(base);
		return;
	}
#endif
	if (*ispreviewmsg()||cgi("return"))
	{
		output_form("newmsg.html");
		return;
	}
	http_redirect_argsss("&form=newmsg&pos=%s&draftmessage=%s&error=%s",
		cgi("pos"), draftmessage,
		cgi("error"));
}
