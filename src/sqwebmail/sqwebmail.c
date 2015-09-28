/*
** Copyright 1998 - 2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: sqwebmail.c,v 1.11 2003/11/24 11:44:19 roy Exp $
*/
#include	"sqwebmail.h"
#include	"sqconfig.h"
#include	"auth.h"
#include	"folder.h"
#include	"pref.h"
#include	"maildir.h"
#include	"cgi/cgi.h"
#include	"pref.h"
#include	"mailinglist.h"
#include	"newmsg.h"
#include	"addressbook.h"
#include	"autoresponse.h"
#include	"http11/http11.h"
#include	"random128/random128.h"
#include	"maildir/maildirmisc.h"
#include	"rfc822/rfc822hdr.h"
#include	<stdio.h>
#include	<errno.h>
#include	<stdlib.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<string.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#if	HAVE_FCNTL_H
#include	<fcntl.h>
#endif
#if	HAVE_LOCALE_H
#if	HAVE_SETLOCALE
#include	<locale.h>
#endif
#endif
#include	<ctype.h>
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#define	MD5_INTERNAL
#include	"md5/md5.h"

#include	"maildir/maildircache.h"
#include	"mailfilter.h"
#include	"numlib/numlib.h"
#include	"gpglib/gpglib.h"
#include	"authlib/authstaticlist.h"
#include	"gpg.h"
#if	HAVE_CRYPT_H
#include	<crypt.h>
#endif
#if     NEED_CRYPT_PROTOTYPE
extern char *crypt(const char *, const char *);
#endif
#include	"htmllibdir.h"

// by lfan, recaculate quota
#include "vpopmail.h"
#include "vauth.h"
#include "maildir/maildirquota.h"

#if	HAVE_SYSLOG_H
#include	<syslog.h>
#else
#define	syslog(a,b)
#endif

#include	"logindomainlist.h"

extern void spell_show();
extern void spell_check_continue();
extern void print_safe(const char *);
extern void ldaplist();
extern int ldapsearch();
extern void doldapsearch();

extern void sent_gpgerrtxt();
extern void sent_gpgerrresume();
extern const char *redirect_hash(const char *);

const char *sqwebmail_mailboxid=0;
const char *sqwebmail_folder=0;
const char *sqwebmail_sessiontoken=0;

const char *sqwebmail_content_language=0;
const char *sqwebmail_content_locale;
const char *sqwebmail_content_ispelldict;
const char *sqwebmail_content_charset;

static int noimages=0;

time_t	login_time;

/* Need to cache the following environment variables */
static const char * const authvars[] = { "AUTHADDR", "AUTHFULLNAME", 0 };

#ifdef	GZIP
static int gzip_save_fd;

#if	HAVE_LIBFCGI
#error "FastCGI cannot be used with gzip compression, rerun configure --without-gzip"
#endif
#endif

static const char *sqwebmail_formname;

extern void attachments_head(const char *, const char *, const char *);
extern void attachments_opts(const char *);
extern void doattach(const char *, const char *);

static void timezonelist();

struct template_stack {
	struct template_stack *next;
	FILE *fp;
} ;

static struct template_stack *template_stack=NULL;

#if HAVE_LIBFCGI
#include <signal.h>
#include <setjmp.h>

static jmp_buf stackenv;

void fake_exit(int n)
{
	struct template_stack *s;

	maildir_cache_cancel();
	FCGI_SetExitStatus(n);

	while ((s=template_stack) != NULL)
	{
		template_stack=s->next;
		fclose(s->fp);
		free(s);
	}

	longjmp(stackenv,1);
}

#ifdef	BANNERPROG
#error "Bannerprog not supported with FCGI"
#endif

#else

void fake_exit(int n)
{
	maildir_cache_cancel();
	exit(n);
}

#endif

/* Stub to catch aborts from authlib */

void authexit(int n)
{
	fake_exit(n);
}

/* enomem() used to be just an out-of-memory handler.  Now, I use it as a
** generic failure type of a deal.
*/

void rfc2045_error(const char *p)
{
	error(p);
}

void print_attrencodedlen(const char *p, size_t len, int oknl, FILE *fp)
{
	for (; len; p++, --len)
	{
		switch (*p)	{
		case '<':
			fprintf(fp, "&lt;");
			continue;
		case '>':
			fprintf(fp, "&gt;");
			continue;
		case '&':
			fprintf(fp, "&amp;");
			continue;
		case '"':
			fprintf(fp, "&quot;");
			continue;
		case '\n':
			if (oknl)
			{
				if (oknl == 2)
				{
					fprintf(fp, "<BR>");
					continue;
				}
				putc('\n', fp);
				continue;
			}
		default:
			if (!ISCTRL(*p))
			{
				putc(*p, fp);
				continue;
			}
		}
		fprintf(fp, "&#%d;", (int)(unsigned char)*p);
	}
}

void output_attrencoded_fp(const char *p, FILE *fp)
{
	print_attrencodedlen(p, strlen(p), 0, fp);
}

void output_attrencoded(const char *p)
{
	output_attrencoded_fp(p, stdout);
}

void output_attrencoded_oknl_fp(const char *p, FILE *fp)
{
	print_attrencodedlen(p, strlen(p), 1, fp);
}

void output_attrencoded_oknl(const char *p)
{
	output_attrencoded_oknl_fp(p, stdout);
}

void output_attrencoded_nltobr(const char *p)
{
	print_attrencodedlen(p, strlen(p), 2, stdout);
}

void output_urlencoded(const char *p)
{
char	*q=cgiurlencode(p);

	printf("%s", q);
	free(q);
}

void output_loginscriptptr()
{
#if	USE_HTTPS_LOGIN
const	char *p=cgihttpsscriptptr();
#elif	USE_RELATIVE_URL
const	char *p=cgirelscriptptr();
#else
const	char *p=cgihttpscriptptr();
#endif

	printf("%s", p);
}

const char *nonloginscriptptr()
{
#if	USE_HTTPS
	return (cgihttpsscriptptr());
#elif	USE_RELATIVE_URL
	return (cgirelscriptptr());
#else
	return (cgihttpscriptptr());
#endif
}


void output_scriptptr()
{
const	char *p=nonloginscriptptr();

	printf("%s", p);
	if (sqwebmail_mailboxid)
	{
	char	*q=cgiurlencode(sqwebmail_mailboxid);
	char	buf[NUMBUFSIZE];

		printf("/wn/");
			//sqwebmail_sessiontoken ?  sqwebmail_sessiontoken:" ",
			//libmail_str_time_t(login_time, buf));
		free(q);
	}
}

void output_loginscriptptr_get()
{
	output_loginscriptptr();
	if (sqwebmail_mailboxid)
	{
	char	*q=cgiurlencode(sqwebmail_mailboxid);
	char	buf[NUMBUFSIZE];

		printf("/wn/");
			//sqwebmail_sessiontoken ?  sqwebmail_sessiontoken:" ",
			//libmail_str_time_t(login_time, buf));
		free(q);
	}
}

char *scriptptrget()
{
char	*q=0;
size_t	l=0;
int	i;
char	buf[NUMBUFSIZE];

#define	ADD(s) {const char *zz=(s); if (i) strcat(q, zz); l += strlen(zz);}
#define ADDE(ue) { char *yy=cgiurlencode(ue); ADD(yy); free(yy); }

	for (i=0; i<2; i++)
	{
		if (i && (q=malloc(l+1)) == 0)	enomem();
		if (i)	*q=0;
		ADD( nonloginscriptptr() );
		if (!sqwebmail_mailboxid)
		{
			ADD("?");
			continue;
		}

		ADD("/wn/");
		//ADDE(sqwebmail_mailboxid);
		//ADD("/");
		//ADD(sqwebmail_sessiontoken ? sqwebmail_sessiontoken:" ");
		//ADD("/");
		//ADD(libmail_str_time_t(login_time, buf));

		ADD( "?" );
		if (sqwebmail_folder)
		{
			ADD("folder=");
			ADDE(sqwebmail_folder);
		}
	}
#undef	ADD
#undef	ADDE
	return (q);
}

void output_scriptptrget()
{
char	*p=scriptptrget();

	printf("%s", p);
	free(p);
	return;
}

void output_scriptptrpostinfo()
{
#if 0
	if (sqwebmail_mailboxid)
	{
		printf("<INPUT TYPE=HIDDEN NAME=\"mailbox\" VALUE=\"");
		output_attrencoded(sqwebmail_mailboxid);
		printf("\">");
	}
	if (sqwebmail_sessiontoken)
		printf("<INPUT TYPE=HIDDEN NAME=\"session\" VALUE=\"%s\">",
			sqwebmail_sessiontoken);
#endif
	if (sqwebmail_folder)
	{
		printf("<INPUT TYPE=HIDDEN NAME=\"folder\" VALUE=\"");
		output_attrencoded(sqwebmail_folder);
		printf("\">");
	}

	if (*cgi("folderdir"))	/* In folders.html */
	{
		printf("<INPUT TYPE=HIDDEN NAME=\"folderdir\" VALUE=\"");
		output_attrencoded(cgi("folderdir"));
		printf("\">");
	}
}

