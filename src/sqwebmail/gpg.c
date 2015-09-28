/*
** Copyright 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: gpg.c,v 1.1.1.1 2003/05/07 02:15:27 lfan Exp $
*/
#include	"sqwebmail.h"
#include	"config.h"
#include	"gpg.h"
#include	"pref.h"
#include	"cgi/cgi.h"
#include	"gpglib/gpglib.h"
#include	"unicode/unicode.h"
#include	"numlib/numlib.h"
#include	"rfc822/rfc822.h"
#include	"htmllibdir.h"
#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#if HAVE_SYS_WAIT_H
#include	<sys/wait.h>
#endif
#if HAVE_FCNTL_H
#include	<fcntl.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

extern void output_scriptptrget();
extern void print_attrencodedlen(const char *, size_t, int, FILE *);
extern void print_safe(const char *);
const char *sqwebmail_content_charset;

#if HAVE_SQWEBMAIL_UNICODE

static char gpgerrbuf[1024];
static size_t gpgerrcnt=0;

static void gpginiterr()
{
	gpgerrcnt=0;
}

static int gpg_error(const char *p, size_t l, void *dummy)
{
	while (l && gpgerrcnt < sizeof(gpgerrbuf)-1)
	{
		gpgerrbuf[gpgerrcnt++]= *p++;
		--l;
	}
	return (0);
}

int gpgbadarg(const char *p)
{
	for ( ; *p; p++)
	{
		int c=(unsigned char)*p;

		if (c < ' ' || strchr("\",'`;*?()<>", c))
			return (1);
	}
	return (0);
}

static void dump_error()
{
	if (gpgerrcnt >= 0)
	{
		printf("<FONT COLOR=\"#E00000\"><PRE CLASS=\"gpgerroutput\">");
		print_attrencodedlen (gpgerrbuf, gpgerrcnt, 1, stdout);
		printf("</PRE></FONT>\n");
	}
}

struct listinfo {
	int issecret;
	const char *default_key;
} ;

static int show_key(const char *fingerprint, const char *shortname,
                    const char *key, int invalid,
		    struct gpg_list_info *gli)
{
	struct listinfo *li=(struct listinfo *)gli->voidarg;

	printf("<TR VALIGN=MIDDLE CLASS=\"%s\"><TD>"
	       "<input type=radio name=%s value=\"",
	       li->issecret ? "gpgseckey":"gpgpubkey",
	       li->issecret ? "seckeyname":"pubkeyname");

	print_attrencodedlen(fingerprint, strlen(fingerprint), 0, stdout);
	printf("\"%s></TD><TD><TT>",
	       li->default_key && strcmp(li->default_key, fingerprint) == 0
	       ? " CHECKED":"");
	print_safe(key);
	printf("</TT></TD></TR>\n");
	return (0);
}

static void listpubsec(int flag,
		       int (*callback_func)(const char *,
					    const char *,
					    const char *,
					    int,
					    struct gpg_list_info *),
		       const char *default_key
		       )
{
	int rc;
	struct gpg_list_info gli;
	const struct unicode_info *u=unicode_find(sqwebmail_content_charset);
	struct listinfo li;

	if (!u)
		u= &unicode_ISO8859_1;

	li.issecret=flag;

	li.default_key=default_key;

	memset(&gli, 0, sizeof(gli));
	gli.charset=u;

	gli.disabled_msg=getarg("DISABLED");
	gli.revoked_msg=getarg("REVOKED");
	gli.expired_msg=getarg("EXPIRED");
	gli.voidarg= &li;

	gpginiterr();

	rc=gpg_listkeys(GPGDIR, flag, callback_func, gpg_error, &gli);

	if (rc)
	{
		dump_error();
	}
}

void gpglistpub()
{
	printf("<table width=\"100%%\" border=0 cellspacing=2 cellpadding=0 class=\"gpgpubkeys\">");
	listpubsec(0, show_key, NULL);
	printf("</table>");
}

void gpglistsec()
{
	printf("<table width=\"100%%\" border=0 cellspacing=2 cellpadding=0 class=\"gpgseckeys\">");
	listpubsec(1, show_key, NULL);
	printf("</table>");
}

static int select_key(const char *fingerprint, const char *shortname,
		      const char *key,
		      struct gpg_list_info *gli,
		      int is_select)
{
	printf("<OPTION VALUE=\"");
	print_attrencodedlen(fingerprint, strlen(fingerprint), 0, stdout);
	printf("\"%s>", is_select ? " SELECTED":"");

	print_safe(shortname);
	return (0);
}

