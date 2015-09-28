/*
** Copyright 1998 - 2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/


/*
** $Id: pref.c,v 1.5 2004/01/08 05:50:38 roy Exp $
*/
#include	"pref.h"
#include	"config.h"
#include	"auth.h"
#include	"sqwebmail.h"
#include	"sqconfig.h"
#include	"mailinglist.h"
#include	"cgi/cgi.h"
#include	"authlib/authstaticlist.h"
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>

//by lfan, for vpopmail
#include        "vpopmail.h"
#include        "vauth.h"

#define	OLDEST1ST	"OLDEST1ST"
#define	FULLHEADERS	"FULLHEADERS"
#define	SORTORDER	"SORT"
#define	PAGESIZE	"PAGESIZE"
#define	AUTOPURGE_V	"AUTOPURGE"
#define	NOHTML		"NOHTML"
#define	FROM		"FROM"
#define	LDAP		"LDAP"
#define FLOWEDTEXT	"NOFLOWEDTEXT"
#define NOARCHIVE	"NOARCHIVE"
#define STARTOFWEEK	"STARTOFWEEK"

#define	OLDEST1ST_PREF	"oldest1st"
#define	FULLHEADERS_PREF "fullheaders"
#define	HTML_PREF "doshowhtml"
#define FLOWEDTEXT_PREF "noflowedtext"
#define NOARCHIVE_PREF "noarchive"

#define DEFAULTKEY	"DEFAULTKEY"

int pref_flagisoldest1st, pref_flagfullheaders;
int pref_showhtml;
int pref_flagsortorder;
int pref_flagpagesize;
int pref_autopurge;
int pref_noflowedtext;
int pref_noarchive;
int pref_startofweek;

char *pref_from=0;
char *pref_ldap=0;

#if ENABLE_WEBPASS
extern int check_sqwebpass(const char *);
extern void set_sqwebpass(const char *);
#endif
extern void output_attrencoded_oknl(const char *);
extern const char *sqwebmail_mailboxid;

static const char hex[]="0123456789ABCDEF";

// by lfan
void get_dotqmail_file( char* fpath )
{
	struct vqpasswd *mypw;
	char *user, *domain, *p, *p2;
	p=login_returnaddr();
	p2=strdup(p);
	user=strtok(p2, "@");
	domain=strtok(0, "@");
	
	if ( (mypw = vauth_getpw( user, domain )) != NULL ) {
		sprintf( fpath, "%s/.qmail", mypw->pw_dir );
		free(mypw);
	}
	if( p2 )
		free(p2);
}

//by roy
void get_antispam_file( char* fpath )
{
	strcpy(fpath,".antispam");
}
static int nybble(char c)
{
char	*p=strchr(hex, c);

	if (p)	return (p-hex);
	return (0);
}

static void decode(char *t)
{
char *s;

	for (s=t; *s; s++)
	{
		if (*s != '+')
		{
			*t++ = *s;
			continue;
		}
		if (s[1] == 0 || s[2] == 0)
			continue;
		*t++ = nybble(s[1]) * 16 + nybble(s[2]);
		s += 2;
	}
	*t=0;
}