void error(const char *errmsg)
{
	cginocache();
	printf("Content-Type: text/html; charset=\"us-ascii\"\n\n<H1>%s</H1>\n",
		errmsg);
	cleanup();
	fake_exit(1);
}

void error2(const char *file, int line)
{
	cginocache();
	printf("Content-Type: text/html; charset=\"us-ascii\"\n\n<H1>Internal error (module %s, line %d) - contact system administrator</H1>\n",
		file, line);
	cleanup();
	fake_exit(1);
}

FILE *open_langform(const char *lang, const char *formname,
		    int print_header)
{
char	*formpath;
FILE	*f;
char	*templatedir=getenv("SQWEBMAIL_TEMPLATEDIR");
	
	if (!templatedir || !*templatedir)	templatedir=HTMLLIBDIR;

	/* templatedir/lang/formname */

	if (!(formpath=malloc(strlen(templatedir)+3+
		strlen(lang)+strlen(formname))))
		error("Out of memory.");

	strcat(strcat(strcat(strcat(strcpy(formpath, templatedir), "/"),
		lang), "/"), formname);

	f=fopen(formpath, "r");
	free(formpath);
	if (f && print_header)
		printf("Content-Language: %s\n", lang);
	if (f)
		fcntl(fileno(f), F_SETFD, FD_CLOEXEC);
	return (f);
}

int ishttps()
{
	const char *p=getenv("HTTPS");

	return (p && strcasecmp(p, "on") == 0);
}

struct var_put_buf {
	char argbuf[3072];
	char *argp;
	size_t argn;
} ;

static void var_put_buf_func(int c, void *p)
{
	struct var_put_buf *pp=(struct var_put_buf *)p;

	if (pp->argn)
	{
		*pp->argp++=c;
		--pp->argn;
	}
}

static void pass_image_through(int c, void *p)
{
	putchar(c);
}

static void output_image( FILE *f,
			  void (*output_func)(int, void *), void *void_arg)
{
	int c;

	/*
	  Conditional image.  It's formatted as follows:

	  @@filename,width=x height=y@text@
            ^
            |
	    ----- we're at this point now.

	  If images are enabled, we replace that with an IMG tag we build from
	  filename,width=x, height=y.
	  If images are disabled, we replace all of this with text.

	*/

#define	MKIMG(c)	(*output_func)((c), void_arg)

	if (noimages)
	{
		while ((c=getc(f)) >= 0
		       && c != '@')
			;
		while ((c=getc(f)) >= 0
		       && c != '@')
			MKIMG(c);
	}
	else
	{
		char	*p;

		MKIMG('<');
		MKIMG('I');
		MKIMG('M');
		MKIMG('G');
		MKIMG(' ');
		MKIMG('S');
		MKIMG('R');
		MKIMG('C');
		MKIMG('=');
		MKIMG('"');
		for (p=IMGPATH; *p; p++)
			MKIMG(*p);
		MKIMG('/');
		while ((c=getc(f)) >= 0
		       && c != '@' && c != ',')
			MKIMG(c);
		MKIMG('"');
		MKIMG(' ');
		if (c == ',')
			c=getc(f);
		while (c >= 0 && c != '@')
		{
			MKIMG(c);
			c=getc(f);
		}
		while ((c=getc(f)) >= 0 && c != '@')
			;
		MKIMG('>');
	}
}

/* ---- time zone list ---- */

static int timezonefile( int (*callback_func)(const char *, const char *,
					      void *), void *callback_arg)
{
	FILE *f=NULL;
	char buffer[BUFSIZ];

	if (sqwebmail_content_language)
		f=open_langform(sqwebmail_content_language, "TIMEZONELIST", 0);

	if (!f)	f=open_langform(HTTP11_DEFAULTLANG, "TIMEZONELIST", 0);

	if (!f)
		return (0);

	while (fgets(buffer, sizeof(buffer), f) != NULL)
	{
		char *p=strchr(buffer, '\n');
		char *tz;
		int rc;

		if (p) *p=0;

		p=strchr(buffer, '#');
		if (p) *p=0;

		for (p=buffer; *p; p++)
			if (!isspace((int)(unsigned char)*p))
				break;

		if (!*p)
			continue;

		tz=p;
		while (*p)
		{
			if (isspace((int)(unsigned char)*p))
				break;
			++p;
		}
		if (*p) *p++=0;
		while (*p && isspace((int)(unsigned char)*p))
			++p;

		if (strcmp(p, "*") == 0)
			p="";
		if (strcmp(tz, "*") == 0)
			tz="";

		rc= (*callback_func)(tz, p, callback_arg);

		if (rc)
		{
			fclose(f);
			return (rc);
		}
	}
	fclose(f);
	return (0);
}

static int callback_timezonelist(const char *, const char *, void *);

static void timezonelist()
{
	printf("<select name=\"timezonelist\">");
	timezonefile(callback_timezonelist, NULL);
	printf("</select>\n");
}

static int callback_timezonelist(const char *tz, const char *n, void *dummy)
{
	printf("<option value=\"%s\">", tz);
	output_attrencoded(n);
	printf("\n");
	return (0);
}

static int set_timezone(const char *p)
{
	static char *s_buffer=0;
	char *buffer;

	if (!p || !*p || strcmp(p, "*") == 0)
		return (0);

	buffer=malloc(strlen(p)+10);
	if (!buffer)
		return (0);
	strcat(strcpy(buffer, "TZ="), p);

	putenv(buffer);

	if (s_buffer)
		free(buffer);
	s_buffer=buffer;

	return (0);
}

static int callback_get_timezone(const char *, const char *, void *);

/* Return TZ selected from login dropdown */

static char *get_timezone()
{
	char *langptr=0;

	timezonefile(callback_get_timezone, &langptr);

	if (!langptr)
	{
		langptr=strdup("");
		if (!langptr)
			enomem();
	}

	if (*langptr == 0)
	{
		free(langptr);
		langptr=strdup("*");
		if (!langptr)
			enomem();
	}

	return(langptr);
}

static int callback_get_timezone(const char *tz, const char *n, void *dummy)
{
	if (strcmp(tz, cgi("timezonelist")) == 0)
	{
		char **p=(char **)dummy;

		if (*p)
			free(*p);

		*p=strdup(tz);
	}
	return (0);
}

/* ------------------------ */

static FILE *do_open_form(const char *formname, int flag)
{
	struct template_stack *ts;
	FILE	*f=NULL;

	if ((ts=(struct template_stack *)malloc(sizeof(struct template_stack)))
	    == NULL)
		return (NULL);

	if (sqwebmail_content_language)
		f=open_langform(sqwebmail_content_language, formname, flag);
	if (!f)	f=open_langform(HTTP11_DEFAULTLANG, formname, flag);

	if (!f)
	{
		free(ts);
		return (NULL);
	}

	ts->next=template_stack;
	template_stack=ts;
	ts->fp=f;
	return (f);
}

static void do_close_form()
{
	struct template_stack *ts=template_stack;

	if (!ts)
		enomem();

	fclose(ts->fp);
	template_stack=ts->next;
	free(ts);
}

static void do_output_form_loop(FILE *);