static int select_key_default(const char *fingerprint, const char *shortname,
			      const char *key,
			      int invalid,
			      struct gpg_list_info *gli)
{
	struct listinfo *li=(struct listinfo *)gli->voidarg;

	return (select_key(fingerprint, shortname, key, gli,
			   li->default_key && strcmp(li->default_key,
						     fingerprint)
			   == 0));
}

void gpgselectkey()
{
	char *default_key=pref_getdefaultgpgkey();

	listpubsec(1, select_key_default, default_key);

	if (default_key)
		free(default_key);
}

void gpgselectpubkey()
{
	listpubsec(0, select_key_default, NULL);
}

void gpgselectprivkey()
{
	listpubsec(1, select_key_default, NULL);
}

/*
** Check if this encryption key address is included in the list of recipients
** of the message.
*/

static int knownkey(const char *shortname, const char *known_keys)
{
	struct rfc822t *t=rfc822t_alloc_new(shortname, NULL, NULL);
	struct rfc822a *a;
	int i;

	if (!t)
		return (0);

	a=rfc822a_alloc(t);

	if (!a)
	{
		rfc822t_free(t);
		return (0);
	}

	for (i=0; i<a->naddrs; i++)
	{
		char *p=rfc822_getaddr(a, i);
		int plen;
		const char *q;

		if (!p)
			continue;

		if (!*p)
		{
			free(p);
			continue;
		}

		plen=strlen(p);

		for (q=known_keys; *q; )
		{
			if (strncasecmp(q, p, plen) == 0 && q[plen] == '\n')
			{
				free(p);
				rfc822a_free(a);
				rfc822t_free(t);
				return (1);
			}

			while (*q)
				if (*q++ == '\n')
					break;
		}
		free(p);
	}
	rfc822a_free(a);
	rfc822t_free(t);
	return (0);
}

static int encrypt_key_default(const char *fingerprint, const char *shortname,
			      const char *key,
			      int invalid,
			      struct gpg_list_info *gli)
{
	struct listinfo *li=(struct listinfo *)gli->voidarg;

	if (invalid)
		return (0);

	return (select_key(fingerprint, shortname, key, gli,
			   knownkey(shortname, li->default_key)));
}

void gpgencryptkeys(const char *select_keys)
{
	listpubsec(0, encrypt_key_default, select_keys);
}


/*
** Create a new key
*/

static int dump_func(const char *p, size_t l, void *vp)
{
	int *ip=(int *)vp;

	while (l)
	{
		if (*ip >= 80)
		{
			printf("\n");
			*ip=0;
		}

		++*ip;

		switch (*p) {
		case '<':
			printf("&lt;");
			break;
		case '>':
			printf("&gt;");
			break;
		case '\n':
			*ip=0;
			/* FALLTHROUGH */
		default:
			putchar(*p);
			break;
		}

		++p;
		--l;
	}
	fflush(stdout);
	return (0);
}

static int timeout_func(void *vp)
{
	return (dump_func("*", 1, vp));
}

void gpgcreate()
{
	int linelen;

	const struct unicode_info *u=unicode_find(sqwebmail_content_charset);
	const char *newname=cgi("newname");
	const char *newaddress=cgi("newaddress");
	const char *newcomment=cgi("newcomment");
	unsigned skl=atoi(cgi("skeylength"));
	unsigned ekl=atoi(cgi("ekeylength"));
	unsigned newexpire=atoi(cgi("newexpire"));
	char newexpirewhen=*cgi("newexpirewhen");
	const char *passphrase, *p;

	if (!u)
		u= &unicode_ISO8859_1;

	if (*newname == 0 || *newaddress == 0 || strchr(newaddress, '@') == 0
	    || gpgbadarg(newname) || gpgbadarg(newaddress) || gpgbadarg(newcomment)
	    || ekl < 512 || ekl > 2048 || skl < 512 || skl > 1024)
	{
		printf("%s\n", getarg("BADARGS"));
		return;
	}
	passphrase=cgi("passphrase");
	if (strcmp(passphrase, cgi("passphrase2")))
	{
		printf("%s\n", getarg("PASSPHRASEFAIL"));
		return;
	}

	for (p=passphrase; *p; p++)
	{
		if ((int)(unsigned char)*p < ' ')
		{
			printf("%s\n", getarg("PASSPHRASEFAIL"));
			return;
		}
	}

	printf("<PRE CLASS=\"gpgcreate\">");

	linelen=0;

	gpg_genkey(GPGDIR, u, newname, newaddress, newcomment, skl, ekl,
		   newexpire, newexpirewhen,
		   passphrase,
		   &dump_func,
		   &timeout_func,
		   &linelen);
	printf("</PRE>");
}

