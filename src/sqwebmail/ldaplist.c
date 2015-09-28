/*
** Copyright 2000-2003 Double Precision, Inc.  See COPYING for
** distribution information.
*/

/*
** $Id: ldaplist.c,v 1.1.1.1 2003/05/07 02:15:27 lfan Exp $
*/
#include	"sqwebmail.h"
#include	<stdio.h>
#include	<errno.h>
#include	<stdlib.h>
#include	<ctype.h>
#if	HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<string.h>
#include	"cgi/cgi.h"
#include	"ldapaddressbook/ldapaddressbook.h"
#include	"maildir/maildircreate.h"
#include	"numlib/numlib.h"
#include	"htmllibdir.h"
#include	"addressbook.h"
#include	"pref.h"
#include	"rfc2045/base64.h"

#define	LOCALABOOK	"sqwebmail-ldapaddressbook"

extern void output_scriptptrget();
extern void output_attrencoded(const char *);
extern void output_attrencoded_oknl(const char *p);
extern void output_urlencoded(const char *);
extern void output_attrencoded_fp(const char *, FILE *);
extern void output_attrencoded_oknl_fp(const char *, FILE *);

extern const char *sqwebmail_content_charset;

#if HAVE_SQWEBMAIL_UNICODE
#include	"unicode/unicode.h"
#endif

