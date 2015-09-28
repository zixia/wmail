/*
** Copyright 1998 - 2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: folder.c,v 1.11 2004/01/07 00:43:11 roy Exp $
*/
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<stdlib.h>
#include	"config.h"
#include	"sqwebmail.h"
#include	"maildir/maildirmisc.h"
#include	"maildir/maildircreate.h"
#include	"rfc822/rfc822.h"
#include	"rfc822/rfc2047.h"
#include	"rfc2045/rfc2045.h"
#include	"rfc2045/rfc2646.h"
#include	"rfc2646html.h"
#include	"md5/md5.h"
#include	"gpglib/gpglib.h"
#include	"maildir.h"
#include	"mailfilter.h"
#include	"maildir/maildirquota.h"
#include	"maildir/maildirgetquota.h"
#include	"numlib/numlib.h"
#include	"folder.h"
#include	"addressbook.h"
#include	"cgi/cgi.h"
#include	"pref.h"
#include	"html.h"
#include	"token.h"
#include	"filter.h"
#include	"buf.h"
#include	"pref.h"
#include	"newmsg.h"
#include	"htmllibdir.h"
#include	"gpg.h"

#if	TIME_WITH_SYS_TIME
#include	<sys/time.h>
#include	<time.h>
#else
#if	HAVE_SYS_TIME_H
#include	<sys/time.h>
#else
#include	<time.h>
#endif
#endif
#if	HAVE_SYS_STAT_H
#include	<sys/stat.h>
#endif
#include	<errno.h>

extern void print_attrencodedlen(const char *, size_t, int, FILE *);

extern const char *showsize(unsigned long);
extern void maildir_cleanup();
extern const char *nonloginscriptptr();
extern int pref_flagpagesize;
extern int ishttps();
extern const char *sqwebmail_content_charset;

extern time_t rfc822_parsedt(const char *);
static time_t	current_time;

static const char *folder_err_msg=0;

extern const char *sqwebmail_folder;

extern void output_scriptptrget();
extern void output_scriptptr();
extern void output_scriptptrpostinfo();
extern void output_attrencoded(const char *);
extern void output_urlencoded(const char *);
extern char *scriptptrget();

// by lfan
extern char *xlate_mdir(const char *foldername);
extern int countcurnew(const char *, time_t *, off_t *, unsigned *);
void list_folder_xlate(const char *p, const char *n_inbox, const char *n_drafts,
	const char *n_sent, const char *n_trash, const char *n_spam);

void print_safe_len(const char *p, size_t n, void (*func)(const char *, size_t))
{
char	buf[10];
const	char *q=p;

	while (n)
	{
		--n;
		if (*p == '<')	strcpy(buf, "&lt;");
		else if (*p == '>') strcpy(buf, "&gt;");
		else if (*p == '&') strcpy(buf, "&amp;");
		else if (*p == ' ') strcpy(buf, "&nbsp;");
		else if (*p == '\n') strcpy(buf, "<BR>");
		else if (ISCTRL(*p))
			sprintf(buf, "&#%d;", (int)(unsigned char)*p);
		else
		{
			p++;
			continue;
		}

		(*func)(q, p-q);
		(*func)(buf, strlen(buf));
		p++;
		q=p;
	}
	(*func)(q, p-q);
}

static void print_safe_to_stdout(const char *p, size_t cnt)
{
	fwrite(p, cnt, 1, stdout);
}

void print_safe(const char *p)
{
	print_safe_len(p, strlen(p), print_safe_to_stdout);
}

void call_print_safe_to_stdout(const char *p, size_t cnt)
{
	print_safe_len(p, cnt, print_safe_to_stdout);
}
	
void folder_contents_title()
{
const char *lab;
const char *f;
const char *inbox_lab, *drafts_lab, *trash_lab, *sent_lab,*spam_lab;

	lab=getarg("FOLDERTITLE");
	inbox_lab=getarg("INBOX");
	drafts_lab=getarg("DRAFTS");
	trash_lab=getarg("TRASH");
	sent_lab=getarg("SENT");
	spam_lab=getarg("SPAM");

	f=sqwebmail_folder;
	if (strcmp(f, INBOX) == 0)	f=inbox_lab;
	else if (strcmp(f, DRAFTS) == 0)	f=drafts_lab;
	else if (strcmp(f, SENT) == 0)	f=sent_lab;
	else if (strcmp(f, TRASH) == 0)	f=trash_lab;
	else if (strcmp(f, SPAM) == 0 ) f=spam_lab;

	if (lab)
	{
		char *ff, *origff;
		char	*q;

		origff=ff=folder_fromutf7(f);

		q=malloc(strlen(lab)+strlen(ff)+1);

		if (*ff == ':')	++ff;
		if (!q)
		{
			free(ff);
			enomem();
		}
		sprintf(q, lab, ff);
		free(origff);
		output_attrencoded(q);
		free(q);
	}
}

static int group_movedel(const char *folder,
			int (*func)(const char *, const char *, size_t))
{
struct cgi_arglist *arg;

	if (*cgi("SELECTALL"))	/* Everything is selected */
	{
		for (arg=cgi_arglist; arg; arg=arg->next)
		{
		const	char *f;

			if (strncmp(arg->argname, "MOVEFILE-", 9)) continue;
			f=cgi(arg->argname);
			CHECKFILENAME(f);
			if ((*func)(folder, f, atol(arg->argname+9)))
				return (-1);
		}
		return (0);
	}

	for (arg=cgi_arglist; arg; arg=arg->next)
	{
	unsigned long l;
	char	movedel[MAXLONGSIZE+10];
	const	char *f;

		if (strncmp(arg->argname, "MOVE-", 5))	continue;
		l=atol(arg->argname+5);
		sprintf(movedel, "MOVEFILE-%lu", l);
		f=cgi(movedel);
		CHECKFILENAME(f);
		if ((*func)(folder, f, l))
			return (-1);
	}
	return (0);
}

static int groupdel(const char *folder, const char *file, size_t pos)
{
	maildir_msgdeletefile(folder, file, pos);
	return (0);
}

static int groupmove(const char *folder, const char *file, size_t pos)
{
	return (maildir_msgmovefile(folder, file, cgi("moveto"), pos));
}

void folder_delmsgs(const char *dir, size_t pos)
{
int	rc=0;
// by lfan, improve delete message
char    *dir1=xlate_mdir(dir);

	if (*cgi("cmddel"))
	{
		rc=group_movedel( dir, &groupdel );
		maildir_savefoldermsgs(dir);
	}
	else if (*cgi("cmddelall")) {
		maildir_deleteall(dir);
	}
	else if (*cgi("cmdmove"))
	{
		rc=group_movedel( dir, &groupmove );
		maildir_savefoldermsgs(dir);
	}
	maildir_checknew(dir1);
        free(dir1);
	
	maildir_cleanup();

	http_redirect_argu(
		(rc ? "&error=quota&form=folder&pos=%s":"&form=folder&pos=%s"),
			(unsigned long)pos);
}

static void folder_msg_link(const char *, const char *, size_t, char);
static void folder_msg_unlink(const char *, size_t, char);

void folder_contents(const char *dir, size_t pos)
{
MSGINFO	**contents;
int	i, found;
int	morebefore, moreafter;
// by lfan
size_t	msg_new, msg_total;

const char	*beforelab, *afterlab;
const char	*nomsg, *selectalllab;
const char	*qerrmsg;

	maildir_reload(dir);
	contents=maildir_read(dir, pref_flagpagesize, &pos,
		&morebefore, &moreafter);
	// by lfan
	maildir_count(dir, &msg_new, &msg_total);
	msg_total += msg_new;

	time(&current_time);
	nomsg=getarg("NOMESSAGES");
	beforelab=getarg("PREVPAGE");
	afterlab=getarg("NEXTPAGE");
	//selectalllab=getarg("SELECTALL");

	qerrmsg=getarg("PERMERR");

	if (!qerrmsg)	qerrmsg="";

	if (strcmp(cgi("error"), "quota") == 0)
		printf("%s", qerrmsg);

	// by lfan
        
	printf("<TABLE WIDTH=\"100%%\" CLASS=\"csmtype\" BGCOLOR=\"#93BEE2\" BORDER=0 CELLPADDING=2 CELLSPACING=1>\n");
	printf("<TR><TD WIDTH=5%%></TD><TD WIDTH=25%%>%s", getarg("CURPOS"));
	list_folder_xlate(dir, getarg("INBOX"), getarg("DRAFTS"), getarg("SENT"), getarg("TRASH"),getarg("SPAM"));
	printf("</TD><TD WIDTH=25%%>");
	printf(getarg("MAILMSG"), msg_total, msg_new);
	printf("</TD><TD WIDTH=25%%>%s%d &nbsp;", getarg("PAGETOTAL"), 
		(msg_total + pref_flagpagesize - 1) / pref_flagpagesize );
	
	{
		int	nTotal = (msg_total + pref_flagpagesize - 1) / pref_flagpagesize;
		int	i;
		printf(getarg("CURPAGE"));
		printf("<SELECT NAME=\"page\" onChange=\"GotoPage(this.form);\" CLASS=\"myselect\">");
		printf("<OPTION VALUE=%d>%d</OPTION>\n", pos / pref_flagpagesize + 1, 
			pos / pref_flagpagesize + 1);
		for( i = 0; i < nTotal; i++ ) {
			printf("<OPTION VALUE=?&form=folder&pos=%d&folder=", i*pref_flagpagesize);
			printf("%s>%d</OPTION>", cgi("folder"), i+1);
		}
		
		printf("</SELECT>\n");
	}
	printf("</TD><TD WIDTH=10%% ALIGN=RIGHT>");
	
        if (morebefore) {
		size_t  beforepos;

		if (pos < pref_flagpagesize)    beforepos=0;
		else    beforepos=pos-pref_flagpagesize;
		printf("<A HREF=\"");
		output_scriptptrget();
		printf("&form=folder&pos=%ld\">", (long)beforepos);
		printf("%s</A>", beforelab);
	}
	printf("</TD><TD WIDTH=10%% ALIGN=LEFT>");
	if (moreafter) {
		printf("<A HREF=\"");
		output_scriptptrget();
		printf("&form=folder&pos=%ld\">", (long)(pos+pref_flagpagesize));
		printf("%s</A>", afterlab);
	}
	
	printf("</TD></TR></TABLE><BR>\n");
	
	/* by lfan
	printf("<TABLE WIDTH=\"100%%\" CLASS=\"folder-nextprev-background\" BORDER=0 CELLPADDING=0 CELLSPACING=0>");
	printf("<TR><TD ALIGN=LEFT>");
	printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS=\"folder-nextprev-buttons\"><TR><TD>");
	if (morebefore)
	{
	size_t	beforepos;

		if (pos < pref_flagpagesize)	beforepos=0;
		else	beforepos=pos-pref_flagpagesize;

		printf("<A HREF=\"");
		output_scriptptrget();
		printf("&form=folder&pos=%ld\" STYLE=\"text-decoration: none\">",
			(long)beforepos);
	}
	printf("%s", beforelab);
	if (morebefore)
		printf("</A>");

	printf("</TD></TR></TABLE>\n");

	printf("</TD><TD ALIGN=RIGHT>\n");

	printf("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS=\"folder-nextprev-buttons\"><TR><TD>");
	if (moreafter)
	{
		printf("<A HREF=\"");
		output_scriptptrget();
		printf("&form=folder&pos=%ld\" STYLE=\"text-decoration: none\">",
			(long)(pos+pref_flagpagesize));
	}
	printf("%s", afterlab);
	if (moreafter)
		printf("</A>");

	printf("</TD></TR></TABLE>\n");

	printf("</TD></TR></TABLE>");
	*/

	// by lfan
	//printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=4><TR CLASS=\"folder-index-header\"><TH ALIGN=CENTER>%s</TH><TH>&nbsp;</TH><TH>%s</TH><TH>%s</TH><TH>%s</TH><TH>%s</TH></TR>\n",

	printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=1 CELLPADDING=5 BGCOLOR=\"#336699\" CLASS=\"csmtype\">"
	       "<TR ALIGN=LEFT CLASS=\"wmtype\"><TH WIDTH=\"5%%\">%s</TH>"
	       "<TH WIDTH=\"20%%\">%s</TH><TH WIDTH=\"20%%\">%s</TH><TH WIDTH=\"48%%\">%s</TH>"
	       "<TH ALIGN=RIGHT WIDTH=\"7%%\">%s</TH></TR>\n",
	       getarg("STATUS"),
	       getarg("DATE"),
	       getarg("FROM"),
	       getarg("SUBJECT"),
	       getarg("SIZE"));

	found=0;
	for (i=0; i<pref_flagpagesize; i++)
	{
	const char *date, *from, *subj, *size;
	char	*froms, *subjs;
	const char *p, *q;
	// by lfan size_t l;
	char type[8], attflag;
	char *start_bold, *end_bold;

	static const char folder_index_entry_start[]="<FONT COLOR=\"#000000\" STYLE=\"folder-index-message\">";
	static const char folder_index_entry_end[]="</FONT>";

		if (contents[i] == 0)	continue;
		found=1;

		date=MSGINFO_DATE(contents[i]);
		from=MSGINFO_FROM(contents[i]);
		subj=MSGINFO_SUBJECT(contents[i]);
		size=MSGINFO_SIZE(contents[i]);

		type[0]=maildirfile_type(MSGINFO_FILENAME(contents[i]));
		attflag = strstr( MSGINFO_FILENAME(contents[i]), ":2,A" ) ? 1: 0;
		type[1]='\0';
		if (type[0] == '\0')	strcpy(type, "&nbsp;");

		start_bold="<font class=\"read-message\">"; end_bold="</font>";

		if (type[0] == MSGTYPE_NEW)
		{
			// by lfan
			//start_bold="<B CLASS=\"unread-message\">";
			//end_bold="</B>";
			start_bold = " BGCOLOR=#FFF7E5";
		}
		else
			start_bold = " BGCOLOR=#FFFFF8";

		p=MSGINFO_FILENAME(contents[i]);

		if ((q=strrchr(p, '/')) != 0)
			p=q+1;

		// by lfan
		//printf("<TR CLASS=\"folder-index-bg-%d\"><TD ALIGN=RIGHT>%s%s<font class=\"message-number\">%ld.</font>%s%s</TD><TD><INPUT TYPE=CHECKBOX NAME=\"MOVE-%ld",
		printf("<TR %s><TD><INPUT TYPE=CHECKBOX NAME=\"MOVE-%ld",
		       //(i & 1)+1,
		       start_bold,
		       //folder_index_entry_start, start_bold,
		       //(long)(i+pos+1),
		       //end_bold, folder_index_entry_end,
		       (long) (pos+i));
		printf("\"%s><INPUT TYPE=HIDDEN NAME=\"MOVEFILE-%ld\" VALUE=\"",
			(type[0] == MSGTYPE_DELETED ? " DISABLED":""),
			(long)(pos+i));
		output_attrencoded(p);
		printf("\">&nbsp;%s</font></TD><TD>",
		       //folder_index_entry_start,
		       //type,
		       (type[0] == MSGTYPE_NEW ? getarg("UNREADICON"): (type[0] == MSGTYPE_REPLIED ? getarg("REPLIEDICON"):"")) 
		       //folder_index_entry_end,
		       //folder_index_entry_start
			);
		if (!*date)	date=" ";
		//folder_msg_link("message-date", dir, pos+i, type[0]);
		print_safe(date);
		//folder_msg_unlink(dir, pos+i, type[0]);
		printf("</TD><TD>");
		if (!*from)	from=" ";
		folder_msg_link("message-from", dir, pos+i, type[0]);

		froms=rfc2047_decode_simple(from);
		if (froms == 0)	enomem();
		if (strlen(froms) >= 30)
			strcpy(froms+27, "...");
		print_safe(froms);
		free(froms);
		folder_msg_unlink(dir, pos+i, type[0]);
		printf("<BR></TD><TD>");

		folder_msg_link("message-subject", dir, pos+i, type[0]);

		subjs=rfc2047_decode_simple(subj);
		if (subjs == 0)	enomem();
		if (strlen(subjs) >= 40) {
			size_t k;
			for( k = 36; k>=0; k-- )
				if( subjs[k] >= 0 )
					break;
			if( (36-k) % 2  == 1 ) // half char!
				k=38;
			else
				k=37;
			strcpy(subjs+k, "...");
		}

		// by lfan
		if (strlen(subjs) == 0)
			strcpy(subjs, getarg("NOSUB"));
		print_safe(subjs);
		//l=strlen(subjs);
		//while (l++ < 8)
		//	printf("&nbsp;");
		free(subjs);

		folder_msg_unlink(dir, pos+i, type[0]);
		printf("</TD><TD ALIGN=RIGHT>%s&nbsp;%s<BR></TD></TR>\n", size, attflag? getarg("ATTICON"):"");
	}
	
	/* by lfan, move to template
	if (found)
	{
		puts("<TR CLASS=\"folder-index-bg-1\"><TD COLSPAN=6><HR></TD></TR>");
		puts("<TR CLASS=\"folder-index-bg-2\"><TD>&nbsp;</TD>");
		puts("<TR BGCOLOR=\"#FFFFF8\"><TD COLSPAN=6><HR></TD></TR>");
		puts("<TR CLASS=\"folder-index-bg-2\"><TD>&nbsp;</TD>");
		printf("<TD COLSPAN=5><INPUT TYPE=CHECKBOX NAME=\"SELECTALL\">&nbsp;%s</TD></TR>",
			selectalllab);
	}
	*/

	if (!found && nomsg)
	{
		// by lfan
		//puts("<TR CLASS=\"folder-index-bg-1\"><TD COLSPAN=6 ALIGN=LEFT><P>");
		
		puts("<TR BGCOLOR=\"#FFFFF8\"><TD COLSPAN=6 ALIGN=LEFT>");
		puts(nomsg);
		puts("<BR></TD></TR>");
	}

	printf("</TABLE>\n");
	maildir_free(contents, pref_flagpagesize);
}