static void delkey(const char *keyname, int flag)
{
	int rc;

	if (gpgbadarg(keyname))
		return;

	gpginiterr();

	rc=gpg_deletekey(GPGDIR, flag, keyname, gpg_error, NULL);

	if (rc)
	{
		printf("<blockquote>%s\n", getarg("DELETEFAIL"));
		dump_error();
		printf("</blockquote>\n");
	}
}

static FILE *passphrasefp()
{
	FILE *fp=NULL;
	const char *passphrase;

	passphrase=cgi("passphrase");
	if (*passphrase)
	{
		fp=tmpfile();
		if (fp)
		{
			fprintf(fp, "%s", passphrase);
			if (fflush(fp) || ferror(fp)
			    || lseek(fileno(fp), 0L, SEEK_SET) < 0
			    || fcntl(fileno(fp), F_SETFD, 0) < 0)
			{
				fclose(fp);
				fp=NULL;
			}
		}
	}
	return (fp);
}

static void signkey(const char *signthis, const char *signwith,
		    const char *trustlevel)
{
	int rc;
	FILE *fp=NULL;

	int n=atoi(trustlevel);

	if (n < 0 || n > 9)
		n=0;

	if (gpgbadarg(signthis) || gpgbadarg(signwith))
		return;

	gpginiterr();


	fp=passphrasefp();

	rc=gpg_signkey(GPGDIR, signthis, signwith,
		       fp ? fileno(fp):-1, gpg_error, n, NULL);

	if (fp)
		fclose(fp);

	if (rc)
	{
		printf("<blockquote>%s\n", getarg("SIGNFAIL"));
		dump_error();
		printf("</blockquote>\n");
	}
}

static void setdefault(const char *def)
{
	if (gpgbadarg(def))
		return;

	pref_setdefaultgpgkey(def);
}

void gpgdo()
{
	if (*cgi("delpub"))
		delkey(cgi("pubkeyname"), 0);
	else if (*cgi("delsec") && *cgi("really"))
		delkey(cgi("seckeyname"), 1);
	else if (*cgi("sign"))
		signkey(cgi("pubkeyname"), cgi("seckeyname"),
			cgi("signlevel"));
	else if (*cgi("setdefault"))
		setdefault(cgi("seckeyname"));
}

static char gpgerrbuf[1024];

static int wait_mimegpg(pid_t pid, int errfd)
{
	pid_t pid2;
	int waitstat;
	char *p;
	int i, n;

	p=gpgerrbuf;
	n=sizeof(gpgerrbuf)-1;

	while (n && (i=read(errfd, p, n)) > 0)
	{
		p += i;
		n -= i;
	}
	close(errfd);
	*p=0;

	while ((pid2=wait(&waitstat)) != pid)
	{
		if (pid2 < 0 && errno == ECHILD)
			return (-1);
	}

	return (WIFEXITED(waitstat) && WEXITSTATUS(waitstat) == 0 ? 0:-1);
}