void output_form(const char *formname)
{
	FILE	*f;

#ifdef	GZIP
	int	dogzip;
	int	pipefd[2];
	pid_t	pid= -1;
#endif

	noimages= access(NOIMAGES, 0) == 0;

	f=do_open_form(formname, 1);

	sqwebmail_formname=formname;

	if (!f)	error("Can't open form template.");

	/*
	** Except for the dummy frame window (and the tiny empty frame),
	** and the window containing the print preview of the message,
	** expire everything.
	*/

	if (strcmp(formname, "index.html") && strcmp(formname, "empty.html") &&
		strcmp(formname, "invalid.html"))
	{
		cginocache();
		//cginocache_msie();
	}
#ifdef	GZIP

	dogzip=0;
	if (strcmp(formname, "readmsg.html") == 0 ||
	    strcmp(formname, "folder.html") == 0 ||
	    strcmp(formname, "folders.html") == 0 ||
	    strcmp(formname, "gpg.html") == 0)
	{
	const char *p=getenv("HTTP_ACCEPT_ENCODING");

		if (p)
		{
		char	*q=strdup(p), *r;

			if (!q)	enomem();
			for (r=q; *r; r++)
				*r= tolower((int)(unsigned char)*r);
			for (r=q; (r=strtok(r, ", ")) != 0; r=0)
				if (strcmp(r, "gzip") == 0)
				{
					dogzip=1;
					if (pipe(pipefd))
						enomem();
				}
			free(q);
		}
	}
#endif

	/* Do not send a Vary header for attachment downloads */

//	if (*cgi("download") == 0)
//		printf("Vary: Accept-Language\n");

#ifdef	GZIP
	if (dogzip)
		printf("Content-Encoding: gzip\n");
#endif

	printf("Content-Type: text/html");

	if (sqwebmail_content_charset)
		printf("; charset=\"%s\"", sqwebmail_content_charset);

	printf("\n\n");

#ifdef	GZIP
	if (dogzip)
	{
		fflush(stdout);
		while ((pid=fork()) == -1)
			sleep(5);
		if (pid == 0)
		{
			close(0);
			dup(pipefd[0]);
			close(pipefd[0]);
			close(pipefd[1]);
			execl(GZIP, "gzip", "-c", (char *)0);
			syslog(LOG_DAEMON | LOG_CRIT,
			       "sqwebmail: Cannot execute " GZIP);
			exit(1);
		}

		gzip_save_fd=dup(1);
		close(1);
		dup(pipefd[1]);
		close(pipefd[1]);
		close(pipefd[0]);
	}
#endif

	do_output_form_loop(f);
	do_close_form();

#ifdef	GZIP
	if (pid > 0)
	{
	int	waitstat;
	pid_t	p2;

		/* Restore original stdout */

		fflush(stdout);
		close(1);
		dup(gzip_save_fd);
		close(gzip_save_fd);
		gzip_save_fd= -1;
		while ((p2=wait(&waitstat)) >= 0 && p2 != pid)
			;
	}
#endif
}

static FILE *openinclude(const char *);


void insert_include(const char *inc_name)
{
	FILE *ff=openinclude(inc_name);
	do_output_form_loop(ff);
	do_close_form();
}