void pref_init()
{
const	char *p;
char	*q, *r;

	p=read_sqconfig(".", CONFIGFILE, 0);
	pref_flagisoldest1st=0;
	pref_flagfullheaders=0;
	pref_flagsortorder=0;
	pref_flagpagesize=10;
	pref_startofweek=0;
	pref_autopurge=AUTOPURGE;
	pref_showhtml=1;

	if(pref_from) {
		free(pref_from);
		pref_from=0;
		}

	if(pref_ldap) {
		free(pref_ldap);
		pref_ldap=0;
		}


	if (p)
	{
		q=strdup(p);
		if (!q)	enomem();

		for (r=q; (r=strtok(r, " ")) != 0; r=0)
		{
			if (strcmp(r, OLDEST1ST) == 0)
				pref_flagisoldest1st=1;
			if (strcmp(r, FULLHEADERS) == 0)
				pref_flagfullheaders=1;
			if (strcmp(r, NOHTML) == 0)
				pref_showhtml=0;
			if (strncmp(r, SORTORDER, sizeof(SORTORDER)-1) == 0
				&& r[sizeof(SORTORDER)-1] == '=')
				pref_flagsortorder=r[sizeof(SORTORDER)];
			if (strncmp(r, PAGESIZE, sizeof(PAGESIZE)-1) == 0
				&& r[sizeof(PAGESIZE)-1] == '=')
				pref_flagpagesize=atoi(r+sizeof(PAGESIZE));
			if (strncmp(r, AUTOPURGE_V, sizeof(AUTOPURGE_V)-1) == 0
				&& r[sizeof(AUTOPURGE_V)-1] == '=')
				pref_autopurge=atoi(r+sizeof(AUTOPURGE_V));
			if (strncmp(r, FLOWEDTEXT, sizeof(FLOWEDTEXT)-1) == 0
				&& r[sizeof(FLOWEDTEXT)-1] == '=')
				pref_noflowedtext=atoi(r+sizeof(FLOWEDTEXT));
			if (strncmp(r, NOARCHIVE, sizeof(NOARCHIVE)-1) == 0
				&& r[sizeof(NOARCHIVE)-1] == '=')
				pref_noarchive=atoi(r+sizeof(NOARCHIVE));
			if (strncmp(r, FROM, sizeof(FROM)-1) == 0
				&& r[sizeof(FROM)-1] == '=')
			{
				if (pref_from)	free(pref_from);
				if ((pref_from=strdup(r+sizeof(FROM))) == 0)
					enomem();

				decode(pref_from);
			}
			if (strncmp(r, LDAP, sizeof(LDAP)-1) == 0
				&& r[sizeof(LDAP)-1] == '=')
			{
				if (pref_ldap)	free(pref_ldap);
				if ((pref_ldap=strdup(r+sizeof(LDAP))) == 0)
					enomem();

				decode(pref_ldap);
			}
			if (strncmp(r, STARTOFWEEK, sizeof(STARTOFWEEK)-1) == 0
				&& r[sizeof(STARTOFWEEK)-1] == '=')
			{
				int n=atoi(r+sizeof(STARTOFWEEK));

				if (n >= 0 && n < 7)
					pref_startofweek=n;
			}

		}
		free(q);
	}
	switch (pref_flagpagesize)	{
	case 20:
	case 50:
	case 100:
	case 250:
		break;
	default:
		pref_flagpagesize=10;
		break;
	}

	if (pref_autopurge < 0)	pref_autopurge=0;
	if (pref_autopurge > MAXPURGE)	pref_autopurge=MAXPURGE;

	switch (pref_flagsortorder)	{
	case 'F':
	case 'S':
		break;
	default:
		pref_flagsortorder='D';
		break;
	}
}

#if 0
#if ENABLE_WEBPASS
static int goodpass(const char *p)
{
	for ( ; *p; p++)
		if (*p < ' ')	return (0);
	return (1);
}
#endif
#endif

static char *append_str(const char *prefs, const char *label,
	const char *value)
{
int	l=strlen(prefs) + sizeof(" =") +
	strlen(label)+ (value ? strlen(value):0);
int	i;
char	*p;
const char *q;

	for (i=0; value && value[i]; i++)
		if (value[i] <= ' ' || value[i] >= 127
			|| value[i] == '+')
			l += 2;

	p=malloc(l);
	if (!p)	enomem();
	strcpy(p, prefs);
	if (!value || !*value)	return (p);

	strcat(strcat(strcat(p, " "), label), "=");
	i=strlen(p);
	for (q=value; *q; q++)
	{
		if (*q <= ' ' || *q >= 127 || *q == '+')
		{
			sprintf(p+i, "+%02X", (int)(unsigned char)*q);
			i += 3;
			continue;
		}
		p[i++]= *q;
	}
	p[i]=0;
	return (p);
}

void pref_update()
{
char	buf[1000];
char	*p;
char	*q;

	sprintf(buf, SORTORDER "=%c " PAGESIZE "=%d " AUTOPURGE_V "=%d "
		FLOWEDTEXT "=%d " NOARCHIVE "=%d " STARTOFWEEK "=%d",
		pref_flagsortorder, pref_flagpagesize, pref_autopurge,
		pref_noflowedtext,
		pref_noarchive,
		pref_startofweek);

	if (pref_flagisoldest1st)
		strcat(buf, " " OLDEST1ST);

	if (pref_flagfullheaders)
		strcat(buf, " " FULLHEADERS);

	if (!pref_showhtml)
		strcat(buf, " " NOHTML);

	p=append_str(buf, FROM, pref_from);
	q=append_str(p, LDAP, pref_ldap);
	write_sqconfig(".", CONFIGFILE, q);
	free(q);
	free(p);
}