void	ldaplist()
{
struct ldapabook *abooks[2];
int	i;
struct ldapabook *p;
const char	*delabook=getarg("DELABOOK");
const char	*sysbook=getarg("SYSBOOK");

	if (!delabook)	delabook="";
	if (!sysbook)	sysbook="";

	if (*cgi("addabook"))
	{
		struct ldapabook newbook;
		struct ldapabook_opts zopt, xopt, uopt, oopt, yopt;
		char *p;

		memset(&newbook, 0, sizeof(newbook));

		newbook.name=(char *)cgi("name");
		newbook.host=(char *)cgi("host");
		newbook.port=(char *)cgi("port");
		newbook.suffix=(char *)cgi("suffix");
		newbook.binddn=(char *)cgi("binddn");
		newbook.bindpw=(char *)cgi("bindpw");

		p= (char *)cgi("zopt");

		if (strcmp(p, "Z") == 0 || strcmp(p, "ZZ") == 0)
		{
			zopt.options=p;
			zopt.next=newbook.opts;
			newbook.opts= &zopt;
		}

		xopt.options=NULL;
		uopt.options=NULL;
		oopt.options=NULL;
		yopt.options=NULL;
		p= (char *)cgi("xoptu");
		if (*p)
		{
			xopt.options=malloc(strlen(p)+6);
			if (!xopt.options)
				enomem();
			xopt.options[0]=SASL_AUTHENTICATION_RID;
			strcpy(xopt.options+1, "u:");
			strcat(xopt.options, p);
			xopt.next=newbook.opts;
			newbook.opts= &xopt;
		}
		else if (*(p=(char *)cgi("xoptdn")))
		{
			xopt.options=malloc(strlen(p)+6);
			if (!xopt.options)
				enomem();
			xopt.options[0]=SASL_AUTHENTICATION_RID;
			strcpy(xopt.options+1, "dn:");
			strcat(xopt.options, p);
			xopt.next=newbook.opts;
			newbook.opts= &xopt;
		}

		p= (char *)cgi("uopt");
		if (*p)
		{
			uopt.options=malloc(strlen(p)+2);
			if (!uopt.options)
				enomem();
			uopt.options[0]=SASL_AUTHENTICATION_ID;
			strcpy(uopt.options+1, p);
			uopt.next=newbook.opts;
			newbook.opts= &uopt;
		}

		p= (char *)cgi("yopt");
		if (*p)
		{
			yopt.options=malloc(strlen(p)+2);
			if (!yopt.options)
				enomem();
			yopt.options[0]=SASL_AUTHENTICATION_MECHANISM;
			strcpy(yopt.options+1, p);
			yopt.next=newbook.opts;
			newbook.opts= &yopt;
		}

		p= (char *)cgi("oopt");
		if (*p)
		{
			oopt.options=malloc(strlen(p)+2);
			if (!oopt.options)
				enomem();
			oopt.options[0]=SASL_SECURITY_PROPERTIES;
			strcpy(oopt.options+1, p);
			oopt.next=newbook.opts;
			newbook.opts= &oopt;
		}

		if (*newbook.name && *newbook.host &&
			ldapabook_add(LOCALABOOK, &newbook) < 0)
		{
			printf("<pre>\n");
			perror("ldapabook_add");
			printf("</pre>\n");
		}
		if (xopt.options)
			free(xopt.options);
		if (yopt.options)
			free(yopt.options);
		if (oopt.options)
			free(oopt.options);
		if (uopt.options)
			free(uopt.options);
	}

	if (*cgi("delabook"))
	{
		struct maildir_tmpcreate_info createInfo;
		int fd;
		maildir_tmpcreate_init(&createInfo);

		createInfo.uniq="abook";

		if ((fd=maildir_tmpcreate_fd(&createInfo)) >= 0)
		{
			close(fd);
			unlink(createInfo.tmpname);
			ldapabook_del(LOCALABOOK, createInfo.tmpname,
				      cgi("ABOOK"));
			maildir_tmpcreate_free(&createInfo);
		}
	}

	abooks[0]=ldapabook_read(LDAPADDRESSBOOK);
	abooks[1]=ldapabook_read(LOCALABOOK);

	printf("<TABLE BORDER=0 CELLPADDING=8 WIDTH=\"100%%\">\n");
	for (i=0; i<2; i++)
	{
		for (p=abooks[i]; p; p=p->next)
		{
			printf("<TR VALIGN=TOP><TD ALIGN=RIGHT>");
			printf("<INPUT BORDER=0 TYPE=RADIO NAME=ABOOK");

			if (pref_ldap && strcmp(pref_ldap, p->name) == 0)
				printf(" CHECKED");

			printf(" VALUE=\"");
			output_attrencoded(p->name);
			printf("\"></TD><TD><FONT SIZE=\"+1\""
			       " CLASS=\"ldaplist-name\">%s</FONT><BR>"
			       "&nbsp;&nbsp;&nbsp;<TT><FONT SIZE=\"-2\""
			       " CLASS=\"ldaplist-ldapurl\">ldap://", p->name);
			if (*p->binddn || *p->bindpw)
			{
				printf("%s", p->binddn);
				if (*p->bindpw)
					printf(":%s", p->bindpw);
				printf("@");
			}
			printf("%s", p->host);
			if (atoi(p->port) != 389)
				printf(":%s", p->port);
			if (*p->suffix)
			{
			char	*q;

				printf("/");
				q=cgiurlencode_noeq(p->suffix);
				if (q)
				{
					printf("%s", q);
					free(q);
				}
			}
			printf("</FONT></TT>%s</TD></TR>",
				i ? "":sysbook);
		}
	}

	if (abooks[1])
	{
		printf("<TR><TD></TD><TD>");
		printf("<input type=submit name=delabook value=\"%s\">",
				delabook);
		printf("</TD></TR>\n");
	}
	printf("</TABLE>\n");
	ldapabook_free(abooks[0]);
	ldapabook_free(abooks[1]);
}

int	ldapsearch()
{
	if (*cgi("ABOOK") == 0 || *cgi("attr1") == 0 || *cgi("op1") == 0
		|| *cgi("value1") == 0) return (-1);
	return (0);
}

static const char *getattrn(const char *s, unsigned n)
{
char	buf1[NUMBUFSIZE+20], bufn[NUMBUFSIZE];

	return (cgi(strcat(strcpy(buf1, s), libmail_str_size_t(n, bufn))));
}

static char *getfiltern(unsigned n)
{
const char *attrname=getattrn("attr", n);
const char *attrop=getattrn("op", n);
const char *attrval=getattrn("value", n);

char *buf;

	if (*attrname == 0 || *attrop == 0 || *attrval == 0)	return (0);

	buf=malloc(strlen(attrname)+strlen(attrop)+strlen(attrval)+40);

	if (!buf)	return (0);

	strcpy(buf, "(");
	strcat(buf, attrname);
	strcat(buf, strcmp(attrop, "=*") == 0 ? "=":attrop);
	strcat(buf, attrval);
	if (strcmp(attrop, "=*") == 0)
		strcat(buf, "*");
	strcat(buf, ")");
	return (buf);
}