static void do_output_form_loop(FILE *f)
{
	int	c, c2, c3;

	while ((c=getc(f)) >= 0)
	{
	char	kw[64];

		if (c != '[')
		{
			putchar(c);
			continue;
		}
		c=getc(f);
		if (c != '#')
		{
			putchar('[');
			ungetc(c,f);
			continue;
		}
		c=getc(f);
		if (c == '?')
		{
			c=getc(f);
			if (c < '0' || c > '9')
			{
				putchar('[');
				putchar('#');
				putchar('?');
				putchar(c);
				continue;
			}
			if (
			    ( c == '0' && auth_changepass == 0) ||
			    (c == '1' && *cgi("folderdir") == ':') ||
			    (c == '2' && *cgi("folderdir") != ':') ||
			    (c == '4' && maildir_filter_hasmaildirfilter(".")) ||
			    (c == '5' && has_gpg(GPGDIR)) ||
			    (c == '6' && !ishttps())
			    )
			{
				while ((c=getc(f)) != EOF)
				{
					if (c != '[')	continue;
					if ( getc(f) != '#')	continue;
					if ( getc(f) != '?')	continue;
					if ( getc(f) != '#')	continue;
					if ( getc(f) == ']')	break;
				}
			}
			continue;
		}

		if (c == '$')
		{
			struct var_put_buf buf;

			buf.argp=buf.argbuf;
			buf.argn=sizeof(buf.argbuf)-1;

			while ((c=getc(f)) >= 0 && c != '\n')
			{
				if (c == '#')
				{
					c=getc(f);
					if (c == ']')	break;
					ungetc(c, f);
					c='#';
				}

				if (c == '@')
				{
					c=getc(f);
					if (c == '@')
					{
						output_image(f,
							     var_put_buf_func,
							     &buf);
						continue;
					}
					ungetc(c, f);
					c='@';
				}
				var_put_buf_func(c, &buf);
			}
			*buf.argp=0;
			addarg(buf.argbuf);

			continue;
		}

		if (c == '@')
		{
			output_image(f, pass_image_through, NULL);
			c=getc(f);
			if (c == '#')
			{
				c=getc(f);
				if (c == ']')
					continue;
			}
			if (c != EOF)
				ungetc(c, f);
			continue;
		}

		if (!isalnum(c) && c != ':')
		{
			putchar('[');
			putchar('#');
			ungetc(c, f);
			continue;
		}
		c2=0;
		while (c != EOF && (isalnum(c) || c == ':' || c == '_'))
		{
			if (c2 < sizeof(kw)-1)
				kw[c2++]=c;
			c=getc(f);
		}
		kw[c2]=0;
		c2=c;

		if (c2 != '#')
		{
			putchar('[');
			putchar('#');
			printf("%s", kw);
			ungetc(c2, f);
			continue;
		}

		if ((c3=getc(f)) != ']')
		{
			putchar('[');
			putchar('#');
			printf("%s", kw);
			putchar(c2);
			ungetc(c3, f);
			continue;
		}

		if (strcmp(kw, "a") == 0)
		{
			addressbook();
		}
		else if (strcmp(kw, "d") == 0)
		{
			if (*cgi("folderdir"))
			{
			const char *c=cgi("folderdir");

				if (*c == ':')
					++c;	/* Sharable hierarchy */

				printf(" - ");
				print_safe(c);
			}
		}
		else if (strcmp(kw, "D") == 0)
		{
			const char *p=cgi("folder");
			const char *q=strrchr(p, '.');

				if (q)
				{
				char	*r=malloc(q-p+1);

					if (!r)	enomem();
					memcpy(r, p, q-p);
					r[q-p]=0;
					output_urlencoded(r);
					free(r);
				}
		}
		else if (strcmp(kw, "G") == 0)
		{
			output_attrencoded(login_returnaddr());
		}
		else if (strcmp(kw, "r") == 0)
		{
			output_attrencoded(cgi("redirect"));
		}
		else if (strcmp(kw, "s") == 0)
		{
			output_scriptptrget();
		}
		else if (strcmp(kw, "S") == 0)
		{
			output_loginscriptptr();
		}
		else if (strcmp(kw, "R") == 0)
		{
			output_loginscriptptr_get();
		}
		else if (strcmp(kw, "p") == 0)
		{
			output_scriptptr();
		}
		else if (strcmp(kw, "P") == 0)
		{
			output_scriptptrpostinfo();
		}
		else if (strcmp(kw, "f") == 0)
		{
			folder_contents_title();
		}
		else if (strcmp(kw, "F") == 0)
		{
			folder_contents(sqwebmail_folder, atol(cgi("pos")));
		}
		else if (strcmp(kw, "n") == 0)
		{
			folder_initnextprev(sqwebmail_folder, atol(cgi("pos")));
		}
		else if (strcmp(kw, "N") == 0)
		{
			folder_nextprev();
		}
		else if (strcmp(kw, "m") == 0)
		{
			folder_msgmove();
		}
		else if (strcmp(kw, "M") == 0)
		{
			folder_showmsg(sqwebmail_folder, atol(cgi("pos")));
		}
		else if (strcmp(kw, "T") == 0)
		{
			folder_showtransfer();
		}
		else if (strcmp(kw, "L") == 0)
		{
			folder_list();
		}
		else if (strcmp(kw, "l") == 0)
		{
			folder_list2();
		}
		// by lfan, support left frame
                else if (strcmp(kw, "L3") == 0)
		{
			folder_list3();
		}		
		// by lfan, support forward
		else if (strcmp(kw, "Fwd") == 0)
		{
			pref_forward();
		}
		//by roy support anti-spam setting
		else if (strcmp(kw, "SPAM") == 0) {
			pref_spam();
		}
		else if (strcmp(kw, "E") == 0)
		{
			folder_rename_list();
		}
		else if (strcmp(kw, "W") == 0)
		{
			newmsg_init(sqwebmail_folder, cgi("pos"));
		}
		else if (strcmp(kw, "z") == 0)
		{
			pref_isdisplayfullmsg();
		}
		else if (strcmp(kw, "y") == 0)
		{
			pref_isoldest1st();
		}
		else if (strcmp(kw, "H") == 0)
		{
			pref_displayhtml();
		}
		else if (strcmp(kw, "FLOWEDTEXT") == 0)
		{
			pref_displayflowedtext();
		}
		else if (strcmp(kw, "NOARCHIVE") == 0)
		{
			pref_displaynoarchive();
		}
		else if (strcmp(kw, "x") == 0)
		{
			pref_setprefs();
		}
		else if (strcmp(kw, "w") == 0)
		{
			pref_sortorder();
		}
		else if (strcmp(kw, "t") == 0)
		{
			pref_signature();
		}
		else if (strcmp(kw, "u") == 0)
		{
			pref_pagesize();
		}
		else if (strcmp(kw, "v") == 0)
		{
			pref_displayautopurge();
		}
		else if (strcmp(kw, "l1") == 0)
		{
			pref_getfrom();
		}
		else if (strcmp(kw, "A") == 0)
		{
			attachments_head(sqwebmail_folder, cgi("pos"),
				cgi("draft"));
		}
		else if (strcmp(kw, "ATTACHOPTS") == 0)
		{
			attachments_opts(cgi("draft"));
		}
		else if (strcmp(kw, "GPGERR") == 0)
		{
			sent_gpgerrtxt();
		}
		else if (strcmp(kw, "GPGERRRESUME") == 0)
		{
			sent_gpgerrresume();
		}
#ifdef	ISPELL
		else if (strcmp(kw, "K") == 0)
		{
			spell_show();
		}
#endif
		// modified by lfan, Display copyright
		else if (strcmp(kw, "B") == 0)
		{
			printf("<br><br><center class=csmtype>Powered by <a href=\"http://wmail.aka.cn\" target=_blank>WMail</a>&copy;</Center>");
		}
		else if (strcmp(kw, "h") == 0)
		{
			FILE *fp=fopen(LOGINDOMAINLIST, "r");

			if (fp) {
				/* parse LOGINDOMAINLIST and print proper output */
				print_logindomainlist(fp);
				fclose(fp);
			}
		}
		else if (strcmp(kw, "o") == 0)
		{
			ldaplist();
		}
		else if (strcmp(kw, "O") == 0)
		{
			doldapsearch();
		}
		else if (strcmp(kw, "LOADMAILFILTER") == 0)
		{
			mailfilter_init();
		}
		else if (strcmp(kw, "MAILFILTERLIST") == 0)
		{
			mailfilter_list();
		}
		else if (strcmp(kw, "MAILFILTERLISTFOLDERS") == 0)
		{
			mailfilter_listfolders();
		}
		else if (strcmp(kw, "QUOTA") == 0)
		{
			folder_showquota();
		}
		else if (strcmp(kw, "NICKLIST") == 0)
		{
		        ab_listselect();
		}
		else if (strcmp(kw, "LISTPUB") == 0)
		{
			gpglistpub();
		}
		else if (strcmp(kw, "LISTSEC") == 0)
		{
			gpglistsec();
		}
		else if (strcmp(kw, "KEYIMPORT") == 0)
		{
			folder_keyimport(sqwebmail_folder, atol(cgi("pos")));
		}
		else if (strcmp(kw, "GPGCREATE") == 0)
		{
			gpgcreate();
		}
		else if (strcmp(kw, "DOGPG") == 0)
		{
			gpgdo();
		}
		else if (strcmp(kw, "ATTACHPUB") == 0)
		{
			gpgselectpubkey();
		}
		else if (strcmp(kw, "ATTACHSEC") == 0)
		{
			gpgselectprivkey();
		}
		else if (strcmp(kw, "MAILINGLISTS") == 0)
		{
			char *p=getmailinglists();

			/* <sigh> amaya inserts a bunch of spaces that mess
			** things up in Netscape.
			*/

			printf("<textarea style=\"font-family: courier;\" cols=\"40\" rows=\"4\" "
			       "name=\"mailinglists\" wrap=soft>");
			output_attrencoded(p ? p:"");
			if (p)
				free(p);
			printf("</textarea>");
		}
		else if (strcmp(kw, "AUTORESPONSE") == 0)
		{
			autoresponse();
		}
		else if (strcmp(kw, "AUTORESPONSE_LIST") == 0)
		{
			autoresponselist();
		}
		else if (strcmp(kw, "AUTORESPONSE_PICK") == 0)
		{
			autoresponsepick();
		}
		else if (strcmp(kw, "AUTORESPONSE_DELETE") == 0)
		{
			autoresponsedelete();
		}
		else if (strcmp(kw, "SQWEBMAILCSS") == 0)
		{
			printf(IMGPATH "/sqwebmail.css");
		}
		else if (strcmp(kw, "timezonelist") == 0)
		{
			timezonelist();
		}
		else if (strcmp(kw, "PREFWEEK") == 0)
		{
			pref_displayweekstart();
		}
/* by lfan, remove calendar
		else if (strcmp(kw, "NEWEVENT") == 0)
		{
			sqpcp_newevent();
		}
		else if (strcmp(kw, "RECURRING") == 0)
		{
			printf("%s", getarg("RECURRING"));
		}
		else if (strcmp(kw, "EVENTSTART") == 0)
		{
			sqpcp_eventstart();
		}
		else if (strcmp(kw, "EVENTEND") == 0)
		{
			sqpcp_eventend();
		}
		else if (strcmp(kw, "EVENTFROM") == 0)
		{
			sqpcp_eventfrom();
		}
		else if (strcmp(kw, "EVENTTIMES") == 0)
		{
			sqpcp_eventtimes();
		}
		else if (strcmp(kw, "EVENTPARTICIPANTS") == 0)
		{
			sqpcp_eventparticipants();
		}
		else if (strcmp(kw, "EVENTTEXT") == 0)
		{
			sqpcp_eventtext();
		}
		else if (strcmp(kw, "EVENTATTACH") == 0)
		{
			sqpcp_eventattach();
		}
		else if (strcmp(kw, "EVENTSUMMARY") == 0)
		{
			sqpcp_summary();
		}
		else if (strcmp(kw, "CALENDARTODAY") == 0)
		{
			sqpcp_todays_date();
		}
		else if (strcmp(kw, "CALENDARWEEKLYLINK") == 0)
		{
			sqpcp_weeklylink();
		}
		else if (strcmp(kw, "CALENDARMONTHLYLINK") == 0)
		{
			sqpcp_monthlylink();
		}
		else if (strcmp(kw, "CALENDARTODAYV") == 0)
		{
			sqpcp_todays_date_verbose();
		}
		else if (strcmp(kw, "CALENDARDAYVIEW") == 0)
		{
			sqpcp_daily_view();
		}
		else if (strcmp(kw, "CALENDARPREVDAY") == 0)
		{
			sqpcp_prevday();
		}
		else if (strcmp(kw, "CALENDARNEXTDAY") == 0)
		{
			sqpcp_nextday();
		}
		else if (strcmp(kw, "CALENDARWEEK") == 0)
		{
			sqpcp_show_cal_week();
		}
		else if (strcmp(kw, "CALENDARNEXTWEEK") == 0)
		{
			sqpcp_show_cal_nextweek();
		}
		else if (strcmp(kw, "CALENDARPREVWEEK") == 0)
		{
			sqpcp_show_cal_prevweek();
		}
		else if (strcmp(kw, "CALENDARWEEKVIEW") == 0)
		{
			sqpcp_displayweek();
		}
		else if (strcmp(kw, "CALENDARMONTH") == 0)
		{
			sqpcp_show_cal_month();
		}
		else if (strcmp(kw, "CALENDARNEXTMONTH") == 0)
		{
			sqpcp_show_cal_nextmonth();
		}
		else if (strcmp(kw, "CALENDARPREVMONTH") == 0)
		{
			sqpcp_show_cal_prevmonth();
		}
		else if (strcmp(kw, "CALENDARMONTHVIEW") == 0)
		{
			sqpcp_displaymonth();
		}
		else if (strcmp(kw, "EVENTDISPLAYINIT") == 0)
		{
			sqpcp_displayeventinit();
		}
		else if (strcmp(kw, "EVENTDELETEINIT") == 0)
		{
			sqpcp_deleteeventinit();
		}
		else if (strcmp(kw, "EVENTDISPLAY") == 0)
		{
			sqpcp_displayevent();
		}
		else if (strcmp(kw, "EVENTBACKLINK") == 0)
		{
			sqpcp_eventbacklink();
		}
		else if (strcmp(kw, "EVENTEDITLINK") == 0)
		{
			sqpcp_eventeditlink();
		}
		else if (strcmp(kw, "EVENTCANCELUNCANCELLINK") == 0)
		{
			sqpcp_eventcanceluncancellink();
		}
		else if (strcmp(kw, "EVENTCANCELUNCANCELLINK") == 0)
		{
			sqpcp_eventcanceluncancellink();
		}
		else if (strcmp(kw, "EVENTCANCELUNCANCELIMAGE") == 0)
		{
			sqpcp_eventcanceluncancelimage();
		}
		else if (strcmp(kw, "EVENTCANCELUNCANCELTEXT") == 0)
		{
			sqpcp_eventcanceluncanceltext();
		}
		else if (strcmp(kw, "EVENTDELETELINK") == 0)
		{
			sqpcp_eventdeletelink();
		}
		else if (strcmp(kw, "EVENTACL") == 0)
		{
			sqpcp_eventacl();
		}
*/
		else if (strcmp(kw, "ABOOKNAMELIST") == 0)
		{
			ab_addrselect();
		}
		// by lfan
	        else if (strcmp(kw, "LFANTODAY") == 0 )
		{
                        char date_buf[1024], *p;
                        int len = 0;
                        time_t now = time(NULL);
			p = "%YƒÍ%m‘¬%d»’ %A";
                        len = strftime(date_buf, sizeof(date_buf)-1, p, localtime(&now));
                        date_buf[len]=0;
                        printf(date_buf);			
		}
		// by lfan, display send ok return addr
		else if (strcmp(kw, "WATERNIGHT") == 0 )
		{
			char buf[1024];
			
			if( strcmp( cgi("pos"), "-1" ) == 0 ) {
				sprintf( buf, "&form=folders" );
			}
			else {
				sprintf( buf, "&form=readmsg&pos=%s", cgi("pos") );
			}
			printf(buf);
		}
		else if (strncmp(kw, "radio:", 6) == 0)
		{
		const char *name=strtok(kw+6, ":");
		const char *value=strtok(0, ":");

			if (name && value)
			{
				printf("<input type=\"radio\" name=\"%s\""
					" value=\"%s\"",
					name, value);
				if ( strcmp(cgi(name), value) == 0)
					printf(" checked");
				printf(">");
			}
		}
		else if (strncmp(kw, "checkbox:", 9) == 0)
		{
		const char *name=strtok(kw+9, ":");
		const char *cgivar=strtok(0, ":");

			if (name && cgivar)
			{
				printf("<input type=\"checkbox\" name=\"%s\""
					"%s>",
					name,
					*cgi(cgivar) ? " checked":"");
			}
		}
		else if (strncmp(kw, "input:", 6) == 0)
		{
			output_attrencoded(cgi(kw+6));
		}
		else if (strncmp(kw, "select:", 7) == 0)
		{
		const char *name=strtok(kw+7, ":");
		const char *size=strtok(0, ":");

			printf("<select name=\"%s\"", name ? name:"");
			if (size)	printf(" size=%s", size);
			printf(">");
		}
		else if (strncmp(kw, "option:", 7) == 0)
		{
		const char *name=strtok(kw+7, ":");
		const char *cgivar=strtok(0, ":");
		const char *cgival=strtok(0, ":");

			printf("<option value=\"%s\"", name ? name:"");
			if (cgivar && cgival &&
				strcmp(cgi(cgivar), cgival) == 0)
				printf(" selected");
			printf(">");
		}
		else if (strcmp(kw, "endselect") == 0)
			printf("</select>");
		else if (strncmp(kw, "env:", 4) == 0) {
			const char *val = getenv(kw+4);
			if (val) output_attrencoded(val);
		}
		else if (strncmp(kw, "include:", 8) == 0)
		{
			insert_include(kw+8);
		}
		else if (strcmp(kw, "endinclude") == 0)
		{
			break;
		}
	}
}