void pref_setfrom(const char *p)
{
	if (pref_from)	free(pref_from);
	// by lfan
	// pref_from=strdup(p);
	pref_from = malloc(256);
	if (!pref_from)	enomem();
	sprintf(pref_from, "\"%s\" <%s>", p, login_returnaddr());
	pref_update();
}

void pref_getfrom()
{
	char p[256];
	char *p1 = 0;
	char *p2 = 0;
	bzero(p, 256);
	if(pref_from) p1=strchr(pref_from, '\"');
	if(p1) p2=strchr(p1+1, '\"');
	if (pref_from && p1 && p2) {
		strncat(p, p1+1, p2 - p1 -1);
	}
	printf("<INPUT TYPE=TEXT CLASS=myinput name=\"preffrom\" value=\"%s\">", p);
}

void pref_setldap(const char *p)
{
	if (pref_ldap && strcmp(p, pref_ldap) == 0)
		return;

	if (pref_ldap)	free(pref_ldap);
	pref_ldap=strdup(p);
	if (!pref_ldap)	enomem();
	pref_update();
}

void pref_setprefs()
{
	if (*cgi("do.changeprefs"))
	{
	char	buf[1000];
	FILE	*fp;
	char	*p;
	char	*q;

		sprintf(buf, SORTORDER "=%c " PAGESIZE "=%s "
			AUTOPURGE_V "=%s " FLOWEDTEXT "=%s "
			NOARCHIVE "=%s " STARTOFWEEK "=%d",
			*cgi("sortorder"), cgi("pagesize"),
			cgi("autopurge"),
			*cgi(FLOWEDTEXT_PREF) ? "1":"0",
			*cgi(NOARCHIVE_PREF) ? "1":"0",

			// by lfan, cancel calendar
			//(int)((unsigned)atoi(cgi(STARTOFWEEK)) % 7)
			0
			);

		if (*cgi(OLDEST1ST_PREF))
			strcat(buf, " " OLDEST1ST);
		if (*cgi(FULLHEADERS_PREF))
			strcat(buf, " " FULLHEADERS);
		if (!*cgi(HTML_PREF))
			strcat(buf, " " NOHTML);
		if (*cgi("preffrom"))
			pref_setfrom(cgi("preffrom"));

		p=append_str(buf, FROM, pref_from);
		q=append_str(p, LDAP, pref_ldap);
		write_sqconfig(".", CONFIGFILE, q);
		free(p);
		free(q);
		pref_init();

		/* by lfan, in other setting
		if ((fp=fopen(SIGNATURE, "w")) != NULL)
		{
			fprintf(fp, "%s", cgi("signature"));
			fclose(fp);
		}

		savemailinglists(cgi("mailinglists"));
		*/
	}
	// by lfan, support signature
	else if (*cgi("do.changesign"))
	{
	FILE *fp;
		if ((fp=fopen(SIGNATURE, "w")) != NULL)
		{
			fprintf(fp, "%s", cgi("signature"));
			fclose(fp);
		}
	}
	//by roy, support spam
	else if (*cgi("do.changespam"))
	{
		FILE *fp;
		char fpath[256];
		get_antispam_file(fpath);
		if ((fp=fopen(fpath, "w")) != NULL)
		{
			if (!(*cgi("spam"))) {
				fprintf(fp,"0");
			} else if (!strcmp(cgi("spam"),"1") )
				fprintf(fp, "1");
			else  if (!strcmp(cgi("spam"),"2") )
				fprintf(fp,"2");
			else
				fprintf(fp,"0");
			fclose(fp);
		}
	}
	// by lfan, support forward
	else if (*cgi("do.changefwd"))
        {
        FILE *fp1, *fp2;
        char buf[512], fpath[256], folder[256], *p;

                int lastc;
		get_dotqmail_file(fpath);
		p = cgi("forwardlists");

        	sprintf(folder, "%s.tmp", fpath);

        	if ((fp2=fopen(folder, "w")) == NULL)
        	        return;


		lastc = 1;
		
		if ((fp1=fopen(fpath, "r")) != NULL)
		{
		    while (fgets(buf, sizeof(buf), fp1))
		    {
			if( buf[0] == '&' ) continue;
			if( buf[0] == '#' )
			{
				if( *cgi("save") || strlen(p) < 4 ) 
					fprintf(fp2, "%s", buf+1);
				else		
					fprintf(fp2, "%s", buf);
			}
			else {
				if( *cgi("save") || strlen(p) < 4)
					fprintf(fp2, "%s", buf);
				else
					fprintf(fp2, "#%s", buf);
			}
			lastc = 0;
		    }
		    fclose(fp1);
		}

		if( lastc )
		{
			if( strlen(p) < 4 ) {
				unlink( fpath );
				return;
			}
			// by lfan, if first created, add deliver
			if( *cgi("save") )
				fprintf(fp2, "./Maildir/\n" );
		}

        	for (lastc='\n'; *p; p++)
        	{
        	    if (isspace((int)(unsigned char)*p) && *p != '\n')
        	        continue;
        	    if (*p == '\n' && lastc == '\n')
        	        continue;
        	    if (lastc == '\n')
        	    {
        	        putc('&', fp2);
        	    }
        	    putc(*p, fp2);
        	    lastc = *p;
        	}
		if( lastc != '\n' )
			putc('\n', fp2);
        	fclose(fp2);
        	rename(folder, fpath);
        }

	if (*cgi("do.changepwd") && auth_changepass)
	{
		int status=1;
		const	char *p=cgi("newpass");
		int has_syspwd=0;

		if ( *p && strcmp(p, cgi("newpass2")) == 0)
		{
			has_syspwd=
				login_changepwd(sqwebmail_mailboxid,
						cgi("oldpass"), p,
						&status);
		}

		if (has_syspwd || status)
		{
			printf("%s\n", getarg("PWDERR"));
		}
		else
			printf("%s\n", getarg("PWDCHGOK"));
	}
}