int gpgdomsg(int in_fd, int out_fd, const char *signkey,
	     const char *encryptkeys)
{
	char *k=strdup(encryptkeys ? encryptkeys:"");
	int n;
	int i;
	char *p;
	char **argvec;
	pid_t pid;
	int stderrfd[2];
	FILE *passfd=NULL;
	const char *cp;
	char passfd_buf[NUMBUFSIZE];

	if (!k)
		enomem();

	if (pipe(stderrfd))
	{
		free(k);
		enomem();
	}

	if (*(cp=cgi("passphrase")) != 0)
	{
		passfd=tmpfile();
		if (passfd)
		{
			fprintf(passfd, "%s", cp);
			if (fflush(passfd) || ferror(passfd)
			    || lseek(fileno(passfd), 0L, SEEK_SET) < 0
			    || fcntl(fileno(passfd), F_SETFD, 0) < 0)
			{
				fclose(passfd);
				passfd=NULL;
			}
		}
	}

	n=0;
	for (p=k; (p=strtok(p, " ")) != NULL; p=NULL)
		++n;

	argvec=malloc((n * 2 + 22)*sizeof(char *));
	if (!argvec)
	{
		close(stderrfd[0]);
		close(stderrfd[1]);
		free(k);
		enomem();
	}

	i=0;
	argvec[i++]=MIMEGPG;
	if (signkey)
		argvec[i++]="-s";
	if (n > 0)
		argvec[i++]="-E";
	if (passfd)
	{
		argvec[i++]="-p";
		argvec[i++]=libmail_str_size_t(fileno(passfd), passfd_buf);
	}

	argvec[i++] = "--";
	argvec[i++] = "--no-tty";
	if (signkey)
	{
		argvec[i++]="--default-key";
		argvec[i++]=(char *)signkey;
	}

	argvec[i++]="--always-trust";

	for (p=strcpy(k, encryptkeys ? encryptkeys:"");
	     (p=strtok(p, " ")) != NULL; p=NULL)
	{
		argvec[i++]="-r";
		argvec[i++]=p;
	}
	argvec[i]=0;

	pid=fork();
	if (pid < 0)
	{
		free(k);
		close(stderrfd[0]);
		close(stderrfd[1]);
		return (-1);
	}

	if (pid == 0)
	{
		close(0);
		dup(in_fd);
		close(1);
		dup(out_fd);
		close(in_fd);
		close(out_fd);

		close(2);
		dup(stderrfd[1]);
		close(stderrfd[1]);
		close(stderrfd[0]);

		putenv("GNUPGHOME=" GPGDIR);
		execv(argvec[0], argvec);
		perror("mimegpg");
		exit(1);
	}
	free(argvec);
	free(k);
	close(stderrfd[1]);
	if (passfd)
		fclose(passfd);

	return ( wait_mimegpg(pid, stderrfd[0]));
}

void sent_gpgerrtxt()
{
	const char *p;

	for (p=gpgerrbuf; *p; p++)
	{
		switch (*p) {
		case '<':
			printf("&lt;");
			break;
		case '>':
			printf("&gt;");
			break;
		default:
			putchar((int)(unsigned char)*p);
			break;
		}
	}
}

void sent_gpgerrresume()
{
	output_scriptptrget();
	printf("&form=newmsg&pos=%s&draft=%s", cgi("pos"),
	       cgi("draftmessage"));
}

int gpgdecode(int in_fd, int out_fd)
{
	pid_t pid;
	int stderrfd[2];
	int rc;

	if (pipe(stderrfd))
	{
		enomem();
	}

	pid=fork();
	if (pid < 0)
	{
		close(stderrfd[0]);
		close(stderrfd[1]);
		enomem();
	}

	if (pid == 0)
	{
		FILE *fp=passphrasefp();
		char *argvec[20];
		int i;
		char passfd_buf[NUMBUFSIZE];

		close(0);
		dup(in_fd);
		close(1);
		dup(out_fd);
		close(in_fd);
		close(out_fd);

		close(2);
		dup(stderrfd[1]);
		close(stderrfd[1]);
		close(stderrfd[0]);

		argvec[0]=MIMEGPG;
		argvec[1]="-c";
		argvec[2]="-d";

		i=3;

		if (fp)
		{
			argvec[i++]="-p";
			argvec[i++]=libmail_str_size_t(fileno(fp), passfd_buf);
		}

		argvec[i++]="--";
		argvec[i++]="--no-tty";
		argvec[i]=0;

		putenv("GNUPGHOME=" GPGDIR);
		execv(MIMEGPG, argvec);
		perror(MIMEGPG);
		exit(1);
	}
	close (stderrfd[1]);
	rc=wait_mimegpg(pid, stderrfd[0]);

	if (rc)
	{
		printf("<blockquote><pre style=\"color: red;\">");
		sent_gpgerrtxt();
		printf("</pre></blockquote>\n");
	}
	return (rc);
}

int gpgexportkey(const char *fingerprint, int issecret,
		 int (*func)(const char *, size_t, void *),
		 void *arg)
{
	gpginiterr();

	return (gpg_exportkey(GPGDIR, issecret, fingerprint,
			      func,
			      gpg_error,
			      arg));
}

#else
void gpglistpub()
{
}

void gpglistsec()
{
}

void gpgcreate()
{
}

void gpgdo()
{
}

void sent_gpgerrtxt()
{
}

void sent_gpgerrresume()
{
}

void gpgselectpubkey()
{
}

void gpgselectprivkey()
{
}

int gpgexportkey(const char *fingerprint, int issecret,
		 int (*func)(const char *, size_t, void *),
		 void *arg)
{
	return (1);
}

#endif