static void folder_msg_link(const char *style,
			    const char *dir, size_t pos, char t)
{
#if 0
	if (t == MSGTYPE_DELETED)
	{
		printf("<A HREF=\"");
		output_scriptptrget();
		printf("&form=folder&pos=%s\">", cgi("pos"));
		return;
	}
#endif

	printf("<A HREF=\"");
	if (strcmp(dir, DRAFTS))
	{
		output_scriptptrget();
		printf("&form=readmsg&pos=%ld\">", (long)pos);
	}
	else
	{
	size_t	mpos=pos;
	char	*filename=maildir_posfind(dir, &mpos);
	char	*basename=filename ? maildir_basename(filename):NULL;

		output_scriptptrget();
		printf("&form=open-draft&draft=");
		output_urlencoded(basename);
		printf("\">");
		if (basename)	free(basename);
		if (filename)	free(filename);
	}

	printf("<font class=\"%s\">", style);
}

static void folder_msg_unlink(const char *dir, size_t pos, char t)
{
	printf("</font></A>");
}

static size_t	msg_pos, msg_count;
static char	*msg_posfile;
static int	msg_hasprev, msg_hasnext;
static const char	*msg_nextlab=0, *msg_prevlab=0, *msg_deletelab=0,
		*msg_purgelab=0, *msg_folderlab=0;
static char	msg_type;

static const char	*msg_replylab=0;
static const char	*msg_replyalllab=0;
static const char	*msg_replylistlab=0;
static const char	*msg_forwardlab=0;
static const char	*msg_forwardattlab=0;
static const char	*msg_fullheaderlab=0;
static const char	*msg_movetolab=0;
static const char	*msg_print=0;

static const char	*folder_inbox=0;
static const char	*folder_drafts=0;
static const char	*folder_trash=0;
static const char	*folder_sent=0;
static const char	*folder_spam=0;
static const char	*msg_golab=0;

static const char *msg_msglab;
static const char *msg_add=0;

static int	initnextprevcnt;

void folder_initnextprev(const char *dir, size_t pos)
{
	MSGINFO	**info;
	const	char *p;
	const	char *msg_numlab, *msg_numnewlab;
	static char *filename=0;
#if HAVE_SQWEBMAIL_UNICODE
	int fd;
#endif

	cgi_put(MIMEGPGFILENAME, "");

	if (filename)
		free(filename);
	filename=0;

#if HAVE_SQWEBMAIL_UNICODE

	if (*cgi("mimegpg") && (filename=maildir_posfind(dir, &pos)) != 0)
	{
		char *tptr;
		int nfd;

		fd=maildir_semisafeopen(filename, O_RDONLY, 0);

		if (fd >= 0)
		{
			struct maildir_tmpcreate_info createInfo;

			maildir_purgemimegpg();

			maildir_tmpcreate_init(&createInfo);

			createInfo.uniq=":mimegpg:";
			createInfo.doordie=1;

			if ((nfd=maildir_tmpcreate_fd(&createInfo)) < 0)
			{
				free(filename);
				error("Can't create new file.");
			}

			tptr=createInfo.tmpname;
			createInfo.tmpname=NULL;
			maildir_tmpcreate_free(&createInfo);

			chmod(tptr, 0600);

			/*
			** Decrypt/check message into a temporary file
			** that's immediately marked as deleted, so that it
			** gets purged at the next sweep.
			*/

			if (gpgdecode(fd, nfd) < 0)
			{
				close(nfd);
				unlink(tptr);
				free(tptr);
			}
			else
			{
				close(fd);
				free(filename);
				filename=tptr;
				fd=nfd;

				cgi_put(MIMEGPGFILENAME,
					strrchr(filename, '/')+1);
			}
			close(fd);
		}
	}
#endif

	initnextprevcnt=0;
	msg_nextlab=getarg("NEXTLAB");
	msg_prevlab=getarg("PREVLAB");
	msg_deletelab=getarg("DELETELAB");
	msg_purgelab=getarg("PURGELAB");

	msg_folderlab=getarg("FOLDERLAB");

	msg_replylab=getarg("REPLY");
	msg_replyalllab=getarg("REPLYALL");
	msg_replylistlab=getarg("REPLYLIST");

	msg_forwardlab=getarg("FORWARD");
	msg_forwardattlab=getarg("FORWARDATT");

	msg_numlab=getarg("MSGNUM");
	msg_numnewlab=getarg("MSGNEWNUM");

	msg_fullheaderlab=getarg("FULLHDRS");

	msg_movetolab=getarg("MOVETO");
	msg_print=getarg("PRINT");

	folder_inbox=getarg("INBOX");
	folder_drafts=getarg("DRAFTS");
	folder_trash=getarg("TRASH");
	folder_sent=getarg("SENT");
	folder_spam=getarg("SPAM");

	p=getarg("CREATEFAIL");
	if (strcmp(cgi("error"),"quota") == 0)
		printf("%s", p);

	msg_golab=getarg("GOLAB");
	msg_add=getarg("QUICKADD");

	info=maildir_read(dir, 1, &pos, &msg_hasprev, &msg_hasnext);
	msg_pos=pos;

	p=strrchr(MSGINFO_FILENAME(info[0]), '/');
	if (p)	p++;
	else	p=MSGINFO_FILENAME(info[0]);
	msg_posfile=strdup(p);
	if (!msg_posfile)	enomem();

	if ((msg_type=maildirfile_type(MSGINFO_FILENAME(info[0])))
		== MSGTYPE_NEW) msg_numlab=msg_numnewlab;

	msg_msglab=msg_numlab;
	msg_count=maildir_countof(dir);
	maildir_free(info, 1);
}

char *get_msgfilename(const char *folder, size_t *pos)
{
	char *filename;

	if (*cgi(MIMEGPGFILENAME))
	{
		const char *p=cgi(MIMEGPGFILENAME);

		CHECKFILENAME(p);

		filename=malloc(sizeof("tmp/")+strlen(p));
		if (!filename)
			enomem();
		strcat(strcpy(filename, "tmp/"), p);
	}
	else
		filename=maildir_posfind(folder, pos);

	if (!filename)	error("Message not found.");

	return (filename);
}

static void output_mimegpgfilename()
{
	if (*cgi(MIMEGPGFILENAME))
	{
		printf("&" MIMEGPGFILENAME "=");
		output_urlencoded(cgi(MIMEGPGFILENAME));
	}
}