/* Include another template file */

static FILE *openinclude(const char *p)
{
	char buffer[BUFSIZ];
	FILE *f;

	buffer[0]=0;
	strncat(buffer, p, 100);
	strcat(buffer, ".inc.html");

	f=do_open_form(buffer, 0);

	if (!f)
		error("Can't open form template.");

	while (fgets(buffer, sizeof(buffer), f))
	{
		const char *p=strchr(buffer, '[');

		if (!p)
			continue;

		if (strncmp(p, "[#begininclude#]", 16) == 0)
		{
			break;
		}
	}
	return (f);
}


/* Top level HTTP redirect without referencing a particular mailbox */

static void http_redirect_top(const char *app)
{
const	char *p=nonloginscriptptr();
char	*buf=malloc(strlen(p)+strlen(app)+2);

	if (!buf)	enomem();
	strcat(strcpy(buf, p), app);
	cgiredirect(buf);
	free(buf);
}

/* HTTP redirects within a given mailbox, various formats */

void http_redirect_argu(const char *fmt, unsigned long un)
{
char	buf[MAXLONGSIZE];

	sprintf(buf, "%lu", un);
	http_redirect_argss(fmt, buf, "");
}

void http_redirect_argss(const char *fmt, const char *arg1, const char *arg2)
{
	http_redirect_argsss(fmt, arg1, arg2, "");
}

void http_redirect_argsss(const char *fmt, const char *arg1, const char *arg2,
	const char *arg3)
{
char *base=scriptptrget();
char *arg1s=cgiurlencode(arg1);
char *arg2s=cgiurlencode(arg2);
char *arg3s=cgiurlencode(arg3);
char *q;

	/* We generate a Location: redirected_url header.  The actual
	** header is generated in cgiredirect, we just build it here */

	q=malloc(strlen(base)+strlen(fmt)+strlen(arg1s)+strlen(arg2s)+
			strlen(arg3s)+1);
	if (!q)	enomem();
	strcpy(q, base);
	sprintf(q+strlen(q), fmt, arg1s, arg2s, arg3s);
	cgiredirect(q);
	free(q);
	free(arg1s);
	free(arg2s);
	free(arg3s);
	free(base);
}

void output_user_form(const char *formname)
{
char	*p;

	if (!*formname || strchr(formname, '.') || strchr(formname, '/'))
		error("Invalid request.");

	if ((strcmp(formname, "filter") == 0
	     || strcmp(formname, "autoresponse") == 0)
	    && maildir_filter_hasmaildirfilter("."))
		/* Script kiddies... */
		formname="nofilter";

	if (strcmp(formname, "filter") == 0 && *cgi("do.submitfilter"))
		mailfilter_submit();

	if (strcmp(formname, "gpg") == 0 && has_gpg(GPGDIR))
		error("Invalid request.");

	if (strcmp(formname, "gpgcreate") == 0 && has_gpg(GPGDIR))
		error("Invalid request.");

	if (*cgi("ldapsearch"))	/* Special voodoo for LDAP address book stuff */
	{
		if (ldapsearch() == 0)
		{
			output_form("ldapsearch.html");
			return;
		}
	}

	/*
	** In order to hide the session ID in the URL of the message what
	** we do is that the initial URL, that contains setcookie=1, results
	** in us setting a temporary cookie that contains the session ID,
	** then we return a redirect to a url which has /printmsg/ in the
	** PATH_INFO, instead of the session ID.  The code in main()
	** traps /printmsg/ PATH_INFO, fetches the path info from the
	** cookie, and punts after resetting setcookie to 0.
	*/

	if (strcmp(formname, "print") == 0 && *cgi("setcookie") == '1')
	{
	const char *qs=getenv("QUERY_STRING");
	const char *pi=getenv("PATH_INFO");
	const char *nl;
	char	*buf;

		if (!pi)	pi="";
		if (!pi)	pi="";

		nl=nonloginscriptptr();

		buf=malloc(strlen(nl) + sizeof("/printmsg/print?")+strlen(qs));
		if (!buf)	enomem();
		strcat(strcat(strcpy(buf, nl), "/printmsg/print?"), qs);
		cginocache();
		cgi_setcookie("sqwebmail-pi", pi);
		printf("Refresh: 0; URL=\"%s\"\n", buf);
		free(buf);
		output_form("printredirect.html");
		return;
	}

	if (strcmp(cgi("fromscreen"), "mailfilter") == 0)
		maildir_filter_endmaildirfilter(".");	/* Remove the temp file */

	if (strcmp(formname, "logout") == 0)
	{
		// by lfan, remove cookie
		cginocache();
		cgi_setcookie("wmail-session", "NO");
		unlink(IPFILE);
		http_redirect_top("");
		return;
	}

	if (strcmp(formname, "fetch") == 0)
	{
		folder_download( sqwebmail_folder, atol(cgi("pos")),
			cgi("mimeid") );
		return;
	}

	if (strcmp(formname, "delmsg") == 0)
	{
		folder_delmsg( atol(cgi("pos")));
		return;
	}

	if (strcmp(formname, "donewmsg") == 0)
	{
		newmsg_do(sqwebmail_folder);
		return;
	}

	if (strcmp(formname, "doattach") == 0)
	{
		doattach(sqwebmail_folder, cgi("draft"));
		return;
	}

	if (strcmp(formname, "folderdel") == 0)
	{
		folder_delmsgs(sqwebmail_folder, atol(cgi("pos")));
		return;
	}
	if (strcmp(formname, "spellchk") == 0)
	{
#ifdef	ISPELL
		spell_check_continue();
#else
		printf("Status: 404");
#endif
		return;
	}
/*
	if (sqpcp_loggedin())
	{
		if (*cgi("do.neweventpreview"))
		{
			sqpcp_preview();
			return;
		}

		if (*cgi("do.neweventsave"))
		{
			sqpcp_save();
			return;
		}

		if (*cgi("do.neweventpostpone"))
		{
			sqpcp_postpone();
			return;
		}

		if (*cgi("do.neweventdeleteattach"))
		{
			sqpcp_deleteattach();
			return;
		}

		if (*cgi("do.neweventupload"))
		{
			sqpcp_uploadattach();
			return;
		}

		if (*cgi("do.neweventuppubkey"))
		{
			sqpcp_attachpubkey();
			return;
		}

		if (*cgi("do.neweventupprivkey"))
		{
			sqpcp_attachprivkey();
			return;
		}
		if (*cgi("do.eventdelete"))
		{
			sqpcp_dodelete();
			return;
		}
	}

	if (strcmp(formname, "event-edit") == 0)
	{
		formname="folders";
		if (sqpcp_loggedin())
		{
			formname="eventshow";	// default
			if (sqpcp_eventedit() == 0)
				formname="newevent";
		}
	}
*/


	if (strcmp(formname, "open-draft") == 0)
	{
		formname="newmsg";
		/*
		if (sqpcp_has_calendar())
			// DRAFTS may contain event files 
		{
			const char *n=cgi("draft");
			char *filename;
			FILE *fp;

			CHECKFILENAME(n);

			filename=maildir_find(DRAFTS, n);

			if (filename)
			{
				if ((fp=fopen(filename, "r")) != NULL)
				{
					struct rfc822hdr h;

					rfc822hdr_init(&h, 8192);

					while (rfc822hdr_read(&h, fp, NULL, 0)
					       == 0)
					{
						if (strcasecmp(h.header,
							       "X-Event") == 0)
						{
							formname="newevent";
							cgi_put("draftmessage",
								cgi("draft"));
							break;
						}
					}
					rfc822hdr_free(&h);
					fclose(fp);
				}
				free(filename);
			}
		}
		*/
	}
/*
	if (strcmp(formname, "newevent") == 0 ||
	    strcmp(formname, "eventdaily") == 0 ||
	    strcmp(formname, "eventweekly") == 0 ||
	    strcmp(formname, "eventmonthly") == 0 ||
	    strcmp(formname, "eventshow") == 0 ||
	    strcmp(formname, "eventacl") == 0)
	{
		if (!sqpcp_has_calendar() ||
		    !sqpcp_loggedin())
			formname="folders";	// Naughty boy 
	}
*/
	p=malloc(strlen(formname)+6);
	if (!p)	enomem();

	strcat(strcpy(p, formname),".html");
	output_form(p);
	free(p);
}