// by lfan, support forward
void pref_forward()
{
FILE *fp;
char buf[256], fpath[80], *p, flag;

        printf("<textarea cols=40 rows=4 name=\"forwardlists\" wrap=\"off\">");

	get_dotqmail_file(fpath);

	flag = 0;
        if ((fp=fopen(fpath, "r")) != NULL)
        {
                p = buf;
                p += 1;
                while (fgets(buf, sizeof(buf), fp))
                {
                    if( buf[0] != '&' ) 
		    { 
			    if( buf[0] != '#' )
			    	flag = 1; 
			    continue; 
		    }
                    output_attrencoded_oknl(p);
                }
                fclose(fp);
        }
		
        printf("</textarea><br>\n");
	printf("<input type=checkbox name=\"save\" value=\"yes\" %s>", flag? "checked":"" );
}


void pref_spam()
{
FILE *fp;
char buf[256], fpath[80], *p;
int flag;


	get_antispam_file(fpath);
	flag = 0;
        if ((fp=fopen(fpath, "r")) != NULL)
        {
		fscanf(fp,"%d",&flag);		
                fclose(fp);
	}
	printf("<input type=\"radio\" name=\"spam\" value=\"0\" %s>保存可疑邮件到收件箱中<br>", (flag==0)? "checked":"" );
	printf("<input type=\"radio\" name=\"spam\" value=\"1\" %s>保存可疑邮件到垃圾邮件箱中<br>", (flag==1)? "checked":"" );
	printf("<input type=\"radio\" name=\"spam\" value=\"2\" %s>直接丢弃可疑邮件<br>", (flag==2)? "checked":"" );
}


void pref_isoldest1st()
{
	printf("<INPUT TYPE=CHECKBOX NAME=\"%s\"%s>",
		OLDEST1ST_PREF, pref_flagisoldest1st ? " CHECKED":"");
}

void pref_isdisplayfullmsg()
{
	printf("<INPUT TYPE=CHECKBOX NAME=\"%s\"%s>",
		FULLHEADERS_PREF, pref_flagfullheaders ? " CHECKED":"");
}

void pref_displayhtml()
{
	printf("<INPUT TYPE=CHECKBOX NAME=\"%s\"%s>",
		HTML_PREF, pref_showhtml ? " CHECKED":"");
}

void pref_displayflowedtext()
{
	printf("<INPUT TYPE=CHECKBOX NAME=\"%s\"%s>",
		FLOWEDTEXT_PREF, pref_noflowedtext ? " CHECKED":"");
}

void pref_displayweekstart()
{
	int i, j;
	static const int d[3]={6,0,1};

	printf("<SELECT NAME=\"" STARTOFWEEK "\">");

	for (j=0; j<3; j++)
	{
		i=d[j];
		printf("<option value=\"%d\"%s>", i,
		       i == pref_startofweek ? " SELECTED":"");

		//output_attrencoded_oknl( pcp_wdayname_long(i));
	}
	printf("</SELECT>");
}