static char *getfilter()
{
char	*filter=0;
char	*s;
unsigned n=atoi(cgi("maxattrs"));
unsigned i;

	if (n < 3)	n=3;
	if (n > 20)	n=20;	/* sanity check */

	for (i=0; i<n; i++)
	{
	char	*p=getfiltern(i+1);

		if (!p)
			continue;

		if (!filter)	filter=p;
		else
		{
			s=malloc(strlen(p) + strlen(filter));
			if (!s)
			{
				free(filter);
				free(p);
				return (0);
			}
			strcat(strcpy(s, filter), p);
			free(filter);
			free(p);
			filter=s;
		}
	}
	s=malloc(strlen(filter)+sizeof("(&)"));
	if (!s)
	{
		free(filter);
		return(0);
	}
	strcat(strcat(strcpy(s, "(&"), filter), ")");
	free(filter);
	return (s);
}

static void parsesearch(FILE *, FILE *);

void	doldapsearch()
{
char	*f;
struct ldapabook *abooks[2];
const struct ldapabook *ptr;

	abooks[0]=ldapabook_read(LDAPADDRESSBOOK);
	abooks[1]=ldapabook_read(LOCALABOOK);

	ptr=ldapabook_find(abooks[0], cgi("ABOOK"));
	if (!ptr)
		ptr=ldapabook_find(abooks[1], cgi("ABOOK"));

	if (ptr && (f=getfilter()) != 0)
	{
	int	fd;
	FILE	*fpw=0;
	char	*tmpname=0;

		pref_setldap(ptr->name);
		printf("<PRE>");
		fflush(stdout);

		fd=ldapabook_search(ptr, LDAPSEARCH, f, 1);
		free(f);

                if (fd >= 0)
		{
		FILE *fp=fdopen(fd, "r");

			if (fp)
			{
				struct maildir_tmpcreate_info createInfo;

				maildir_tmpcreate_init(&createInfo);
				createInfo.uniq="ldap";
				createInfo.doordie=1;

				fpw=maildir_tmpcreate_fp(&createInfo);

				tmpname=createInfo.tmpname;
				createInfo.tmpname=NULL;

				if (fpw)
					parsesearch(fp, fpw);
				else
					perror(tmpname);

				maildir_tmpcreate_free(&createInfo);

				fclose(fp);
			}
			else
				perror("fdopen");
			close(fd);
		}
		printf("</PRE>");

		if (fpw)
		{
		int	c;

			fflush(fpw);
			rewind(fpw);
			while ((c=getc(fpw)) != EOF)
				putchar(c);
			fclose(fpw);
		}
		if (tmpname)
		{
			unlink(tmpname);
			free(tmpname);
		}
	}
}