#if 0
#if ENABLE_WEBPASS

/* System-compatible crypt function */

static const char *auth_crypt(const char *password, const char *salt)
{
#if     HAVE_CRYPT
	if (strncmp(salt, "$1$", 3))
		return (crypt(password, salt));
#endif
	return (md5_crypt(password, salt));
}

/*
** Support for sqwebmail-only password.  Versions prior to 0.12 stored
** it in clear test.  0.12 and later use crypt(), but still are able to
** handle cleartext.
**
** flag is non-zero if the authentication module does not support password
** changes, therefore we need to create sqwebmail-webpass.
*/

int check_sqwebpass(const char *pass)
{
const char *p=read_sqconfig(".", PASSFILE, 0);

	if (p && *p == '\t')
					/* Starts with \t - crypt */
	{
		++p;
		if (strcmp(p, auth_crypt(pass, p)) == 0)	return (0);
		return (-1);
	}

	if (p && strcmp(p, pass) == 0)	return (0);
	return (-1);
}

int has_sqwebpass()
{
	return (read_sqconfig(".", PASSFILE, 0) ? 1:0);
}
#endif

#if 0
static const char salt[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

void set_sqwebpass(const char *pw)
{
char	buf[100];
char	buf2[100];
const char *r=random128();
unsigned i;
#ifdef WEBPASS_CHANGE_VCHKPW
const char *p;
#endif
	/* r is 16 bytes.  We need 12 bits to initialize the salt */

	buf2[4]=0;

	i=0;
	while (*r)
	{
		i <<= 1;
		i |= (*r & 1);
		++r;
	}

	buf[0]= salt[ i & 63];
	i /= 64;
	buf[1]= salt[ i & 63 ];
	buf[2]= 0;

	strcpy(buf, auth_crypt(pw, buf));

	buf2[0]='\t';
	strcpy(buf2+1, buf);

#ifdef WEBPASS_CHANGE_VCHKPW
	if(( p=login_returnaddr() ))
	{
		char *user;
		char *domain;
		char *p2;
		char *pw2;

		p2=strdup(p);
		pw2=strdup(pw);
		user=strtok(p2, "@");
		domain=strtok(0, "@");
		if(vpasswd(user,domain,pw2,0) == 0)
			write_sqconfig(".", PASSFILE, buf2);
		free(p2);
		free(pw2);
		return;
	}
#endif
	write_sqconfig(".", PASSFILE, buf2);
}
#endif
#endif

extern void folder_cleanup();
extern void maildir_cleanup();
extern void mailfilter_cleanup();

#ifdef	ISPELL
extern void ispell_cleanup();
#endif

void cleanup()
{
	sqwebmail_formname = NULL;
	sqwebmail_mailboxid=0;
	sqwebmail_folder=0;
	sqwebmail_sessiontoken=0;
	sqwebmail_content_language=0;
	sqwebmail_content_locale=0;
	sqwebmail_content_ispelldict=0;
	folder_cleanup();
	maildir_cleanup();
	mailfilter_cleanup();
#ifdef ISPELL
	ispell_cleanup();
#endif

#ifdef GZIP
	if (gzip_save_fd >= 0)	/* Restore original stdout */
	{
		close(1);
		dup(gzip_save_fd);
		close(gzip_save_fd);
		gzip_save_fd= -1;
	}
#endif

#if HAVE_SQWEBMAIL_UNICODE
	gpg_cleanup();
#endif
	freeargs();
	//sqpcp_close();
}

#if HAVE_SQWEBMAIL_UNICODE

#else

/* Head it off at the pass */

int has_gpg(const char *dummy)
{
	return (-1);
}
#endif

static RETSIGTYPE catch_sig(int n)
{
	n=n;
	cleanup();
	fake_exit(0);
#if RETSIGTYPE != void
	return (0);
#endif
}

static void setlang()
{
	static char *lang_buf=0;
	char *p;

	if (sqwebmail_content_locale && *sqwebmail_content_locale
	    && (p=malloc(sizeof("LANG=")+strlen(sqwebmail_content_locale)))!=0)
	{
		strcat(strcpy(p, "LANG="), sqwebmail_content_locale);
		putenv(p);
		if (lang_buf)
			free(lang_buf);
		lang_buf=p;
	}
}

static void init_default_locale()
{
char	*cl=http11_best_content_language(HTMLLIBDIR,
			//by lfan, only Chinese
			//getenv("HTTP_ACCEPT_LANGUAGE"));
			"zh-cn");

	sqwebmail_content_language=
				http11_content_language(HTMLLIBDIR, cl);
	sqwebmail_content_locale=
			http11_content_locale(HTMLLIBDIR, cl);
	sqwebmail_content_ispelldict=
			http11_content_ispelldict(HTMLLIBDIR, cl);
	sqwebmail_content_charset=
			http11_content_charset(HTMLLIBDIR, cl);

	free(cl);
#if	HAVE_LOCALE_H
#if	HAVE_SETLOCALE
	setlocale(LC_ALL, sqwebmail_content_locale);
	setlocale(LC_CTYPE, "C");
	setlang();
#endif
#endif
}

static void rename_sent_folder()
{
	char buf[128], buf2[200];
	time_t t;
	struct tm *tm;
	struct stat stat_buf;
	char *pp;

	time(&t);
	tm=localtime(&t);
	if (!tm)
		return;

	if (tm->tm_mon == 0)
	{
		tm->tm_mon=11;
		--tm->tm_year;
	}
	else
		--tm->tm_mon;

	if (strftime (buf, sizeof(buf), "." SENT ".%Y.%m-%b", tm) == 0)
		return;

	strcat(strcpy(buf2, "." SENT "/sqwebmail"), buf);

	if (stat(buf2, &stat_buf) == 0)
		return;	/* Already renamed */

	pp=folder_toutf7(buf);

	rename("." SENT, pp);
	free(pp);
	(void)maildir_create(SENT);
	close(open(buf2, O_RDWR|O_CREAT, 0600));
}

static int valid_redirect();

static void redirect(const char *url)
{
	if (valid_redirect())
	{
		printf("Refresh: 0; URL=\"%s\"\n", url);
		output_form("redirect.html");
		return;
	}

	printf("Content-Type: text/plain\n\n"
	       "The URL you clicked on is no longer valid.\n");
	return;
}

static int valid_redirect()
{
	const char *timestamp=cgi("timestamp"), *p;
	unsigned long timestamp_n;
	time_t timestamp_t;
	time_t now;

	if (sscanf(timestamp, "%lu", &timestamp_n) != 1)
		return 0;

	timestamp_t=(time_t)timestamp_n;
	time(&now);

	if (now < timestamp_t || now > timestamp_t + TIMEOUTHARD)
		return 0;

	p=redirect_hash(timestamp);

	if (*p == 0 || strcmp(cgi("md5"), p))
		return 0;
	return 1;
}

static int main2();

int main()
{
int	rc;

	/* If we are running setuid non-root, change our real gid/uid too */
	if (getegid()) setgid(getegid());
	if (geteuid()) setuid(geteuid());

#if	HAVE_LIBFCGI

	while ( FCGI_Accept() >= 0)
	{
		if (setjmp(stackenv) == 0)
		{
			rc=main2();
			cleanup(); /* make sure we hit it */
			FCGI_SetExitStatus(rc);
			continue;
		}
		cleanup(); /* make sure we hit it */

		signal(SIGHUP, catch_sig);
		signal(SIGINT, catch_sig);
		signal(SIGPIPE, catch_sig);
		signal(SIGTERM, catch_sig);
	}
	return (0);
#else
	rc=main2();
	cleanup();
	exit(rc);
	return (rc);
#endif
}

static int setuidgid(uid_t u, gid_t g, const char *dir, void *dummy)
{
	if (setgid(g) || setuid(u))
	{
		syslog(LOG_DAEMON | LOG_CRIT,
			"sqwebmail: Cache - can't setuid/setgid to %u/%u",
		       (unsigned)u, (unsigned)g);
		return (-1);
	}

	if (chdir(dir))
	{
		syslog(LOG_DAEMON | LOG_CRIT,
			"sqwebmail: Cache - can't chdir to %s", dir);
		return (-1);
	}
	return (0);
}

static int main2()
{
const char	*u;
const char	*ip_addr;
char	*pi;
char	*pi_malloced;
int	reset_cookie=0;
time_t	timeouthard=TIMEOUTHARD;


#ifdef	GZIP
	gzip_save_fd= -1;
#endif
	u=ip_addr=pi=NULL;

	ip_addr=getenv("REMOTE_ADDR");

	signal(SIGHUP, catch_sig);
	signal(SIGINT, catch_sig);
	signal(SIGPIPE, catch_sig);
	signal(SIGTERM, catch_sig);

	if (!ip_addr)	ip_addr="127.0.0.1";

	umask(0077);

	{
		const char *p=getenv("SQWEBMAIL_TIMEOUTHARD");

		if (p && *p)
			timeouthard=atoi(p);
	}

	if (maildir_cache_init(timeouthard, CACHEDIR, CACHEOWNER, authvars))
	{
		printf("Content-Type: text/plain\n\nmaildir_cache_init() failed\n");
		fake_exit(0);
	}

	pi=getenv("PATH_INFO");

	pi_malloced=0;
	//sqpcp_init();

	if (pi && strncmp(pi, "/printmsg/", 10) == 0)
	{
		/* See comment in output_user_form */

		pi_malloced=pi=cgi_cookie("sqwebmail-pi");
		if (*pi_malloced == 0)
		{
			free(pi_malloced);
			setgid(getgid());
			setuid(getuid());
			output_form("printnocookie.html");
			return (0);
		}
		reset_cookie=1;
		cgi_setcookie("sqwebmail-pi", "DELETED");
	}

	if (pi && strncmp(pi, "/wn/", 4) == 0)
	{
	const char	*p;
	time_t	last_time, current_time;
	char	*q;
	time_t	timeoutsoft=TIMEOUTSOFT;

		p=getenv("SQWEBMAIL_TIMEOUTSOFT");

		if (p && *p)
			timeoutsoft=atoi(p);

		/* Logging into the mailbox */

		pi=strdup(pi);
		if (pi_malloced)	free(pi_malloced);

		if (!pi)	enomem();

		(void)strtok(pi, "/");	/* Skip login */
		//u=strtok(NULL, "/");	/* mailboxid */
		u=cgi_cookie("wmail-login");
		//sqwebmail_sessiontoken=strtok(NULL, "/"); /* sessiontoken */
		sqwebmail_sessiontoken=cgi_cookie("wmail-session");
		//q=strtok(NULL, "/");	/* login time */
		q=cgi_cookie("wmail-time");
		login_time=0;
		while (q && *q >= '0' && *q <= '9')
			login_time=login_time * 10 + (*q++ - '0');

		if (maildir_cache_search(u, login_time, setuidgid, NULL)
		    && prelogin(u))
		{
			free(pi);
			error("Unable to access your mailbox, sqwebmail permissions may be wrong.");
		}

		time(&current_time);

		/* Ok, boys and girls, time to validate the connection as
		** follows */

		if (	!sqwebmail_sessiontoken

		/* 1. Read IPFILE.  Check that it's timestamp is current enough,
		** and the session hasn't timed out.
		*/

			|| !(p=read_sqconfig(".", IPFILE, &last_time))

/*			|| last_time > current_time	*/

			|| last_time + timeouthard < current_time

		/* 2. IPFILE will contain seven words - IP address, session
		** token, language, locale, ispell dictionary,
		** timezone, charset.  Validate both.
		*/
			|| !(q=strdup(p))
			|| !(p=strtok(q, " "))
			|| (strcmp(p, ip_addr) && strcmp(p, "none"))
			|| !(p=strtok(NULL, " "))
			|| strcmp(p, sqwebmail_sessiontoken)
			|| !(p=strtok(NULL, " "))
			|| !(sqwebmail_content_language=strdup(p))
			|| !(p=strtok(NULL, " "))
			|| !(sqwebmail_content_locale=strdup(p))
			|| !(p=strtok(NULL, " "))
			|| !(sqwebmail_content_ispelldict=strdup(p))
			|| !(p=strtok(NULL, " "))
			|| set_timezone(p)
			|| !(p=strtok(NULL, " "))
			|| !(sqwebmail_content_charset=strdup(p))

		/* 3. Check the timestamp on the TIMESTAMP file.  See if the
		** session has reached its soft timeout.
		*/

			|| !read_sqconfig(".", TIMESTAMP, &last_time)

/*			|| last_time > current_time	*/

			|| last_time + timeoutsoft < current_time)
		{
			setgid(getgid());
			setuid(getuid());	/* Drop root prevs */
			chdir("/");
			cgi_setup();
			init_default_locale();
			free(pi);

			if (strcmp(cgi("form"), "logout") == 0)
				/* Already logged out, and the link
				** had target=_parent tag.
				*/
			{
				http_redirect_top("");
				return (0);
			}
			output_form("expired.html");
			return (0);
		}
		free(q);
		cgiformdatatempdir("tmp");
		cgi_setup();	/* Read CGI environment */
		if (reset_cookie)
			cgi_put("setcookie", "0");

		/* Update soft timeout stamp */

		write_sqconfig(".", TIMESTAMP, "");

		/* We must always have the folder CGI arg */

		if (!*(sqwebmail_folder=cgi("folder")))
		{
			init_default_locale();
			output_form("expired.html");
			free(pi);
			return (0);
		}

		CHECKFILENAME(sqwebmail_folder);

		sqwebmail_mailboxid=u;
#if	HAVE_LOCALE_H
#if	HAVE_SETLOCALE
		setlocale(LC_ALL, sqwebmail_content_locale);
		setlocale(LC_CTYPE, "C");
		setlang();
#endif
#endif
		pref_init();
		//(void)sqpcp_loggedin();
		output_user_form(cgi("form"));
		free(pi);
	}
	else
		/* Must be one of those special forms */
	{
	char	*rm;
	long	n;

		if (pi_malloced)	free(pi_malloced);

		if ((rm=getenv("REQUEST_METHOD")) == 0 ||
			(strcmp(rm, "POST") == 0 &&
				((rm=getenv("CONTENT_TYPE")) != 0 &&
				strncasecmp(rm,"multipart/form-data;", 20)
					== 0)))
			enomem();	/* multipart/formdata posts not allowed
					*/

		/* Some additional safety checks */

		rm=getenv("CONTENT_LENGTH");
		n= rm ? atol(rm):0;
		if (n < 0 || n > 256)	enomem();

		cgi_setup();
		init_default_locale();

		if (*(u=cgi("username")))
			/* Request to log in */
		{
		const char *p=cgi("password");
		const char *mailboxid;
		const char *u2=cgi("domain");
		char	*ubuf=malloc(strlen(u)+strlen(u2)+2);
		int can_changepwd;

			strcpy(ubuf, u);
			if (*u2)
				strcat(strcat(ubuf, "@"), u2);
			else if( strchr(ubuf, '@') == 0 ) {
				// by lfan, add default domain
				char domain[256];
				bzero( domain, 256 );
				vset_default_domain( domain );
				if( strlen(domain) > 0 ) {
					free(ubuf);
					ubuf=malloc(strlen(u)+strlen(domain)+2);
					strcpy(ubuf, u);
					strcat(strcat(ubuf, "@"), domain);
				}
			}

			maildir_cache_start();
			if (*p && (mailboxid=login(ubuf, p, &can_changepwd))
			    != 0)
			{
				char	*q;
				const	char *saveip=ip_addr;
				char	*tz;
				char	*p;
				time_t  last_time, current_time;
			
				// add by lfan, support concurrent login
				time(&current_time);	
				p=read_sqconfig(".", IPFILE, &last_time);
				if( p && (last_time + timeouthard >= current_time) ) {
					char	*pp;
					time_t  timeoutsoft=TIMEOUTSOFT;

					pp=getenv("SQWEBMAIL_TIMEOUTSOFT");

					if (pp && *pp)
						timeoutsoft=atoi(pp);
				
					if( (pp=strdup(p)) && (p=strtok(pp, " ")) 
						&& (strcmp(p, ip_addr) == 0 || strcmp(p, "none") == 0)
						&& (p=strtok(NULL, " "))  
						&& read_sqconfig(".", TIMESTAMP, &last_time)
						&& last_time + timeoutsoft >= current_time )
					{
						sqwebmail_sessiontoken=strdup(p);
						free(pp);
					}
					else
						sqwebmail_sessiontoken=random128();
				}
				else
					sqwebmail_sessiontoken=random128();

#if 0
#if ENABLE_WEBPASS
				if (!can_changepwd && !has_sqwebpass())
					set_sqwebpass(p);
#endif
#endif
				sqwebmail_mailboxid=mailboxid;
				sqwebmail_folder="INBOX";
				//sqwebmail_sessiontoken=random128();

				tz=get_timezone();
				if (*cgi("nosecure") != 0)
					saveip="none";

				q=malloc(strlen(saveip)
					 +strlen(sqwebmail_sessiontoken)
					 +strlen(sqwebmail_content_language)
					 +strlen(sqwebmail_content_ispelldict)
					 +strlen(sqwebmail_content_charset)
					 +strlen(tz)
					 +strlen(sqwebmail_content_locale)+7);
				if (!q)	enomem();
				sprintf(q, "%s %s %s %s %s %s %s", saveip,
					sqwebmail_sessiontoken,
					sqwebmail_content_language,
					sqwebmail_content_locale,
					sqwebmail_content_ispelldict,
					tz,
					sqwebmail_content_charset);
				write_sqconfig(".", IPFILE, q);
				free(q);
				free(tz);
				time(&login_time);
				{
					char buf[1024];

					buf[sizeof(buf)-1]=0;
					if (getcwd(buf, sizeof(buf)-1) == 0)
					{
						syslog(LOG_DAEMON,
						       "sqwebmail: "
						       "getcwd() failed");
						fake_exit(1);
					} /* oops */

					maildir_cache_save(mailboxid,
							   login_time,
							   buf,
							   geteuid(), getegid()
							   );

				}
				write_sqconfig(".", TIMESTAMP, "");
#if	HAVE_LOCALE_H
#if	HAVE_SETLOCALE
				setlocale(LC_ALL, sqwebmail_content_locale);
				setlocale(LC_CTYPE, "C");
				setlang();
#endif
#endif
				(void)maildir_create(DRAFTS);
				{
					const char *autorenamesent=AUTORENAMESENT;
					const char *p=getenv("SQWEBMAIL_AUTORENAMESENT");
					if (p && *p)
						autorenamesent = p;
					/* by lfan, disable rename sent folder
					if ( strncmp(autorenamesent, "no", 2) != 0 )
						(void)rename_sent_folder();
					*/
				}
				(void)maildir_create(SENT);
				(void)maildir_create(TRASH);
				pref_init();
				if (pref_autopurge > 0) maildir_autopurge();

				//sqpcp_login(ubuf, p);
				// by lfan, correct maildirsize error
				{
					struct vqpasswd *mypw;
					char *p, *p1, *p2;
					if( strchr( ubuf, '@' ) ) {
						p = strdup( ubuf );
			                	p1 = strtok(p, "@");
						p2 = strtok(0, "@");
					} else {
						p = 0;
						p1 = u;
						p2 = "";
					}
					if ( (mypw = vauth_getpw( p1, p2 )) != NULL ) {
						struct maildirsize quotainfo;
						char buf[1024];
						long lSize = 0;
						lSize = atol(mypw->pw_shell);
						if( strchr( mypw->pw_shell, 'M' ) ) {
							lSize *= 1048576;
						}
						else if( strchr( mypw->pw_shell, 'K' ) ) {
							lSize *= 1024;	
						}
						sprintf( buf, "%dS", lSize );
						if( maildir_openquotafile_init( &quotainfo, ".", buf) == 0 )
						{
							maildir_checkquota( &quotainfo, 0, 0 );
							maildir_closequotafile(&quotainfo);
						}
					}
					if( p ) 
						free(p);
				}

				// by lfan, support cookie
				{
					char buf[256];
					cgi_setcookie("wmail-session", sqwebmail_sessiontoken );
					cgi_setcookie("wmail-time", libmail_str_time_t(login_time, buf));
					cgi_setcookie("wmail-login", sqwebmail_mailboxid);
				}

				//by lfan
				//http_redirect_argss("&form=folders", "", "");
				http_redirect_argss("&form=main", "", "");
				
				/* output_form("folders.html"); */
				free(ubuf);
				return(0);
			}
			maildir_cache_cancel();

			free(ubuf);
			setgid(getgid());
			setuid(getuid());
			output_form("invalid.html");	/* Invalid login */
			return (0);
		}

		setgid(getgid());
		setuid(getuid());
		if ( *(u=cgi("redirect")))
			/* Redirection request to hide the referral tag */
		{
			redirect(u);
		}
		else if ( *cgi("noframes") == '1')
			output_form("login.html");	/* Main frame */
		else
		if ( *cgi("empty") == '1')
			output_form("empty.html");	/* Minor frameset */

/*
** Apparently we can't show just SCRIPT NAME as our frameset due to some
** weird bug in Communicator which, under certain conditions, will get
** confused figuring out which page views have expired.  POSTs with URLs
** referring to SCRIPT_NAME will be replied with an expiration header, and
** Communicator will assume that index.html also has expired, forcing a
** frameset reload the next time the Communicator window is resized,
** essentially logging the user off.
*/

		else if (*cgi("index") == '1')
			output_form("index.html");	/* Frameset Window */
		else
		{
			http_redirect_top("?index=1");
		}
				
		return (0);
	}
	return (0);
}

#ifdef	malloc

#undef	malloc
#undef	realloc
#undef	free
#undef	strdup
#undef	calloc

static void *allocp[1000];

extern void *malloc(size_t), *realloc(void *, size_t), free(void *),
	*calloc(size_t, size_t);
extern char *strdup(const char *);

char *my_strdup(const char *c)
{
size_t	i;

	for (i=0; i<sizeof(allocp)/sizeof(allocp[0]); i++)
		if (!allocp[i])
			return (allocp[i]=strdup(c));
	abort();
	return (0);
}

void *my_malloc(size_t n)
{
size_t	i;

	for (i=0; i<sizeof(allocp)/sizeof(allocp[0]); i++)
		if (!allocp[i])
			return (allocp[i]=malloc(n));
	abort();
	return (0);
}

void *my_calloc(size_t a, size_t b)
{
size_t	i;

	for (i=0; i<sizeof(allocp)/sizeof(allocp[0]); i++)
		if (!allocp[i])
			return (allocp[i]=calloc(a,b));
	abort();
	return (0);
}

void *my_realloc(void *p, size_t s)
{
size_t	i;

	for (i=0; i<sizeof(allocp)/sizeof(allocp[0]); i++)
		if (p && allocp[i] == p)
		{
		void	*q=realloc(p, s);

			if (q)	allocp[i]=q;
			return (q);
		}
	abort();
}

void my_free(void *p)
{
size_t i;

	for (i=0; i<sizeof(allocp)/sizeof(allocp[0]); i++)
		if (p && allocp[i] == p)
		{
			free(p);
			allocp[i]=0;
			return;
		}
	abort();
}
#endif