void pref_displaynoarchive()
{
	printf("<INPUT TYPE=CHECKBOX NAME=\"%s\"%s>",
		NOARCHIVE_PREF, pref_noarchive ? " CHECKED":"");
}

void pref_displayautopurge()
{
	printf("<INPUT CLASS=myinput TYPE=TEXT NAME=\"autopurge\" VALUE=\"%d\" SIZE=2 MAXLENGTH=2>",
		pref_autopurge);
}

void pref_sortorder()
{
static const char selected[]=" SELECTED";

	printf("<SELECT NAME=sortorder>");
	printf("<OPTION VALUE=DATE%s>%s\n",
		pref_flagsortorder == 'D' ? selected:"",
	       getarg("DATE"));

	printf("<OPTION VALUE=FROM%s>%s\n",
		pref_flagsortorder == 'F' ? selected:"",
	       getarg("SENDER"));
	printf("<OPTION VALUE=SUBJECT%s>%s\n",
		pref_flagsortorder == 'S' ? selected:"",
	       getarg("SUBJECT"));
	printf("</SELECT>\n");
}

void pref_pagesize()
{
static const char selected[]=" SELECTED";

	printf("<SELECT NAME=pagesize>");
	printf("<OPTION VALUE=10%s>10\n",
		pref_flagpagesize == 10 ? selected:"");
	printf("<OPTION VALUE=20%s>20\n",
		pref_flagpagesize == 20 ? selected:"");
	printf("<OPTION VALUE=50%s>50\n",
		pref_flagpagesize == 50 ? selected:"");
	printf("<OPTION VALUE=100%s>100\n",
		pref_flagpagesize == 100 ? selected:"");
	printf("<OPTION VALUE=250%s>250\n",
		pref_flagpagesize == 250 ? selected:"");
	printf("</SELECT>\n");
}

void pref_signature()
{
FILE	*fp;
char	buf[256];
int	n;

	printf("<textarea cols=40 rows=4 name=\"signature\" wrap=\"off\">");

	if ((fp=fopen(SIGNATURE, "r")) != NULL)
	{
		while ((n=fread(buf, 1, sizeof(buf)-1, fp)) > 0)
		{
			buf[n]=0;
			output_attrencoded_oknl(buf);
		}
		fclose(fp);
	}
	printf("</textarea>");
}

/*
** Get a setting from GPGCONFIGFILE
**
** GPGCONFIGFILE consists of space-separated settings.
*/

static char *getgpgconfig(const char *name)
{
	const char *p;
	char	*q, *r;

	int name_l=strlen(name);

	p=read_sqconfig(".", GPGCONFIGFILE, 0);

	if (p)
	{
		q=strdup(p);
		if (!q)
			enomem();

		for (r=q; (r=strtok(r, " ")) != NULL; r=NULL)
			if (strncmp(r, name, name_l) == 0 &&
			    r[name_l] == '=')
			{
				r=strdup(r+name_l+1);
				free(q);
				if (!r)
					enomem();
				return (r);
			}
		free(q);
	}
	return (NULL);
}

/*
** Enter a setting into GPGCONFIGFILE
*/

static void setgpgconfig(const char *name, const char *value)
{
	const	char *p;
	char *q, *r, *s;
	int name_l=strlen(name);

	/* Get the existing settings */

	p=read_sqconfig(".", GPGCONFIGFILE, 0);

	if (!p)
		p="";

	q=strdup(p);
	if (!q)
		enomem();

	s=malloc(strlen(q)+strlen(name)+strlen(value)+4);
	if (!s)
		enomem();
	*s=0;

	/*
	** Copy existing settings into a new buffer, deleting any old
	** setting.
	*/

	for (r=q; (r=strtok(r, " ")) != NULL; r=NULL)
	{
		if (strncmp(r, name, name_l) == 0 &&
		    r[name_l] == '=')
		{
			continue;
		}

		if (*s)
			strcat(s, " ");
		strcat(s, r);
	}

	/* Append the new setting */

	if (*s)
		strcat(s, " ");
	strcat(strcat(strcat(s, name), "="), value);
	free(q);
	write_sqconfig(".", GPGCONFIGFILE, s);
	free(s);
}

char *pref_getdefaultgpgkey()
{
	return (getgpgconfig(DEFAULTKEY));
}

void pref_setdefaultgpgkey(const char *v)
{
	setgpgconfig(DEFAULTKEY, v);
}