static void parsesearch(FILE *r, FILE *w)
{
char	buf[BUFSIZ];
char	sn[100];
char	cn[100];
char	givenname[100];
char	o[100];
char	l[100];
char	ou[100];
char	st[100];
char	mail[512];
char	*p;
const char	*add1=getarg("ADD"),
	 *add2=getarg("CREATE"),
	 *submit=getarg("SUBMIT");

char	numbuf[NUMBUFSIZE];
char	numbuf2[NUMBUFSIZE+10];
unsigned counter;

#if HAVE_SQWEBMAIL_UNICODE
const struct unicode_info *uiptr;
#endif
 
	fprintf(w, "<TABLE BORDER=0 CELLPADDING=4>\n");

	counter=0;
	for (;;)
	{
		if (fgets(buf, sizeof(buf), r) == 0)	break;
		/* skip dn */

		sn[0]=0;
		cn[0]=0;
		o[0]=0;
		l[0]=0;
		ou[0]=0;
		st[0]=0;
		mail[0]=0;
		givenname[0]=0;

		for (;;)
		{
			char *base64buf=NULL;

			if (fgets(buf, sizeof(buf), r) == 0)	break;
			if ((p=strchr(buf, '\n')) != 0)	*p=0;
			if (buf[0] == 0)	break;

			for (p=buf; *p; p++)
			{
				if (*p == ':')
				{
					*p++=0;

					if (*p != ':')
					{
						while (*p &&
						       isspace((int)
							       (unsigned char)
							       *p))
							++p;
						break;
					}

					++p;
					while (*p &&isspace((int)
							    (unsigned char)*p))
						++p;

					base64buf=base64_decode_str(p);
					if (base64buf)
						p=base64buf;

#if HAVE_SQWEBMAIL_UNICODE
					if ((uiptr=unicode_find(sqwebmail_content_charset)) != 0 &&
					    (base64buf=unicode_cfromutf8(uiptr,
									 p,
									 NULL))
					    != NULL)
						p=base64buf;

#endif

					break;
				}
				*p=tolower((int)(unsigned char)*p);
			}

#define	SAFECAT(b,buf) strncat(b, buf, sizeof(b)-1-strlen(b))

#define	SAVE(n,b) if (strcmp(buf, n) == 0) \
	{ if (b[0]) SAFECAT(b, "\n"); SAFECAT(b, p); }

			SAVE("sn", sn);

			SAVE("cn", cn);
			SAVE("givenname", givenname);
			SAVE("o", o);
			SAVE("ou", ou);
			SAVE("l", l);
			SAVE("st", st);
			SAVE("mail", mail);
			if (base64buf)
				free(base64buf);
		}
		if (mail[0] == 0)	continue;

		for (p=mail; (p=strtok(p, "\n")) != 0; p=0)
		{
		char	*q;

			fprintf(w, "<TR VALIGN=TOP><TD><INPUT TYPE=CHECKBOX "
				"NAME=\"%s\" VALUE=\"&lt;",
				strcat(strcpy(numbuf2, "ADDY"),
					libmail_str_size_t(counter++, numbuf)));

			output_attrencoded_fp(p, w);
			fprintf(w, "&gt;");

			q=cn;

			if (*q)
			{
				fprintf(w, " &quot;");
				output_attrencoded_fp(q, w);
				fprintf(w, "&quot;");
			}
			else if (sn[0] || givenname[0])
			{
				fprintf(w, " &quot;");
				if (givenname[0])
					output_attrencoded_fp(givenname,
							      w);
				if (sn[0] && givenname[0])
					fprintf(w, " ");
				if (sn[0])
					output_attrencoded_fp(sn,
							      w);
			}


			fprintf(w, "\"></TD><TD><FONT SIZE=\"+1\" CLASS=\"ldapsearch-name\">");

			if (*q)
			{
				fprintf(w, "\"");
				output_attrencoded_fp(q, w);
				fprintf(w, "\" ");
			}
			fprintf(w, "</FONT><FONT SIZE=\"+1\" CLASS=\"ldapsearch-addr\">&lt;");
			output_attrencoded_fp(p, w);
			fprintf(w, "&gt;</FONT>");
			printf("<SPAN CLASS=\"ldapsearch-misc\">");
			if (ou[0])
			{
				fprintf(w, "<BR>");
				output_attrencoded_oknl_fp(ou, w);
			}
			if (o[0])
			{
				fprintf(w, "<BR>");
				output_attrencoded_oknl_fp(o, w);
			}
			if (l[0])
			{
				fprintf(w, "<BR>");
				output_attrencoded_oknl_fp(l, w);
			}
			if (st[0])
			{
				fprintf(w, "<BR>");
				output_attrencoded_oknl_fp(st, w);
			}
			printf("</SPAN>");
			fprintf(w, "</TD></TR>\n");
		}
	}
	fprintf(w, "<TR><TD COLSPAN=2><HR WIDTH=\"90%%\">"
		"<INPUT TYPE=HIDDEN NAME=ADDYCNT VALUE=%u>\n"
		"</TD></TR>\n", counter);
	fprintf(w, "<TR><TD COLSPAN=2><TABLE>");

	fprintf(w, "<TR><TD ALIGN=RIGHT>%s</TD><TD>"
			"<SELECT NAME=nick1><OPTION>\n", add1);
	ab_listselect_fp(w);
	fprintf(w, "</SELECT></TD></TR>\n");
	fprintf(w, "<TR><TD ALIGN=RIGHT>%s</TD><TD>"
			"<INPUT TYPE=TEXT NAME=nick2></TD></TR>\n", add2);
	fprintf(w, "<TR><TD></TD><TD>"
		"<INPUT TYPE=SUBMIT NAME=import VALUE=\"%s\"></TD></TR>",
		submit);
	fprintf(w, "</TABLE></TD></TR></TABLE>\n");
}