void folder_nextprev()
{
	//printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0 CLASS=\"message-menu-background\"><TR VALIGN=MIDDLE>");

    	//printf("<TD WIDTH=\"90%%\"><TABLE BORDER=0 CELLSPACING=4 CELLPADDING=4 WIDTH=\"100%%\" ALIGN=CENTER><TR VALIGN=TOP>");
        printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=0><TR VALIGN=MIDDLE>");

	printf("<TD ALIGN=CENTER><TABLE BORDER=0 CELLSPACING=5><TR VALIGN=TOP>");
		
	/* REPLY */

	printf("<TD CLASS=\"message-menu-button\" WIDTH=60><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();
	printf("&pos=%ld&reply=1&form=newmsg\">%s</A></TD>\n",
		(long)msg_pos,
		msg_replylab);

	/* REPLY ALL */

	printf("<TD CLASS=\"message-menu-button\" WIDTH=60><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();
	printf("&pos=%ld&replyall=1&form=newmsg\">%s</A></TD>\n",
		(long)msg_pos,
		msg_replyalllab);

	/* REPLY LIST */
	/* by lfan
	printf("<TD CLASS=\"message-menu-button\"><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();
	printf("&pos=%ld&replylist=1&form=newmsg\">%s</A></TD>\n",
		(long)msg_pos,
		msg_replylistlab);

	printf("<TD WIDTH=\"100%%\"></TD></TR></TABLE><TABLE BORDER=0 CELLSPACING=4 CELLPADDING=4><TR>");
	*/

	/* FORWARD */

	printf("<TD CLASS=\"message-menu-button\" WIDTH=60><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();
	printf("&pos=%ld&forward=1&form=newmsg\">%s</A></TD>\n",
		(long)msg_pos,
		msg_forwardlab);

	/* FORWARD AS ATTACHMENT*/
	/* by lfan
	printf("<TD CLASS=\"message-menu-button\" WIDTH=60><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();
	printf("&pos=%ld&forwardatt=1&form=newmsg\">%s</A></TD>\n",
		(long)msg_pos,
		msg_forwardattlab);
	*/

	/* FULL HEADERS */
	/* by lfan
	if (!pref_flagfullheaders && !*cgi("fullheaders"))
	{
		printf("<TD CLASS=\"message-menu-button\" WIDTH=60><A HREF=\"");
		output_scriptptrget();
		output_mimegpgfilename();
		printf("&pos=%ld&form=readmsg&fullheaders=1\">%s</A></TD>\n",
			(long)msg_pos, msg_fullheaderlab);
	}
	*/

	/* PRINT MESSAGE */
	/* by lfan
	printf("<TD CLASS=\"message-menu-button\"><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();

	if (pref_flagfullheaders || *cgi("fullheaders"))
	{
		printf("&pos=%ld&form=print&setcookie=1&fullheaders=1\" TARGET=\"_BLANK\">%s</A></TD>\n", (long)msg_pos, msg_print);
	}
	else
	{
		printf("&pos=%ld&form=print&setcookie=1\" TARGET=\"_BLANK\">%s</A></TD>\n", (long)msg_pos, msg_print);
	}
	*/

	/* SAVE MESSAGE */

	/* by lfan
	printf("<TD CLASS=\"message-menu-button\"><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();

	printf("&pos=%ld&form=fetch&download=1\">%s</A></TD>", (long)msg_pos,
	       getarg("SAVEMESSAGE"));

	*/

        /* DEL */

        printf("<TD CLASS=\"message-menu-button\" WIDTH=60>");
        if (msg_type != MSGTYPE_DELETED)
        {
                printf("<A HREF=\"");
                output_scriptptrget();
                tokennewget();
                printf("&posfile=");
                output_urlencoded(msg_posfile);
                printf("&form=delmsg&pos=%ld\">",
                        (long)msg_pos);
        }
        printf("%s", strcmp(sqwebmail_folder, TRASH) == 0
                ? msg_purgelab : msg_deletelab);

        if (msg_type != MSGTYPE_DELETED)
                printf("</A>");

        printf("</TD>\n");

        /* PREV */

        printf("<TD CLASS=\"message-menu-button\" WIDTH=60>");

        if (msg_hasprev)
        {
                printf("<A HREF=\"");
                output_scriptptrget();
                printf("&form=readmsg&pos=%ld\">",
                        (long)(msg_pos-1));
        }
        
        printf("%s", msg_prevlab ? msg_prevlab:"");
        
        if (msg_hasprev)
        {       
                printf("</A>");
        }
        printf("</TD>");
        
        /* NEXT */
        
        printf("<TD CLASS=\"message-menu-button\" WIDTH=60>");
        
        if (msg_hasnext)
        {       
                printf("<A HREF=\"");
                output_scriptptrget();
                printf("&form=readmsg&pos=%ld\">",
                        (long)(msg_pos+1));
        }
        
        printf("%s", msg_nextlab ? msg_nextlab:"");
        
        if (msg_hasnext)
        {       
                printf("</A>");
        }
        printf("</TD>");

        /* FOLDER */
        
        printf("<TD CLASS=\"message-menu-button\" WIDTH=60><A HREF=\"");
        output_scriptptrget();
        printf("&pos=%ld&form=folder\">%s</A></TD>\n",
                (long)( (msg_pos/pref_flagpagesize)*pref_flagpagesize ),
                msg_folderlab);


    	//printf("<TD WIDTH=100%%></TD></TR></TABLE></TD><TD ALIGN=RIGHT VALIGN=CENTER>");
	printf("</TR></TABLE></TD><TD ALIGN=RIGHT VALIGN=CENTER>");
	
	printf("<TABLE BORDER=0 CELLSPACING=4><TR><TD><FONT color=#000099>&nbsp;");
	printf(msg_msglab, (int)msg_pos+1, (int)msg_count);
	printf("&nbsp;</FONT></TD></TR></TABLE>");
    printf("</TD></TR></TABLE>\n");
}

void list_folder(const char *p)
{
	char *s=folder_fromutf7(p);
	print_safe(s);
	free(s);
}

void list_folder_xlate(const char *p,
		       const char *n_inbox,
		       const char *n_drafts,
		       const char *n_sent,
		       const char *n_trash,
		       const char *n_spam)
{
	if (strcmp(p, INBOX) == 0)
		printf("%s", n_inbox);
	else if (strcmp(p, DRAFTS) == 0)
		printf("%s", n_drafts);
	else if (strcmp(p, TRASH) == 0)
		printf("%s", n_trash);
	else if (strcmp(p, SENT) == 0)
		printf("%s", n_sent);
	else if (strcmp(p, SPAM) == 0)
		printf("%s", n_spam);
	else
		list_folder(p);
}

static void show_transfer_dest(const char *cur_folder)
{
char	**folders;
size_t	i;
const	char *p;
int	has_shared=0;

	maildir_readfolders(&folders);
	for (i=0; folders[i]; i++)
	{
		/* Transferring TO drafts is prohibited */

		if (cur_folder == NULL || strcmp(cur_folder, DRAFTS))
		{
			if (strcmp(folders[i], DRAFTS) == 0)
				continue;
		}
		else
		{
			if (folders[i][0] != ':' && strcmp(folders[i], TRASH))
				continue;
		}

		if (cur_folder && strcmp(cur_folder, folders[i]) == 0)
			continue;

		p=folders[i];

		if (strcmp(p, INBOX) == 0)
			p=folder_inbox;
		else if (strcmp(p, DRAFTS) == 0)
			p=folder_drafts;
		else if (strcmp(p, TRASH) == 0)
			p=folder_trash;
		else if (strcmp(p, SENT) == 0)
			p=folder_sent;
		else if (strcmp(p, SPAM) == 0)
			p=folder_spam;
		if (!p)	p=folders[i];

		if (folders[i][0] == ':')
		{
		char	*d=maildir_shareddir(".", folders[i]+1);
		struct	stat	stat_buf;

			if (!d)
			{
				maildir_freefolders(&folders);
				enomem();
			}
			if (stat(d, &stat_buf))	/* Not subscribed */
			{
				free(d);
				continue;
			}
			free(d);
		}

		if (folders[i][0] == ':' && !has_shared)
		{
			printf("<OPTION VALUE=\"\">\n");
			has_shared=1;
		}
		printf("<OPTION VALUE=\"");
		output_attrencoded(folders[i]);
		printf("\">");

		list_folder(folders[i][0] == ':' ? p+1:p);
		printf("\n");
	}
	maildir_freefolders(&folders);
}

void folder_msgmove()
{
	++initnextprevcnt;
	printf("<TABLE BORDER=0 class=\"box-small-outer\"><TR><TD>\n");
	printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR><TD CLASS=\"folder-move-background\">&nbsp;%s&nbsp;<SELECT NAME=list%d>\n", msg_movetolab, initnextprevcnt);

	show_transfer_dest(sqwebmail_folder);

	printf("</SELECT><INPUT TYPE=SUBMIT %s NAME=move%d VALUE=\"%s\"></TD></TR></TABLE>\n",
		(msg_type == MSGTYPE_DELETED ? "DISABLED":""),
		initnextprevcnt,
		msg_golab ? msg_golab:"");
	printf("<input type=hidden name=pos value=\"%s\">", cgi("pos"));
	printf("<input type=hidden name=posfile value=\"");
	output_attrencoded(msg_posfile ? msg_posfile:"");
	printf("\"></TD></TR></TABLE>\n");
}

void folder_delmsg(size_t pos)
{
MSGINFO	**info;
int	dummy;
const	char *f=cgi("posfile");
size_t	newpos;
int	rc=0;
// by lfan, improve message refresh
char    *dir1=xlate_mdir(sqwebmail_folder);

	CHECKFILENAME(f);

	if (*cgi("move1"))
	{
		rc=maildir_msgmovefile(sqwebmail_folder, f, cgi("list1"), pos);
		maildir_savefoldermsgs(sqwebmail_folder);
	}
	else if (*cgi("move2"))
	{
		rc=maildir_msgmovefile(sqwebmail_folder, f, cgi("list2"), pos);
		maildir_savefoldermsgs(sqwebmail_folder);
	}
	else
	{
		maildir_msgdeletefile(sqwebmail_folder, f, pos);
		maildir_savefoldermsgs(sqwebmail_folder);
	}

	// by lfan, refresh cache
        maildir_checknew(dir1);
	maildir_reload(sqwebmail_folder);
        free(dir1);

	if (rc)
	{
		http_redirect_argu("&form=readmsg&pos=%s&error=quota",
			(unsigned long)pos);
		return;
	}

	newpos=pos+1;
	info=maildir_read(sqwebmail_folder, 1, &newpos, &dummy, &dummy);

	if (info[0] && newpos != pos)
	{
		maildir_free(info, 1);
		http_redirect_argu("&form=readmsg&pos=%s",
						(unsigned long)newpos);
	}
	else
	{
		maildir_free(info, 1);
		http_redirect_argu("&form=folder&pos=%s",
						(unsigned long)pos);
	}
}

static int is_preview_mode()
{
	/* We're in new message window, and we're previewing a draft */

	return (*cgi("showdraft"));
}

static void showmsgrfc822(FILE *, struct rfc2045 *, struct rfc2045id *);
static void showmsgrfc822_body(FILE *, struct rfc2045 *, struct rfc2045id *, int);

#if HAVE_SQWEBMAIL_UNICODE
static void dokeyimport(FILE *, struct rfc2045 *, int);
#endif

void folder_showmsg(const char *dir, size_t pos)
{
	char	*filename;
	FILE	*fp;
	struct	rfc2045 *rfc;
	char	buf[BUFSIZ];
	int	n;
	int	fd;

	if (*cgi("addnick"))
	{
		const char *name=cgi("newname");
		const char *addr=cgi("newaddr");

		const char *nick1=cgi("newnick1");
		const char *nick2=cgi("newnick2");

		while (*nick1 && isspace((int)(unsigned char)*nick1))
		       ++nick1;

		while (*nick2 && isspace((int)(unsigned char)*nick2))
		       ++nick2;

		if (*nick2)
			nick1=nick2;

		if (*nick1)
		{
			ab_add(name, addr, nick1);
		}
	}

	filename=get_msgfilename(dir, &pos);

	fp=0;
	fd=maildir_semisafeopen(filename, O_RDONLY, 0);
	if (fd >= 0)
	{
		if ((fp=fdopen(fd, "r")) == 0)
			close(fd);
	}

	if (!fp)
	{
		free(filename);
		return;
	}

	msg_pos=pos;
	rfc=rfc2045_alloc();

	while ((n=fread(buf, 1, sizeof(buf), fp)) > 0)
		rfc2045_parse(rfc, buf, n);
	rfc2045_parse_partial(rfc);

	showmsgrfc822_body(fp, rfc, NULL, 0);
	rfc2045_free(rfc);
	fclose(fp);
	if (*cgi(MIMEGPGFILENAME) == 0)
		maildir_markread(dir, pos);
	free(filename);
}

void folder_keyimport(const char *dir, size_t pos)
{
	char	*filename;
	FILE	*fp;
	struct	rfc2045 *rfc;
	int	fd;

	filename=get_msgfilename(dir, &pos);

	fp=0;
	fd=maildir_semisafeopen(filename, O_RDONLY, 0);
	if (fd >= 0)
	{
		if ((fp=fdopen(fd, "r")) == 0)
			close(fd);
	}

	if (!fp)
	{
		free(filename);
		return;
	}

	rfc=rfc2045_fromfp(fp);


#if HAVE_SQWEBMAIL_UNICODE
	if (has_gpg(GPGDIR) == 0)
	{
		struct rfc2045 *part;

		if (*cgi("pubkeyimport")
		    && (part=rfc2045_find(rfc, cgi("keymimeid"))) != 0)
		{
			dokeyimport(fp, part, 0);
		}
		else if (*cgi("privkeyimport")
		    && (part=rfc2045_find(rfc, cgi("keymimeid"))) != 0)
		{
			dokeyimport(fp, part, 1);
		}
	}
#endif
	rfc2045_free(rfc);
	fclose(fp);
	free(filename);

	printf("<p><a href=\"");
	output_scriptptrget();
	printf("&form=readmsg&pos=%s", cgi("pos"));
	printf("\">%s</a>", getarg("KEYIMPORT"));
}

#if HAVE_SQWEBMAIL_UNICODE
static int importkey_func(const char *p, size_t cnt, void *voidptr);
static int importkeyin_func(const char *p, size_t cnt, void *voidptr);

static void dokeyimport(FILE *fp, struct rfc2045 *rfcp, int issecret)
{
	off_t	start_pos, end_pos, start_body, ldummy;
	char buf[BUFSIZ];
	int cnt;

	static const char start_str[]=
		"<TABLE WIDTH=\"100%%\" BORDER=0 class=\"box-outer\"><TR><TD>"
		"<TABLE WIDTH=\"100%%\" BORDER=0 CELLSPACING=0 CELLPADDING=4"
		" class=\"box-white-outer\"><TR><TD>%s<PRE>\n";

	static const char end_str[]=
		"</PRE></TD></TR></TABLE></TD></TR></TABLE><BR>\n";

	if (gpg_import_start(GPGDIR, issecret))
		return;

	printf(start_str, getarg("IMPORTHDR"));

	rfc2045_mimepos(rfcp, &start_pos, &end_pos, &start_body,
		&ldummy, &ldummy);
	if (fseek(fp, start_body, SEEK_SET) < 0)
	{
		error("Seek error.");
		gpg_import_finish(&importkey_func, NULL);
		printf("%s", end_str);
		return;
	}

	rfc2045_cdecode_start(rfcp, &importkeyin_func, 0);

	while (start_body < end_pos)
	{
		cnt=sizeof(buf);
		if (cnt > end_pos-start_body)
			cnt=end_pos-start_body;
		cnt=fread(buf, 1, cnt, fp);
		if (cnt <= 0)	break;
		start_body += cnt;
		if (rfc2045_cdecode(rfcp, buf, cnt))
		{
			rfc2045_cdecode_end(rfcp);
			printf("%s", end_str);
			return;
		}
	}

	if (rfc2045_cdecode_end(rfcp) == 0)
	{
		gpg_import_finish(&importkey_func, NULL);
	}

	printf("%s", end_str);
}

static int importkeyin_func(const char *p, size_t cnt, void *voidptr)
{
	return (gpg_import_do(p, cnt, &importkey_func, NULL));
}

static int importkey_func(const char *p, size_t cnt, void *voidptr)
{
	print_attrencodedlen(p, cnt, 1, stdout);
	return (0);
}

static void showkey(FILE *, struct rfc2045 *, struct rfc2045id *);

#endif

static void showtextplain(FILE *, struct rfc2045 *, struct rfc2045id *);
static void showdsn(FILE *, struct rfc2045 *, struct rfc2045id *);
static void showtexthtml(FILE *, struct rfc2045 *, struct rfc2045id *);
static void showmultipart(FILE *, struct rfc2045 *, struct rfc2045id *);
static void showmsgrfc822(FILE *, struct rfc2045 *, struct rfc2045id *);
static void showunknown(FILE *, struct rfc2045 *, struct rfc2045id *);

static void (*get_known_handler(struct rfc2045 *mime))(FILE *, struct rfc2045 *, struct rfc2045id *)
{
const char	*content_type, *dummy;

	rfc2045_mimeinfo(mime, &content_type, &dummy, &dummy);
	if (strncmp(content_type, "multipart/", 10) == 0)
		return ( &showmultipart );

#if HAVE_SQWEBMAIL_UNICODE
	if (strcmp(content_type, "application/pgp-keys") == 0
	    && has_gpg(GPGDIR) == 0)
		return ( &showkey );
#endif

	if (mime->content_disposition
	    && strcmp(mime->content_disposition, "attachment") == 0)
		return (0);

	if (strcmp(content_type, "text/plain") == 0 ||
	    strcmp(content_type, "text/rfc822-headers") == 0 ||
	    strcmp(content_type, "text/x-gpg-output") == 0)
		return ( &showtextplain );
	if (strcmp(content_type, "message/delivery-status") == 0)
		return ( &showdsn);
	if (pref_showhtml && strcmp(content_type, "text/html") == 0)
		return ( &showtexthtml );
	if (strcmp(content_type, "message/rfc822") == 0)
		return ( &showmsgrfc822);

	return (0);
}

static void (*get_handler(struct rfc2045 *mime))(FILE *, struct rfc2045 *, struct rfc2045id *)
{
void (*func)(FILE *, struct rfc2045 *, struct rfc2045id *);

	if ((func=get_known_handler(mime)) == 0)
		func= &showunknown;

	return (func);
}

static void showmsgrfc822(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
	if (rfc->firstpart)
		showmsgrfc822_body(fp, rfc->firstpart, id, 1);
}

static void showmsgrfc822_header(const char *);
static void showmsgrfc822_addressheader(const char *);
static void showmsgrfc2369_header(const char *);

static int isaddressheader(const char *header)
{
	return (strcmp(header, "to") == 0 ||
		strcmp(header, "cc") == 0 ||
		strcmp(header, "from") == 0 ||
		strcmp(header, "sender") == 0 ||
		strcmp(header, "resent-to") == 0 ||
		strcmp(header, "resent-cc") == 0 ||
		strcmp(header, "reply-to") == 0);
}


static void showmimeid(struct rfc2045id *idptr, const char *p)
{
	if (!p)
		p="&mimeid=";

	while (idptr)
	{
		printf("%s%d", p, idptr->idnum);
		idptr=idptr->next;
		p=".";
	}
}

static void digestaction(struct rfc2045id *idptr)
{
	printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0><TR><TD><A HREF=\"");
	output_scriptptrget();
	output_mimegpgfilename();
	showmimeid(idptr, NULL);
	printf("&pos=%ld&reply=1&form=newmsg\"><FONT SIZE=\"-1\">%s</FONT></A>&nbsp;</TD><TD>&nbsp;<A HREF=\"",
		(long)msg_pos, msg_replylab);

	output_scriptptrget();
	output_mimegpgfilename();
	showmimeid(idptr, NULL);
	printf("&pos=%ld&replyall=1&form=newmsg\"><FONT SIZE=\"-1\">%s</FONT></A>&nbsp;</TD><TD>&nbsp;<A HREF=\"",
		(long)msg_pos, msg_replyalllab);
	output_scriptptrget();
	output_mimegpgfilename();
	showmimeid(idptr, NULL);
	printf("&pos=%ld&forward=1&form=newmsg\"><FONT SIZE=\"-1\">%s</FONT></A>&nbsp;</TD><TD>&nbsp;<A HREF=\"",
		(long)msg_pos, msg_forwardlab);

	output_scriptptrget();
	output_mimegpgfilename();
	showmimeid(idptr, NULL);
	printf("&pos=%ld&forwardatt=1&form=newmsg\"><FONT SIZE=\"-1\">%s</FONT></A></TD></TR></TABLE>\n",
		(long)msg_pos, msg_forwardattlab);
}

/* Prettify header name by uppercasing the first character. */

void header_uc(char *h)
{
	while (*h)
	{
		*h=toupper( (int)(unsigned char) *h);
		while (*h)
		{
			if (*h++ == '-')	break;
		}
	}
}

void print_header_uc(char *h)
{	
char *hdrname;
const char *hdrvalue;
int count;

	header_uc(h);
	if ((hdrname=malloc(strlen("DSPHDR_")+strlen(h)+1)) == NULL)	enomem();
	strcpy (hdrname, "DSPHDR_");
	strcat (hdrname, h);
	count = 0;
	while (*(hdrname + count) != 0)
	{
		*(hdrname + count) = toupper (*(hdrname + count));
		++count;
	}
	hdrvalue = getarg (hdrname);
	if (*hdrvalue != 0) h = (char *) hdrvalue;
	printf("<TR VALIGN=BASELINE><TD ALIGN=RIGHT CLASS=\"message-rfc822-header-name\">%s:</TD><TD WIDTH=10></TD>",
	       h);
	free (hdrname);
}

static void showmsgrfc822_body(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *idptr, int flag)
{
char	*header, *value;
char	*save_subject=0;
char	*save_date=0;
off_t	start_pos, end_pos, start_body;
struct	rfc2045id *p, newpart;
off_t	dummy;
off_t	pos;

	rfc2045_mimepos(rfc, &start_pos, &end_pos, &start_body, &dummy, &dummy);
	if (fseek(fp, start_pos, SEEK_SET) < 0)
	{
		error("Seek error.");
		return;
	}

	// by lfan
	//printf("<P><TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 CLASS=\"message-rfc822-header\">\n");
	printf("<P><TABLE BORDER=0 CELLPADDING=5 CELLSPACING=1 BGCOLOR=\"#93BEE2\" WIDTH=\"100%%\">\n");
	printf("<TR><TD><TABLE>\n");

	pos=start_pos;
	while ((header=maildir_readheader_mimepart(fp, &value, 1,
		&pos, &start_body)) != 0)
	{
		if (strcmp(header, "list-help") == 0 ||
			strcmp(header, "list-subscribe") == 0 ||
			strcmp(header, "list-unsubscribe") == 0 ||
			strcmp(header, "list-owner") == 0 ||
			strcmp(header, "list-archive") == 0 ||
			strcmp(header, "list-post") == 0)
		{
			print_header_uc(header);
			printf("<TD><TT CLASS=\"message-rfc822-header-contents\">");
			showmsgrfc2369_header(value);
			printf("</TT></TD></TR>\n");
			continue;
		}

		if (pref_flagfullheaders || *cgi("fullheaders"))
		{
		int	isaddress=isaddressheader(header);

			print_header_uc(header);
			printf("<TD><TT CLASS=\"message-rfc822-header-contents\">");
			if (isaddress)
				showmsgrfc822_addressheader(value);
			else
				showmsgrfc822_header(value);
			printf("</TT></TD></TR>\n");
			continue;
		}
		if (strcmp(header, "subject") == 0)
		{
			if (save_subject)	free(save_subject);

			save_subject=rfc2047_decode_enhanced(value,
				sqwebmail_content_charset);
			if (save_subject == 0)
				enomem();
			continue;
		}
		if (strcmp(header, "date") == 0)
		{
			if (save_date)	free(save_date);
			if ((save_date=malloc(strlen(value)+1)) == 0)
				enomem();
			strcpy(save_date, value);
			continue;
		}
		if (isaddressheader(header))
		{
			print_header_uc(header);
			printf("<TD WIDTH=480 style=\"border-bottom: solid #769DB5 1px;\"><TT CLASS=\"message-rfc822-header-contents\">");
			showmsgrfc822_addressheader(value);
			printf("</TT></TD></TR>\n");
		}
	}

	if (save_date)
	{
	time_t	t=rfc822_parsedt(save_date);
	struct tm *tmp=t ? localtime(&t):0;
	char	date_buf[100];

		if (tmp)
		{
			char date_header[10];
			const char *date_fmt;

			date_fmt = getarg ("DSPFMT_DATE");
			if (*date_fmt == 0)
				date_fmt = "%d %b %Y, %I:%M:%S %p";
			

			strcpy(date_header, "Date");
			print_header_uc(date_header);
			strftime(date_buf, sizeof(date_buf)-1, date_fmt, tmp);
			date_buf[sizeof(date_buf)-1]=0;
			printf("<TD WIDTH=480 style=\"border-bottom: solid #769DB5 1px;\"><TT CLASS=\"message-rfc822-header-contents\">");
			showmsgrfc822_header(date_buf);
			printf("</TT></TD></TR>\n");
		}
		free(save_date);
	}

	if (save_subject)
	{
		char subj_header[20];

		strcpy(subj_header, "Subject");
		print_header_uc(subj_header);

		printf("<TD><TT CLASS=\"message-rfc822-header-contents\">");
		showmsgrfc822_header(save_subject);
		printf("</TT></TD></TR>\n");
	}

	if (flag && !is_preview_mode())
	{
		printf("<TR VALIGN=TOP><TD>&nbsp;</TD><TD ALIGN=LEFT VALIGN=TOP>");
		digestaction(idptr);
		printf("</TD></TR>\n");
	}

	// by lfan
	//printf("</TABLE>\n<HR WIDTH=\"100%%\">\n");
	printf("</TABLE></TD></TR></TABLE>\n");

#if HAVE_SQWEBMAIL_UNICODE
	if (!flag && has_gpg(GPGDIR) == 0 && gpgmime_has_mimegpg(rfc))
	{
		printf("<form method=post action=\"");
		output_scriptptr();
		printf("\">");
		output_scriptptrpostinfo();
		printf("<input type=hidden name=form value=\"readmsg\">");
		printf("<input type=hidden name=pos value=\"%s\">",
		       cgi("pos"));
		printf("<input type=hidden name=mimegpg value=1>\n");

		printf("<table border=0 cellpadding=1"
		       " width=\"100%%\" class=\"box-outer\">"
		       "<tr><td><table width=\"100%%\" border=0 cellspacing=0"
		       " cellpadding=0 class=\"box-white-outer\"><tr><td>");

		if ( *cgi(MIMEGPGFILENAME))
		{
			printf("%s", getarg("NOTCOMPACTGPG"));
		}
		else
		{
			printf("%s\n", getarg("MIMEGPGNOTICE"));

			if (ishttps())
				printf("%s\n", getarg("PASSPHRASE"));

			printf("%s", getarg("DECRYPT"));
		}
		printf("</td><tr></table></td></tr></table></form><br>\n");
	}
#endif
	printf("<TABLE WIDTH=\"100%%\" BGCOLOR=\"#d6e7ef\"><TR><TD HEIGHT=10 COLSPAN=3></TD></TR>"
		"<TR><TD WIDTH=15></TD><TD>");
	if (!idptr)
	{
		idptr= &newpart;
		p=0;
	}
	else
	{
		for (p=idptr; p->next; p=p->next)
			;
		p->next=&newpart;
	}
	newpart.idnum=1;
	newpart.next=0;
	(*get_handler(rfc))(fp, rfc, idptr);
	if (p)
		p->next=0;
	printf("</TD><TD WIDTH=15></TD></TR></TABLE>");
}

static void showmsgrfc822_headerp(const char *p, size_t l)
{
	fwrite(p, 1, l, stdout);
}

static void showmsgrfc822_header(const char *p)
{
	filter_start(FILTER_FOR_DISPLAY, showmsgrfc822_headerp);
	filter(p, strlen(p));
	filter_end();
}

struct showaddrinfo {
	struct rfc822a *a;
	int curindex;
	int isfirstchar;
} ;

static void showaddressheader_printc(char c, void *p)
{
	struct showaddrinfo *sai= (struct showaddrinfo *)p;

	if (sai->isfirstchar)
	{
		char *name=0;
		char *addr=0;

		if (sai->curindex < sai->a->naddrs)
		{
			name=rfc822_getname(sai->a, sai->curindex);
			addr=rfc822_getaddr(sai->a, sai->curindex);
		}

		if (!is_preview_mode())
		{
			printf("<A HREF=\"");
			output_scriptptrget();
			printf("&form=quickadd&pos=%s&newname=",
			       cgi("pos"));

			if (name)
			{
				char *v=rfc2047_decode_enhanced(name,
								sqwebmail_content_charset);
				if (v)
				{
					output_urlencoded(v);
					free(v);
				}
				free(name);
			}
			printf("&newaddr=");
			if (addr)
			{
				output_urlencoded(addr);
				free(addr);
			}

			printf("\" STYLE=\"text-decoration: none\" "
			       "ONMOUSEOVER=\"window.status='%s'; return true;\" "
			       "ONMOUSEOUT=\"window.status=''; return true;\" >"
			       "<FONT CLASS=\"message-rfc822-header-address\">",
			       msg_add ? msg_add:"");
		}

		sai->isfirstchar=0;
	}

	print_safe_len(&c, 1, print_safe_to_stdout);
}

static void showaddressheader_printsep(const char *sep, void *p)
{
	struct showaddrinfo *sai= (struct showaddrinfo *)p;

	printf("</FONT>%s",
	       is_preview_mode() ? "":"</A>");

	if (sai)
	{
		sai->curindex++;
		sai->isfirstchar=1;
	}

	printf("%s", sep);
}

static void showaddressheader_printsep_plain(const char *sep, void *p)
{
	printf("%s", sep);
}

static void showmsgrfc822_addressheader(const char *p)
{
	struct	rfc822t *rfcp;
	struct  rfc822a *rfca;

	struct showaddrinfo sai;

	rfcp=rfc822t_alloc_new(p, NULL, NULL);
	if (!rfcp)	enomem();
	rfca=rfc822a_alloc(rfcp);
	if (!rfca)
	{
		rfc822t_free(rfcp);
		enomem();
	}

	sai.a=rfca;
	sai.curindex=0;
	sai.isfirstchar=1;

	rfc2047_print(rfca, sqwebmail_content_charset,
		showaddressheader_printc, showaddressheader_printsep, &sai);
	if (!sai.isfirstchar)
		showaddressheader_printsep("", &sai);
	/* This closes the final </a> */


	rfc822a_free(rfca);
	rfc822t_free(rfcp);
}

static void showrfc2369_printheader(char c, void *p)
{
	p=p;
	putchar(c);
}

static char *get_textlink(const char *);

static void showmsgrfc2369_header(const char *p)
{
struct	rfc822t *rfcp;
struct  rfc822a *rfca;
int	i;

	rfcp=rfc822t_alloc_new(p, NULL, NULL);
	if (!rfcp)	enomem();
	rfca=rfc822a_alloc(rfcp);
	if (!rfca)	enomem();
	for (i=0; i<rfca->naddrs; i++)
	{
	char	*p=rfc822_getaddr(rfca, i);
	char	*q=get_textlink(p);

		if (rfca->addrs[i].tokens)
		{
			rfca->addrs[i].tokens->token=0;
			if (*q)
				free(p);
			else
			{
			struct	buf b;

				buf_init(&b);
				free(q);
				for (q=p; *q; q++)
				{
				char	c[2];

					switch (*q)	{
					case '<':
						buf_cat(&b, "&lt;");
						break;
					case '>':
						buf_cat(&b, "&gt;");
						break;
					case '&':
						buf_cat(&b, "&amp;");
						break;
					case ' ':
						buf_cat(&b, "&nbsp;");
						break;
					default:
						c[1]=0;
						c[0]=*q;
						buf_cat(&b, c);
						break;
					}
				}
				free(p);
				q=strdup(b.ptr ? b.ptr:"");
				buf_free(&b);
				if (!q)	enomem();
			}
			rfca->addrs[i].tokens->ptr=q;
			rfca->addrs[i].tokens->len=strlen(q);
			rfca->addrs[i].tokens->next=0;
		}
		else
		{
			free(q);
			free(p);
		}
	}

	rfc822_print(rfca, showrfc2369_printheader,
				showaddressheader_printsep_plain, NULL);
	for (i=0; i<rfca->naddrs; i++)
		if (rfca->addrs[i].tokens)
			free((char *)rfca->addrs[i].tokens->ptr);

	rfc822a_free(rfca);
	rfc822t_free(rfcp);
}

static void output_mimeurl(struct rfc2045id *id, const char *form)
{
	output_scriptptrget();
	printf("&form=%s&pos=%ld", form, (long)msg_pos);
	showmimeid(id, NULL);

	output_mimegpgfilename();
}

static void showattname(const char *fmt, const char *name,
	const char *content_type);


static void showunknown(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
const char	*content_type, *dummy;
off_t start_pos, end_pos, start_body;
off_t dummy2;
char	*content_name;

	id=id;
	rfc2045_mimeinfo(rfc, &content_type, &dummy, &dummy);

	/* Punt for image/ MIMEs */

	if (strncmp(content_type, "image/", 6) == 0 &&
		(rfc->content_disposition == 0
		 || strcmp(rfc->content_disposition, "attachment")))
	{
		if (!is_preview_mode())
		{
			printf("<A HREF=\"");
			output_mimeurl(id, "fetch");
			printf("\" TARGET=\"_blank\">");
		}
		printf("<IMG SRC=\"");
		output_mimeurl(id, "fetch");
		printf("\" ALT=\"Inline picture: ");
		output_attrencoded(content_type);
		printf("\">%s\n",
		       is_preview_mode() ? "":"</A>");
		return;
	}

	if (rfc2231_udecodeType(rfc, "name", NULL,
				&content_name) < 0)
		content_name=NULL;

	dummy=content_name;

	rfc2045_mimepos(rfc, &start_pos, &end_pos, &start_body,
			&dummy2, &dummy2);

	// by lfan
	//printf("<TABLE BORDER=0 CELLPADDING=1 CELLSPACING=0 CLASS=\"box-small-outer\"><TR><TD>");
	printf("<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=0 CLASS=\"message-download-attachment\"><TR><TD>");

	if (strcmp(cgi("form"), "print") == 0)
	{
		showattname(getarg("ATTSTUB"), dummy, content_type);
	}
	else
	{
		printf("<CENTER><FONT CLASS=\"message-attachment-header\">");
		showattname(getarg("ATTACHMENT"), dummy, content_type);

		//printf("&nbsp;(%s)</FONT></CENTER>",
		printf("&nbsp;(%s)</FONT> &nbsp;",
		       showsize(end_pos - start_body));
		//by lfan
		//printf("<BR><CENTER>");
		//printf("<CENTER>");
		
/* remed by roy 2003.9.28
		if (!is_preview_mode())
		{
			printf("<A HREF=\"");
			output_mimeurl(id, "fetch");
			printf("\" STYLE=\"text-decoration: none\" TARGET=\"_blank\">");
		}

		printf("%s%s&nbsp;/&nbsp;", getarg("DISPATT"),
		       is_preview_mode() ? "":"</A>");
*/
		if (!is_preview_mode())
		{
			printf("<A HREF=\"");
			output_mimeurl(id, "fetch");
			printf("&download=1\" STYLE=\"text-decoration: none\">");
		}

		printf("%s%s</CENTER>\n", getarg("DOWNATT"),
		       is_preview_mode() ? "":"</A>");
	}

	printf("</TD></TR></TABLE>\n");
	// by lfan
	//printf("</TD></TR></TABLE>\n");

	if (content_name)
		free(content_name);
}

#if HAVE_SQWEBMAIL_UNICODE
static void showkey(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
	printf("<TABLE BORDER=0 CELLPADDING=8 CELLSPACING=1 CLASS=\"box-small-outer\"><TR><TD>");
	printf("<TABLE BORDER=0 CELLPADDING=4 CELLSPACING=4 CLASS=\"message-application-pgpkeys\"><TR><TD>");

	if (strcmp(cgi("form"), "print") == 0 || is_preview_mode())
	{
		printf("%s", getarg("KEY"));
	}
	else
	{
		printf("<CENTER><A HREF=\"");
		output_scriptptrget();
		printf("&form=keyimport&pos=%ld", (long)msg_pos);
		printf("&pubkeyimport=1");
		output_mimegpgfilename();
		showmimeid(id, "&keymimeid=");
		printf("\" STYLE=\"text-decoration: none\" CLASS=\"message-application-pgpkeys\">");
		printf("%s", getarg("PUBKEY"));
		printf("</A></CENTER>");

		printf("<HR WIDTH=\"100%%\">\n");

		printf("<CENTER><A HREF=\"");
		output_scriptptrget();
		printf("&form=keyimport&pos=%ld", (long)msg_pos);
		printf("&privkeyimport=1");
		output_mimegpgfilename();
		showmimeid(id, "&keymimeid=");
		printf("\" STYLE=\"text-decoration: none\" CLASS=\"message-application-pgpkeys\">");
		printf("%s", getarg("PRIVKEY"));
		printf("</A></CENTER>");
	}

	printf("</TD></TR></TABLE>\n");
	printf("</TD></TR></TABLE>\n<BR>\n");
}
#endif

static void showattname(const char *fmt, const char *name,
	const char *content_type)
{
char	*s, *t;

	if (!name || !*name)	name=content_type;
	if (!name)	name="";

	s=rfc2047_decode_simple(name);
	if (!s)	return;

	t=malloc(strlen(s)+strlen(fmt)+100);
	if (!t)
	{
		free(s);
		return;
	}

	sprintf(t, fmt, s ? s:name);
	if (s)	free(s);
	output_attrencoded(t);
}

static void showmultipartdecoded_start(int status, const char **styleptr)
{
	const char *style= status ? "message-gpg-bad":"message-gpg-good";

	printf("<TABLE BORDER=0 CELLPADDING=2 CLASS=\"%s\"><TR><TD>"
	       "<TABLE BORDER=0 CLASS=\"message-gpg\"><TR><TD>", style);
	*styleptr=status ? "message-gpg-bad-text":"message-gpg-good-text";

}

static void showmultipartdecoded_end()
{
	printf("</TD></TR></TABLE></TD></TR></TABLE>\n");
}

static void showmultipart(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
const char	*content_type, *dummy;
struct	rfc2045 *q;
struct	rfc2045id	nextpart, nextnextpart;
struct	rfc2045id	*p;
int gpg_status;

	for (p=id; p->next; p=p->next)
		;
	p->next=&nextpart;
	nextpart.idnum=0;
	nextpart.next=0;

	rfc2045_mimeinfo(rfc, &content_type, &dummy, &dummy);


	if (*cgi(MIMEGPGFILENAME) && !is_preview_mode() &&
	    gpgmime_is_decoded(rfc, &gpg_status))
	{
		const char *style;
		showmultipartdecoded_start(gpg_status, &style);
		for (q=rfc->firstpart; q; q=q->next, ++nextpart.idnum)
		{
			if (q->isdummy)	continue;

			
			if (nextpart.idnum == 1)
			{
				printf("<BLOCKQUOTE CLASS=\"%s\">",
				       style);
			}

			(*get_handler(q))(fp, q, id);
			if (nextpart.idnum == 1)
			{
				printf("</BLOCKQUOTE>");
			}
			else
				if (q->next);
					// by lfan
					//printf("<HR WIDTH=\"100%%\">\n");
		}
		showmultipartdecoded_end();
	}
	else if (strcmp(content_type, "multipart/alternative") == 0)
	{
		struct	rfc2045 *q, *r=0, *s;
	int	idnum=0;
	int	dummy;

		for (q=rfc->firstpart; q; q=q->next, ++idnum)
		{
			int found=0;
			if (q->isdummy)	continue;

			/*
			** We pick this multipart/related section if:
			**
			** 1) This is the first section, or
			** 2) We know how to display this section, or
			** 3) It's a multipart/signed section and we know
			**    how to display the signed content.
			** 4) It's a decoded section, and we know how to
			**    display the decoded section.
			*/

			if (!r)
				found=1;
			else if ((s=gpgmime_is_multipart_signed(q)) != 0)
			{
				if (get_known_handler(s))
					found=1;
			}
			else if ( *cgi(MIMEGPGFILENAME)
				  && gpgmime_is_decoded(q, &dummy))
			{
				if ((s=gpgmime_decoded_content(q)) != 0
				    && get_known_handler(s))
					found=1;
			}
			else if (get_known_handler(q))
			{
				found=1;
			}

			if (found)
			{
				r=q;
				nextpart.idnum=idnum;
			}
		}

		if (r)
			(*get_handler(r))(fp, r, id);
	}
	else if (strcmp(content_type, "multipart/related") == 0)
	{
	char *sid=rfc2045_related_start(rfc);

		/*
		** We can't just walts in, search for the Content-ID:,
		** and skeddaddle, that's because we need to keep track of
		** our MIME section.  So we pretend that we're multipart/mixed,
		** see below, and abort at the first opportunity.
		*/

		for (q=rfc->firstpart; q; q=q->next, ++nextpart.idnum)
		{
		const char *cid;

			if (q->isdummy)	continue;

			cid=rfc2045_content_id(q);

			if (sid && *sid && strcmp(sid, cid))
			{
				struct rfc2045 *qq;

				qq=gpgmime_is_multipart_signed(q);

				if (!qq) continue;

				/* Don't give up just yet */

				cid=rfc2045_content_id(qq);

				if (sid && *sid && strcmp(sid, cid))
				{
					/* Not yet, check for MIME/GPG stuff */



					/* Ok, we can give up now */
					continue;
				}
				nextnextpart.idnum=1;
				nextnextpart.next=0;
				nextpart.next= &nextnextpart;
			}
			(*get_handler(q))(fp, q, id);

			break;
			/* In all cases, we stop after dumping something */
		}
		if (sid)	free(sid);
	}
	else
	{
		for (q=rfc->firstpart; q; q=q->next, ++nextpart.idnum)
		{
			if (q->isdummy)	continue;
			(*get_handler(q))(fp, q, id);
			//if (q->next)
			//	printf("<HR WIDTH=\"100%%\">\n");
		}
	}
	p->next=0;
}

static void text_to_stdout(const char *p, size_t n)
{
	while (n)
	{
		--n;
		putchar(*p++);
	}
}

static struct buf showtextplain_buf;

static const char *skip_text_url(const char *r, const char *end)
{
const char *q=r;

	for (; r < end && (isalnum(*r) || *r == ':' || *r == '/'
		|| *r == '.' || *r == '~' || *r == '%'
		|| *r == '+' || *r == '?' || *r == '&' || *r == '#'
		|| *r == '=' || *r == '@' || *r == ';'
		|| *r == '-' || *r == '_' || *r == ','); r++)
	{
		if (*r == '&' && (end-r < 5 || strncmp(r, "&amp;", 5)))
			break;
	}
	if (r > q && (r[-1] == ',' || r[-1] == '.'))	--r;
	return (r);
}

static char *decode_cgiurlencode(const char *s)
{
char *q=malloc(strlen(s)+1), *r;
const char *t;

	if (!q)	enomem();

	for (r=q, t=s; *t; )
	{
		if (strncmp(t, "&amp;", 5) == 0)
		{
			*r++ = '&';
			t += 5;
			continue;
		}
		if (strncmp(t, "&lt;", 4) == 0)
		{
			*r++ = '<';
			t += 4;
			continue;
		}
		if (strncmp(t, "&gt;", 4) == 0)
		{
			*r++ = '>';
			t += 4;
			continue;
		}
		if (strncmp(t, "&quot;", 6) == 0)
		{
			*r++ = '"';
			t += 6;
			continue;
		}
		*r++ = *t++;
	}
	*r=0;

	r=cgiurlencode(q);
	free(q);
	return (r);
}

const char *redirect_hash(const char *timestamp)
{
	struct stat stat_buf;

	char buffer[NUMBUFSIZE*2+10];

	if (strlen(timestamp) >= NUMBUFSIZE
	    || stat(SENDITSH, &stat_buf) < 0)
		return "";

	strcat(strcpy(buffer, timestamp), " ");
	libmail_str_ino_t(stat_buf.st_ino, buffer+strlen(buffer));

	return md5_hash_courier(buffer);
}

static char *get_textlink(const char *s)
{
char	*t;
struct buf b;

	buf_init(&b);

	if (strncmp(s, "mailto:", 7) == 0)
	{
	int	i;

		buf_cat(&b, "<A HREF=\"");
		buf_cat(&b, scriptptrget());
		buf_cat(&b, "&form=newmsg&to=");

		for (i=7; s[i]; i++)
		{
		char	c[2];

			c[1]=0;
			if ((c[0]=s[i]) == '?')
				c[0]='&';
			buf_cat(&b, c);
		}
		buf_cat(&b, "\">"
			"<FONT CLASS=\"message-text-plain-mailto-link\">");
		buf_cat(&b, s);
		buf_cat(&b, "</FONT></A>");
	}
	else if (strncmp(s, "http:", 5) == 0 || strncmp(s, "https:", 6) == 0)
	{
		char buffer[NUMBUFSIZE];
		time_t now;
		char *hash;

		time(&now);
		libmail_str_time_t(now, buffer);

		hash=cgiurlencode(redirect_hash(buffer));

		t=decode_cgiurlencode(s);
		buf_cat(&b, "<A HREF=\"");
		buf_cat(&b, getenv("SCRIPT_NAME"));
		buf_cat(&b, "?redirect=");
		buf_cat(&b, t);
		buf_cat(&b, "&timestamp=");
		buf_cat(&b, buffer);
		buf_cat(&b, "&md5=");
		if (hash)
		{
			buf_cat(&b, hash);
			free(hash);
		}
		buf_cat(&b, "\" TARGET=\"_blank\">"
			"<FONT CLASS=\"message-text-plain-http-link\">");
		buf_cat(&b, s);
		buf_cat(&b, "</FONT></A>");
		free(t);
	}
	t=strdup(b.ptr ? b.ptr:"");
	if (!t)	enomem();
	buf_free(&b);
	return (t);
}

static void showtextplain_oneline(const char *p, size_t n)
{
const char *q, *r;
char	*s, *t;

	for (q=r=p; q < p+n; q++)
	{
		if ( p+n-q > 7 &&
				(strncmp(q, "http://", 7) == 0
					||
				strncmp(q, "https:/", 7) == 0
					||
				strncmp(q, "mailto:", 7) == 0)
				)
		{
			fwrite(r, 1, q-r, stdout);
			r=skip_text_url(q, p+n);

			if ((s=malloc(r+1-q)) == NULL)	enomem();
			memcpy(s, q, r-q);
			s[r-q]=0;
			printf("%s", (t=get_textlink(s)));
			free(t);
			free(s);
			q=r;
		}
	}
	fwrite(r, 1, q-r, stdout);
}

static void showtextplainfunc(const char *txt, size_t l)
{
const	char *p;
size_t	n;

	if (txt)
	{
		buf_catn(&showtextplain_buf, txt, l);
		while ((p=strchr(showtextplain_buf.ptr, '\n')) != 0)
		{
			n= p+1 - showtextplain_buf.ptr;
			showtextplain_oneline(showtextplain_buf.ptr, n);
			puts("<br>\n");
			buf_trimleft(&showtextplain_buf, n);
		}
	}
	else if (showtextplain_buf.cnt)
		showtextplain_oneline(showtextplain_buf.ptr,
					showtextplain_buf.cnt);
}

static int filter_stub(const char *ptr, size_t cnt, void *voidptr)
{
	struct rfc2646parser *flowparser=(struct rfc2646parser *)voidptr;

	if (flowparser)
		return ( rfc2646_parse(flowparser, ptr, cnt));

	filter(ptr, cnt);
	return (0);
}

static int filter_flowed(const char *ptr, int cnt, void *voidptr)
{
	showtextplainfunc(ptr, cnt);
	return (0);
}

static void showtextplain(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
	int rc;

	const char *mime_charset, *dummy;

	int isflowed;

	struct rfc2646parser *flowparser=0;
	struct rfc2646tohtml *flowtohtml=0;

	id=id;
/*
	if (is_preview_mode())
	{
		const char *cb=rfc2045_getattr(rfc->content_type_attr,
					       "format");
		if (cb && strcasecmp(cb, "xdraft") == 0)
		{
			preview_start();
			rfc2045_decodemimesection(fileno(fp), rfc,
						  &preview_callback, NULL);
			preview_end();
			return;
		}
	}
*/
	rfc2045_mimeinfo(rfc, &dummy, &dummy, &mime_charset);

	if (strcasecmp(mime_charset, sqwebmail_content_charset) &&
	    strcasecmp(mime_charset, "us-ascii"))
	{
		printf(getarg("CHSET"), mime_charset,
		       sqwebmail_content_charset);
	}

	isflowed=rfc2045_isflowed(rfc);

	if (pref_noflowedtext)
		isflowed=0;

	buf_init(&showtextplain_buf);

	if (isflowed)
	{
		flowtohtml=rfc2646tohtml_alloc(filter_flowed, NULL);

		if (!flowtohtml)
			enomem();

		flowparser=RFC2646TOHTML_PARSEALLOC(flowtohtml);

		if (!flowparser)
		{
			rfc2646tohtml_free(flowtohtml);
			enomem();
		}

		printf("<TT CLASS=\"message-text-plain\">");
	}
	else
	{
		// by lfan, remove PRE
		printf("<TT CLASS=\"message-text-plain\">");
		filter_start(FILTER_FOR_DISPLAY, &showtextplainfunc);
	}

#if HAVE_SQWEBMAIL_UNICODE
	rc=rfc2045_decodetextmimesection(fileno(fp), rfc,
					 sqwebmail_content_charset,
					 &filter_stub,
					 flowparser);
#else
	rc=rfc2045_decodemimesection(fileno(fp), rfc,
				     &filter_stub,
				     flowparser);
#endif
	fseek(fp, 0L, SEEK_END);
	fseek(fp, 0L, SEEK_SET);	/* Resync stdio with uio */

	if (isflowed)
	{
		rfc2646_free(flowparser);
		rfc2646tohtml_free(flowtohtml);
	}
	else
	{
		filter_end();
	}
	showtextplainfunc(0, 0);
	buf_free(&showtextplain_buf);

	// by lfan
	//if (isflowed)
		printf("</TT><BR>\n");
	//else
	//	printf("</PRE></TT><BR>\n");
}

static void showdsn(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
off_t	start_pos, end_pos, start_body;
off_t	dummy;

	id=id;
	rfc2045_mimepos(rfc, &start_pos, &end_pos, &start_body, &dummy, &dummy);
	if (fseek(fp, start_body, SEEK_SET) < 0)
	{
		error("Seek error.");
		return;
	}
	printf("<P><TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0>\n");
	while (start_body < end_pos)
	{
	int	c=getc(fp);
	char	*header, *value;

		if (c == EOF)	break;
		if (c == '\n')
		{
			printf("<TR><TD COLSPAN=2><HR></TD></TR>\n");
			++start_body;
			continue;
		}
		ungetc(c, fp);

		if ((header=maildir_readheader_mimepart(fp, &value, 1,
			&start_body, &end_pos)) == 0)
			break;

		print_header_uc(header);
		printf("<TD><TT CLASS=\"message-rfc822-header-contents\">");
		/* showmsgrfc822_addressheader(value); */
		printf("%s", value);
		printf("</TT></TD></TR>\n");
	}
	printf("</TABLE>\n");
}

static int htmlfilter_stub(const char *ptr, size_t cnt, void *voidptr)
{
	htmlfilter(ptr, cnt);
	return (0);
}

/* Recursive search for a Content-ID: header that we want */

static struct rfc2045 *find_cid(struct rfc2045 *p, const char *cidurl)
{
const char *cid=rfc2045_content_id(p);

	if (cid && strcmp(cid, cidurl) == 0)
		return (p);

	for (p=p->firstpart; p; p=p->next)
	{
	struct rfc2045 *q;

		if (p->isdummy)	continue;

		q=find_cid(p, cidurl);
		if (q)	return (q);
	}
	return (0);
}

/*
** Given an rfc2045 ptr, return the mime reference that will resolve to
** this MIME part.
*/

static char *rfc2mimeid(struct rfc2045 *p)
{
char	buf[MAXLONGSIZE+1];
char	*q=0;
unsigned n=p->pindex+1;	/* mime counts start at one */
char	*r;

	if (p->parent)
	{
		q=rfc2mimeid(p->parent);
		if (p->parent->firstpart->isdummy)
			--n;	/* ... except let's ignore the dummy part */
	}
	else	n=1;

	sprintf(buf, "%u", n);
	r=malloc( (q ? strlen(q)+1:0)+strlen(buf)+1);
	if (!r)	enomem();
	*r=0;
	if (q)
	{
		strcat(strcat(r, q), ".");
		free(q);
	}
	strcat(r, buf);
	return (r);
}

/*
** Convert cid: url to a http:// reference that will access the indicated
** MIME section.
*/

static void add_decoded_link(struct rfc2045 *, const char *, int);

static char *convertcid(const char *cidurl, void *voidp)
{
	struct	rfc2045 *rfc= (struct rfc2045 *)voidp;
	struct	rfc2045 *savep;

	char	*p;
	char	*mimeid;
	char	*q;
	const char *pos;
	char *mimegpgfilename=cgiurlencode(cgi(MIMEGPGFILENAME));
	int dummy;


	if (!mimegpgfilename)
		enomem();

	if (rfc->parent)	rfc=rfc->parent;
	if (rfc->parent)
	{
		if (gpgmime_is_multipart_signed(rfc) ||
		    (*mimegpgfilename
		     && gpgmime_is_decoded(rfc, &dummy)))
			rfc=rfc->parent;
	}

	savep=rfc;
	rfc=find_cid(rfc, cidurl);

	if (!rfc)
		/* Sometimes broken MS software needs to go one step higher */
	{
		while ((savep=savep->parent) != NULL)
		{
			rfc=find_cid(savep, cidurl);
			if (rfc)
				break;
		}
	}

	if (!rfc)	/* Not found, punt */
	{
	char	*p=malloc(1);

		if (!p)	enomem();
		*p=0;
		free(mimegpgfilename);
		return (p);
	}

	p=scriptptrget();
	mimeid=rfc2mimeid(rfc);
	pos=cgi("pos");

	q=malloc(strlen(p)+strlen(mimeid)+strlen(pos) +
		 strlen(mimegpgfilename)+
		sizeof("&pos=&form=fetch&mimeid=&mimegpgfilename="));
	if (!q)	enomem();
	strcpy(q, p);
	strcat(q, "&form=fetch&pos=");
	strcat(q, pos);
	if (*mimegpgfilename)
	{
		strcat(strcat(q, "&mimegpgfilename="), mimegpgfilename);

		if (rfc->parent && gpgmime_is_decoded(rfc->parent, &dummy))
			add_decoded_link(rfc->parent, mimeid, dummy);
	}
	strcat(q, "&mimeid=");
	strcat(q, mimeid);
	free(p);
	free(mimeid);
	free(mimegpgfilename);
	return (q);
}

/*
** When we output a multipart/related link to some content that has been
** signed/encoded, we save the decoding status, for later.
**
** Note -- we collapse multiple links to the same content.
*/

static struct decoded_list {
	struct decoded_list *next;
	struct rfc2045 *ptr;
	char *mimeid;
	int status;
} *decoded_first=0, *decoded_last=0;

static void add_decoded_link(struct rfc2045 *ptr, const char *mimeid,
			     int status)
{
	struct decoded_list *p;

	for (p=decoded_first; p; p=p->next)
	{

		if (strcmp(p->mimeid, mimeid) == 0)
			return;	/* Dupe */
	}

	p=(struct decoded_list *)malloc(sizeof(*p));

	if (!p)
		enomem();

	p->mimeid=strdup(mimeid);

	if (!p->mimeid)
	{
		free(p);
		enomem();
	}
	p->next=0;

	if (decoded_last)
		decoded_last->next=p;
	else
		decoded_first=p;

	decoded_last=p;

	p->ptr=ptr;
	p->status=status;
}

static void showtexthtml(FILE *fp, struct rfc2045 *rfc, struct rfc2045id *id)
{
off_t	start_pos, end_pos, start_body;
char	buf[512];
int	cnt;
const char *script_name=nonloginscriptptr();
char	*washpfix;
char	*washpfixmailto;
char	*scriptnameget=scriptptrget();
static const char formbuf[]="&form=newmsg&to=";
off_t	dummy;
char	*content_base;
const char *mime_charset, *dummy_s;

	char nowbuffer[NUMBUFSIZE];
	time_t now;
	char *hash;

	time(&now);
	libmail_str_time_t(now, nowbuffer);

	hash=cgiurlencode(redirect_hash(nowbuffer));

	id=id;
	if (!script_name)	enomem();
	rfc2045_mimepos(rfc, &start_pos, &end_pos, &start_body, &dummy, &dummy);
	if (fseek(fp, start_body, SEEK_SET) < 0)
	{
		error("Seek error.");
		return;
	}

	washpfix=malloc(strlen(script_name) + strlen(hash) + strlen(nowbuffer)
			+ 100);
	if (!washpfix)	enomem();

	strcat(strcat(strcat(strcat(strcat(strcpy(washpfix, script_name),
					   "?timestamp="),
				    nowbuffer),
			     "&md5="),
		      (hash ? hash:"")),
	       "&redirect=");

	if (hash)
		free(hash);

	htmlfilter_washlink(washpfix);
	htmlfilter_convertcid(&convertcid, rfc);

	content_base=rfc2045_content_base(rfc);

	htmlfilter_contentbase(content_base);

	washpfixmailto=malloc(strlen(scriptnameget)+sizeof(formbuf));
	if (!washpfixmailto)	enomem();
	strcat(strcpy(washpfixmailto, scriptnameget), formbuf);
	htmlfilter_washlinkmailto(washpfixmailto);
	htmlfilter_init(&text_to_stdout);

	rfc2045_mimeinfo(rfc, &dummy_s, &dummy_s, &mime_charset);

	if (strcasecmp(mime_charset, sqwebmail_content_charset) &&
	    strcasecmp(mime_charset, "us-ascii"))
	{
		printf(getarg("CHSET"), mime_charset,
		       sqwebmail_content_charset);
	}

	printf("%s", getarg("HTML"));

	printf("<TABLE BORDER=0 CELLPADDING=0 CELLSPACING=0 WIDTH=\"100%%\" CLASS=\"message-text-plain\"><TR><TD>\n");
	rfc2045_cdecode_start(rfc, &htmlfilter_stub, 0);
	while (start_body < end_pos)
	{
		cnt=sizeof(buf);
		if (end_pos - start_body < cnt)
			cnt=end_pos - start_body;
		cnt=fread(buf, 1, cnt, fp);
		if (cnt <= 0)	break;
		// by lfan, decoding bug, cannot identify boundary
		{
			char 	*p;
			int	len = 38;
			p = strstr(buf, "------");
			if( p ) {
				if( *(p+37) && strncmp(p+37, "--", 2) == 0 )
					len += 2;
				bzero( p, len );
			}
		}
		rfc2045_cdecode(rfc, buf, cnt);
		start_body += cnt;
	}
	rfc2045_cdecode_end(rfc);
	printf("</TD></TR>");

	free(washpfix);
	free(washpfixmailto);
	free(content_base);

	while (decoded_first)
	{
		struct decoded_list *p=decoded_first;
		const char *style;

		struct rfc2045 *q;

		printf("<TR><TD>");

		showmultipartdecoded_start(p->status, &style);

		for (q=p->ptr->firstpart; q; q=q->next)
		{
			if (q->isdummy)
				continue;

			printf("<DIV CLASS=\"%s\">", style);
			(*get_handler(q))(fp, q, NULL);
			printf("</DIV>\n");
			break;
		}
		showmultipartdecoded_end();
		decoded_first=p->next;
		free(p->mimeid);
		free(p);
		printf("</TD></TR>\n");
	}
	printf("</TABLE>\n");

}

static char *get_parent_folder(const char *p)
{
const char *q=strrchr(p, '.');
char	*s;

	if (!q)
	{
		s=strdup("");
		if (!s)	enomem();
		return (s);
	}

	s=malloc(q-p+1);
	if (!s)	enomem();
	memcpy(s, p, q-p);
	s[q-p]=0;
	return (s);
}

void folder_list()
{
char	**folders;
size_t	i;
unsigned nnew, nother;
const char	*unread_label;
const char	*err_invalid;
const char	*err_exists;
const char	*err_cantdelete;
const char	*msg_hasbeensent;
const char	*name_inbox;
const char	*name_drafts;
const char	*name_sent;
const char	*name_trash;
const char	*name_spam;
// by lfan
//const char	*folder_img;
//const char	*folders_img;

const char *folderdir;

// by lfan
int     	refresh_left = 0;


	unread_label=getarg("UNREAD");
	err_invalid=getarg("INVALID");
	err_exists=getarg("EXISTS");
	err_cantdelete=getarg("DELETE");
	msg_hasbeensent=getarg("WASSENT");
	name_inbox=getarg("INBOX");
	name_drafts=getarg("DRAFTS");
	name_sent=getarg("SENT");
	name_trash=getarg("TRASH");
	name_spam=getarg("SPAM");
	// by lfan
	//folder_img=getarg("FOLDERICON");
	//folders_img=getarg("FOLDERSICON");

	folder_err_msg=0;

	if (strcmp(cgi("foldermsg"), "sent") == 0)
		folder_err_msg=msg_hasbeensent;

	if (*cgi("do.create"))
	{
	const char	*newfoldername=cgi("foldername");
	const char	*newdirname=cgi("dirname");
	const char	*folderdir=cgi("folderdir");

		// by lfan
		refresh_left = 1;

		/*
		** New folder names cannot contain :s, and must be considered
		** as valid by maildir_folderpath.
		*/

		if (!*newfoldername ||
			strchr(newfoldername, '.') ||
			strchr(newdirname, '.'))
			folder_err_msg=err_invalid;
		else
		{
			char	*p;
			char	*futf7;
			char	*dutf7;

			futf7=folder_toutf7(newfoldername);
			dutf7=folder_toutf7(newdirname);;

			p=malloc(strlen(folderdir)+strlen(futf7)
				 +strlen(dutf7)+2);

			if (!p)	enomem();
			strcpy(p, folderdir);
			if (*dutf7)
			{
				if (*p)	strcat(p, ".");
				strcat(p, dutf7);
			}
			if (*p)	strcat(p, ".");
			strcat(p, futf7);

			free(futf7);
			free(dutf7);

			if (strchr(p, ':'))
			{
				free(p);
				folder_err_msg=err_invalid;
			}
			else
			{
			char	*q=maildir_folderdir(0, p);

				if (!q)
					folder_err_msg=err_invalid;
				else
				{
					free(q);
					if (maildir_create(p))
						folder_err_msg=err_exists;
				}
				free(p);
			}
		}
	}

	if (*cgi("do.delete"))
	{
	const char *p=cgi("DELETE");
	char	*pp=strdup(p);
		// by lfan
		refresh_left = 1;

		if (pp && *pp && strchr(pp, ':') == 0)
		{
			if (mailfilter_folderused(pp))
				folder_err_msg=err_cantdelete;
			// by lfan
			// else if (maildir_delete(pp, *cgi("deletecontent")))
			else if (maildir_delete(pp, 1))
				folder_err_msg=err_cantdelete;
			else
				maildir_quota_recalculate(".");

		}
		if (pp)
			free(pp);
	}

	if (*cgi("do.subunsub"))
	{
	const char *p=cgi("DELETE");
	char	*pp=strdup(p);
	char *d;

		if (pp && *pp == ':' &&
			(d=maildir_shareddir(".", pp+1)) != 0)
		{
		struct stat	stat_buf;

			if (stat(d, &stat_buf) == 0)
				maildir_shared_unsubscribe(".", pp+1);
			else
				maildir_shared_subscribe(".", pp+1);
			free(d);
		}

		if (pp)
			free(pp);
	}

	if (*cgi("do.rename"))
	{
	const char *p=cgi("DELETE");
	char	*pp=strdup(p);
		
		// by lfan
		refresh_left = 1;

		if (pp && strchr(pp, ':') == 0)
		{
			// by lfan
			//const char *qutf7=cgi("renametofolder");
			//const char *r=cgi("renametoname");
			const char *qutf7=INBOX;
			const char *r=cgi("foldername");
			char	*s;
			char	*t;

			char	*rutf7;

			rutf7=folder_toutf7(r);

			s=malloc(strlen(qutf7)+strlen(rutf7)+1);

			if (!s)	enomem();
			*s=0;
			if (strchr(qutf7, '.'))
				strcpy(s, qutf7);

			strcat(s, rutf7);

			if (strchr(s, ':') ||
			    strchr(rutf7, '.') ||
			    (t=maildir_folderdir(0, s)) == 0)
				folder_err_msg=err_invalid;
			else
			{
			struct	stat	stat_buf;

				if (stat(t, &stat_buf) == 0)
					folder_err_msg=err_exists;
				else
				{
					if (mailfilter_folderused(pp))
						folder_err_msg=err_cantdelete;
					else if (maildir_rename_wrapper(pp, s))
						folder_err_msg=err_cantdelete;
				}
				free(t);
			}
			free(rutf7);
			free(pp);
			free(s);
		}

		maildir_quota_recalculate(".");
	}

	sqwebmail_folder=0;
	folderdir=cgi("folderdir");
	
	// by lfan
       	//printf("<TABLE WIDTH=\"100%%\" BORDER=0 CELLPADDING=2 CELLSPACING=0 class=\"folderlist\">\n");
	maildir_readfolders(&folders);
	
	/* by lfan, disable dir hierarchy
	if (*folderdir)
	{
	char *parentfolder=get_parent_folder(folderdir);
	size_t	i;

		printf("<TR><TD ALIGN=LEFT COLSPAN=2 class=\"folderparentdir\">%s&lt;&lt;&lt;&nbsp;<A HREF=\"", folders_img);
		output_scriptptrget();
		printf("&form=folders&folder=INBOX\">");
		print_safe(name_inbox);
		printf("</A>");

		i=0;
		while (parentfolder[i])
		{
		char	*p=strchr(parentfolder+i, '.');
		int	c;

			if (!p)	p=parentfolder+strlen(parentfolder);
			c= *p;
			*p=0;

			printf(".<A HREF=\"");
			output_scriptptrget();
			printf("&form=folders&folder=INBOX&folderdir=");
			output_urlencoded(parentfolder);
			printf("\">");
			list_folder_xlate(parentfolder+i,
					  name_inbox,
					  name_drafts,
					  name_sent,
					  name_trash);
			printf("</A>");
			if ( (*p=c) != 0)	++p;
			i=p-parentfolder;
		}
		printf("</TD></TR>\n");
		free(parentfolder);
	}
	*/
	
	for (i=0; folders[i]; i++)
	{
	// by lfan
	//const	char *p;
	const	char *shortname=folders[i];
	//size_t	j;
	const char *pfix=0;
	int isshared=0;
	int isunsubscribed=0;
	//const char	*img=folder_img;

		// by lfan, for size display
		int	foldersize = 0, foldersize1 = 0;
		time_t  maxtime = 0;
		char	*dirname;
		dirname = xlate_mdir(folders[i]);
		countcurnew(dirname, &maxtime, &foldersize, &foldersize1);
		foldersize = ( foldersize + 1023 ) / 1024;
		free(dirname);
	
		if (*shortname == ':')	/* Shared folders */
		{
		char	*dir;
		struct	stat	stat_buf;

			isshared=1;
			pfix="+++";

			dir=maildir_shareddir(".", shortname+1);
			if (!dir)	continue;
			if (stat(dir, &stat_buf))
				isunsubscribed=1;
			free(dir);
		}

		/* by lfan, disable dir hierarchy
		if (*folderdir)
		{
		unsigned	l=strlen(folderdir);

			if (memcmp(shortname, folderdir, l) ||
				shortname[l] != '.')	continue;
			shortname += l;
			++shortname;
			pfix=0;
		}

		if (!pfix)
		{
			pfix="&gt;&gt;&gt;";
		}

		if ((p=strchr(shortname, '.')) != 0)
		{
		char	*s=malloc(p+1-folders[i]), *t;
		unsigned tot_nnew, tot_nother;

			if (!s)	enomem();
			memcpy(s, folders[i], p-folders[i]);
			s[p-folders[i]]=0;

			img=folders_img;
			printf("<TR class=\"foldersubdir\"><TD ALIGN=LEFT>%s%s&nbsp;<A HREF=\"", img, pfix);
			output_scriptptrget();
			printf("&form=folders&folder=INBOX&folderdir=");
			output_urlencoded(s);
			printf("\">");
			free(s);
			t=malloc(p-shortname+1);
			if (!t)	enomem();
			memcpy(t, shortname, p-shortname);
			t[p-shortname]=0;
			list_folder_xlate(isshared ? t+1:t,
					  name_inbox,
					  name_drafts,
					  name_sent,
					  name_trash);
			free(t);
			printf("</A>");

			tot_nnew=0;
			tot_nother=0;

			j=i;
			while (folders[j] && memcmp(folders[j], folders[i],
				p-folders[i]+1) == 0)
			{
				maildir_count(folders[j], &nnew, &nother);
				++j;
				tot_nnew += nnew;
				tot_nother += nother;
			}
			i=j-1;
			if (tot_nnew)
			{
				printf(" <FONT class=\"subfolderlistunread\" color=\"#800000\" size=\"-1\">");
				printf(unread_label, tot_nnew);
				printf("</FONT>");
			}
			printf("</TD><TD ALIGN=RIGHT VALIGN=TOP><font color=\"#000000\" class=\"subfoldercnt\">%d</font>&nbsp;&nbsp;</TD></TR>\n\n",
				tot_nnew + tot_nother);
			continue;
		}
		*/
		nnew=0;
		nother=0;

		if (!isunsubscribed)
			maildir_count(folders[i], &nnew, &nother);

		// by lfan
		//printf("<TR %s><TD ALIGN=LEFT VALIGN=TOP>",
		//	isunsubscribed ? "class=\"folderunsubscribed\"":"");

		//printf("%s<INPUT BORDER=0 TYPE=\"radio\" NAME=\"DELETE\" VALUE=\"", img);
		//output_attrencoded(folders[i]);
		//printf("\">&nbsp;");
		
		printf("<TR BGCOLOR=\"#FFFFFC\" ALIGN=CENTER><TD ALIGN=RIGHT>");
                if(strcmp(folders[i], INBOX) && strcmp(folders[i], DRAFTS) &&
                   strcmp(folders[i], TRASH) && strcmp(folders[i], SENT) && 
		   strcmp(folders[i], SPAM) ) {
			printf("<INPUT BORDER=0 TYPE=\"radio\" NAME=\"DELETE\" VALUE=\"");
			output_attrencoded(folders[i]);
			printf("\">&nbsp;");
		}
		printf("</TD><TD>");

		if (!isunsubscribed)
		{
			printf("<A CLASS=\"folderlink\" HREF=\"");
			output_scriptptrget();
			printf("&form=folder&folder=");
			output_urlencoded(folders[i]);
			printf("\">");
		}
		if (strcmp(folders[i], INBOX) == 0)
			shortname=name_inbox;
		else if (strcmp(folders[i], DRAFTS) == 0)
			shortname=name_drafts;
		else if (strcmp(folders[i], TRASH) == 0)
			shortname=name_trash;
		else if (strcmp(folders[i], SENT) == 0)
			shortname=name_sent;
		else if (strcmp(folders[i], SPAM) == 0)
			shortname=name_spam;
		list_folder(shortname);

		if (!isunsubscribed)
			printf("</A>");

		printf("<TD>%d</TD><TD>%d</TD><TD>%d</TD>", nnew+nother, nnew, foldersize );
		printf("</TR>\n");
		/* by lfan
		if (nnew)
		{
			printf(" <FONT CLASS=\"folderlistunread\" COLOR=\"#800000\" SIZE=\"-1\">");
			printf(unread_label, nnew);
			printf("</FONT>");
		}
		printf("</TD><TD ALIGN=RIGHT VALIGN=TOP>");

		if (!isunsubscribed)
		{
			printf("<font class=\"foldercnt\" color=\"#000000\">%d</font>&nbsp;&nbsp;",
			       nnew + nother);
		}
		else
		printf("&nbsp;\n");
		printf("</TD></TR>\n\n");
		*/
	}
	maildir_freefolders(&folders);

	// by lfan
	//printf("</TABLE>\n");
        if ( refresh_left == 1 )
		printf("%s\n", getarg("REFRESH_LEFT"));
}

void folder_list2()
{
	if (folder_err_msg)
	{
		printf("<P>%s<BR><BR>\n", folder_err_msg);
	}
}

// by lfan
void folder_list3()
{
	char    **folders;
	size_t  i;      
        maildir_readfolders(&folders);
                
        for (i=0; folders[i]; i++) {
		const   char *p;
		const   char *shortname=folders[i];
		const char      *img;

		if (*shortname == ':')  /* Shared folders */
	        	continue;

                if ((p=strchr(shortname, '.')) != 0)
	                continue;

                if (strcmp(folders[i], INBOX) == 0) {
	                shortname=getarg("INBOX");
	                img = getarg("INBOXICON");
	        }
                else if (strcmp(folders[i], DRAFTS) == 0) {
	                shortname=getarg("DRAFTS");
	                img = getarg("DRAFTSICON");
	        }
                else if (strcmp(folders[i], TRASH) == 0) {
	                shortname=getarg("TRASH");
	                img = getarg("TRASHICON");
	        }
                else if (strcmp(folders[i], SENT) == 0) {
	                shortname=getarg("SENT");
	                img = getarg("SENTICON");
	        }
                else if (strcmp(folders[i], SPAM) == 0) {
	                shortname=getarg("SPAM");
	                img = getarg("SPAMICON");
	        }
                else
	                img = getarg("FOLDERICON");
		
                printf("insDoc(aux1, gLnk(0, \"");
                list_folder(shortname);
                printf("\", \"");
                output_scriptptrget();
                printf("&form=folder&folder=");
                output_urlencoded(folders[i]);
                printf("\", \"%s\"));\n", img);		
	}
        maildir_freefolders(&folders);
}
// end lfan

void folder_rename_list()
{
char	**folders;
int	i;

	printf("<select name=\"renametofolder\">\n");
	printf("<option value=\"%s\">", INBOX);
	printf("( ... )");
	printf("</option>\n");

	maildir_readfolders(&folders);
	for (i=0; folders[i]; i++)
	{
	const char *p=folders[i];
	char	*q;
	size_t	ql;

		if (*p == ':')	continue;	/* Omit shared hierarchy */

		p=strrchr(folders[i], '.');
		if (!p)	continue;
		q=malloc(p-folders[i]+1);
		if (!q)	enomem();
		memcpy(q, folders[i], p-folders[i]);
		q[p-folders[i]]=0;
		printf("<OPTION VALUE=\"");
		output_attrencoded(q);
		printf(".\" %s>",
			strcmp(q, cgi("folderdir")) == 0 ? "SELECTED":"");
		list_folder(q);
		printf(".</OPTION>\n");
		ql=strlen(q);
		while (folders[++i])
		{
			if (memcmp(folders[i], q, ql) ||
				folders[i][ql] != '.' ||
				strchr(folders[i]+ql+1, '.'))	break;
		}
		--i;
		free(q);
	}
	maildir_freefolders(&folders);
	printf("</select>\n");
}

static int download_func(const char *, size_t, void *);

void disposition_attachment(FILE *fp, const char *p)
{
	fprintf(fp, "Content-Disposition: attachment; filename=\"");
	while (*p)
	{
		if (*p == '"' || *p == '\\')
			putc('\\', fp);
		if (!ISCTRL(*p))
			putc(*p, fp);
		p++;
	}
	fprintf(fp, "\"\n");
}

void folder_download(const char *folder, size_t pos, const char *mimeid)
{
char	*filename;
struct	rfc2045 *rfc, *part;
char	buf[BUFSIZ];
int	n,cnt;
const char	*content_type, *dummy, *charset;
off_t	start_pos, end_pos, start_body;
FILE	*fp;
char	*content_name;
off_t	ldummy;
int	fd;


	filename=get_msgfilename(folder, &pos);

	fp=0;
	fd=maildir_semisafeopen(filename, O_RDONLY, 0);
	if (fd >= 0)
	{
		if ((fp=fdopen(fd, "r")) == 0)
			close(fd);
	}

	if (!fp)
	{
		free(filename);
		error("Message not found.");
		return;
	}
	free(filename);
	rfc=rfc2045_alloc();

	while ((n=fread(buf, 1, sizeof(buf), fp)) > 0)
		rfc2045_parse(rfc, buf, n);
	rfc2045_parse_partial(rfc);

	part=*mimeid ? rfc2045_find(rfc, mimeid):rfc;
	if (!part)	error("Message not found.");
	rfc2045_mimeinfo(part, &content_type, &dummy, &charset);
/*
	if (cgi_useragent("MSIE"))
		cginocache_msie();
	else
		cginocache();
*/
	if (rfc2231_udecodeType(part, "name", NULL,
				&content_name) < 0)
		content_name=NULL;

	if (*cgi("download") == '1')
	{
		char *disposition_filename;
		const char *p;

		disposition_filename = rfc2047_decode_simple(content_name);	
		/*
		if (rfc2231_udecodeDisposition(part, "filename", NULL,
					       &disposition_filename) < 0)
		{
			enomem();
			return;
		}
		*/


		p=disposition_filename;

		if (!p || !*p) p=content_name;
		if (!p || !*p) p="message.dat";
		disposition_attachment(stdout, p);
		content_type="application/octet-stream";
		free(disposition_filename);
	}
			
	printf(
		content_name && *content_name ?
		"Content-Type: %s; charset=\"%s\"; name=\"%s\"\n\n":
		"Content-Type: %s; charset=\"%s\"\n\n",
			content_type, charset, content_name ? content_name:"");
	if (content_name)
		free(content_name);

	rfc2045_mimepos(part, &start_pos, &end_pos, &start_body,
		&ldummy, &ldummy);

	if (*mimeid == 0)	/* Download entire message */
	{
		if (fseek(fp, start_pos, SEEK_SET) < 0)
		{
			error("Seek error.");
			return;
		}

		while (start_pos < end_pos)
		{
			cnt=sizeof(buf);
			if (cnt > end_pos-start_pos)
				cnt=end_pos-start_pos;
			cnt=fread(buf, 1, cnt, fp);
			if (cnt <= 0)	break;
			start_pos += cnt;
			download_func(buf, cnt, NULL);
		}
	}
	else
	{
		if (fseek(fp, start_body, SEEK_SET) < 0)
		{
			error("Seek error.");
			return;
		}

		rfc2045_cdecode_start(part, &download_func, 0);

		while (start_body < end_pos)
		{
			cnt=sizeof(buf);
			if (cnt > end_pos-start_body)
				cnt=end_pos-start_body;
			cnt=fread(buf, 1, cnt, fp);
			if (cnt <= 0)	break;
			start_body += cnt;
			rfc2045_cdecode(part, buf, cnt);
		}
		rfc2045_cdecode_end(part);
	}
	fclose(fp);
}

static int download_func(const char *p, size_t cnt, void *voidptr)
{
	while (cnt--)
		if (putchar((int)(unsigned char)*p++) == EOF)
		{
			cleanup();
			fake_exit(0);
		}
	return (0);
}

void folder_showtransfer()
{
	const char	*deletelab, *purgelab, *movelab, *golab;

	deletelab=getarg("DELETE");
	purgelab=getarg("PURGE");
	movelab=getarg("ORMOVETO");
	golab=getarg("GO");
	folder_inbox=getarg("INBOX");
	folder_drafts=getarg("DRAFTS");
	folder_trash=getarg("TRASH");
	folder_sent=getarg("SENT");
	folder_spam=getarg("SPAM");

	printf("<INPUT TYPE=HIDDEN NAME=pos VALUE=%s> ", cgi("pos"));
	/* add by roy 2003.1.1 */
	if (
		strcmp(sqwebmail_folder, TRASH) == 0 || 
		strcmp(sqwebmail_folder, SPAM) == 0
	) printf("<INPUT TYPE=SUBMIT CLASS=\"mybtn\" NAME=cmddelall VALUE=\"È«²¿É¾³ý\"> ");
	printf("<INPUT TYPE=SUBMIT CLASS=\"mybtn\" NAME=cmddel VALUE=\"%s\"> ",
		strcmp(sqwebmail_folder, TRASH) == 0
		? purgelab:deletelab );

	printf("<INPUT TYPE=SUBMIT CLASS=\"mybtn\" NAME=cmdmove VALUE=\"%s\"> <SELECT NAME=moveto>", golab);
	show_transfer_dest(sqwebmail_folder);
	printf("</SELECT>\n");
}

void folder_showquota()
{
	const char	*quotamsg;
	struct maildirsize quotainfo;

	quotamsg=getarg("QUOTAUSAGE");

	if (maildir_openquotafile(&quotainfo, "."))
		return;

	// TODO

	/* by lfan
	if (quotainfo.quota.nmessages != 0 ||
	    quotainfo.quota.nbytes != 0)
		printf(quotamsg, maildir_readquota(&quotainfo));
	*/

	if (quotainfo.quota.nmessages != 0 ||
	    quotainfo.quota.nbytes != 0) {
		maildir_readquota(&quotainfo);
		printf(quotamsg, quotainfo.quota.nbytes/1048576, 
		       ((float)quotainfo.size.nbytes)/1048576, 
		       ((float)quotainfo.quota.nbytes - quotainfo.size.nbytes)/1048576);
		puts("<TABLE ALIGN=CENTER cellSpacing=0 cellPadding=0 border=0 WIDTH=90%><TR><TD WIDTH=3%>0%</TD>"
		     "<TD WIDTH=94%%><TABLE style=\"BORDER:#104a7b 1px solid;\" cellSpacing=0 cellPadding=0 border=0"
		     " bgcolor=#FFFFFF WIDTH=100%%><TR><TD WIDTH=100%%>" 
		     );
		printf("<DIV style=\"WIDTH: %d%%; HEIGHT: 16px; BACKGROUND-COLOR: #339933\">"
		       "</DIV></TD></TR></TABLE></TD><TD ALIGN=RIGHT>100%%</TD></TR></TABLE>",
		       quotainfo.size.nbytes / ( quotainfo.quota.nbytes / 100 ) );
			
	}
	// by lfan, when the mailbox didn't use
	else
		printf(getarg("QUOTANOUSE"));
	
	maildir_closequotafile(&quotainfo);
}

void
folder_cleanup()
{
	msg_purgelab=0;
	msg_folderlab=0;
	folder_drafts=0;
	folder_inbox=0;
	folder_sent=0;
	folder_trash=0;
	msg_forwardattlab=0;
	msg_forwardlab=0;
	msg_fullheaderlab=0;
	msg_golab=0;
	msg_movetolab=0;
	msg_nextlab=0;
	msg_prevlab=0;
	msg_deletelab=0;
	msg_posfile=0;
	msg_replyalllab=0;
	msg_replylistlab=0;
	msg_replylab=0;
	folder_err_msg=0;
	msg_msglab=0;
	msg_add=0;

	msg_type=0;
	initnextprevcnt=0;
	msg_hasprev=0;
	msg_hasnext=0;
	msg_pos=0;
	msg_count=0;
}
