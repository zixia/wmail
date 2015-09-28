/*
** Copyright 2000-2001 Double Precision, Inc.  See COPYING for
** distribution information.
*/

/*
** $Id: addressbook.c,v 1.1.1.1 2003/05/07 02:15:25 lfan Exp $
*/

#include	"sqwebmail.h"
#include	"addressbook.h"
#include	"maildir.h"
#include	"cgi/cgi.h"
#include	"rfc822/rfc822.h"
#include	"maildir/maildirmisc.h"
#include	"numlib/numlib.h"
#include	<stdio.h>
#include	<string.h>
#include	<ctype.h>

#define	ADDRESSBOOK	"sqwebmail-addressbook"

extern void output_attrencoded(const char *);
extern void output_attrencoded_fp(const char *, FILE *);
extern void output_urlencoded(const char *);
extern void print_safe(const char *);

/*
** When adding a new name/address pair into the address book delete
** bad characters from both.
*/

static void fix_nameaddr(const char *name, const char *addr,
	char **nameret, char **addrret)
{
char *names, *addresss;
char	*p, *q;

	names=strdup(name);
	if (!names)
	{
		enomem();
	}

	for (p=q=names; *p; p++)
	{
		if (iscntrl((int)(unsigned char)*p))	continue;
		if (*p == '"' || *p == '\\')	continue;
		*q++=*p;
	}
	*q=0;

	if ((addresss=strdup(addr)) == 0)
	{
		free(names);
		enomem();
	}

	for (p=q=addresss; *p; p++)
	{
		if (isspace((int)(unsigned char)*p))	continue;
		if (iscntrl((int)(unsigned char)*p))	continue;
		if (*p == '<' || *p == '>' || *p == '(' || *p == ')' ||
			*p == '\\')
			continue;
		*q++=*p;
	}
	*q=0;

	*nameret=names;
	*addrret=addresss;
}

void ab_add(const char *name, const char *address, const char *nick)
{
char	*nicks, *names, *addresss, *p, *q;
FILE	*fp;
char	*header, *value;

int	new_fd;
char	*new_name;
FILE	*new_fp;
int	written;

	if (*nick == 0 || *address == 0)
		return;

	/* Delete bad characters from nickname, name, address */

	if ((nicks=strdup(nick)) == 0)	enomem();

	for (p=q=nicks; *p; p++)
	{
		if (isspace((int)(unsigned char)*p))	continue;
		if (iscntrl((int)(unsigned char)*p))	continue;
		if (strchr(":;,<>@\\", *p))	continue;
		*q++=*p;
	}
	*q=0;

	if (*nicks == 0)
	{
		free(nicks);
		return;
	}

	/* Remove quotes from name */

	fix_nameaddr(name, address, &names, &addresss);

	if (*addresss == 0)
	{
		free(addresss);
		free(nicks);
		free(names);
		return;
	}

	fp=fopen(ADDRESSBOOK, "r");

	new_fd=maildir_createmsg(INBOX, "addressbook", &new_name);
	p=malloc(sizeof("tmp/")+strlen(new_name));
	if (!p)
	{
		close(new_fd);
		free(new_name);
		enomem();
	}
	strcat(strcpy(p, "tmp/"), new_name);
	free(new_name);
	new_name=p;

	if (new_fd < 0 || (new_fp=fdopen(new_fd, "w")) == 0)
	{
		if (new_fd >= 0)	close(new_fd);
		free(addresss);
		free(nicks);
		free(names);
		enomem();
		return;
	}

	written=0;
	while (fp && (header=maildir_readheader_nolc(fp, &value)) != 0)
	{
		if (strcmp(header, nicks) == 0)
		{
			fprintf(new_fp, "%s: %s,\n    ",
				nicks, value);
			written=1;
			break;
		}
		fprintf(new_fp, "%s: %s\n", header, value);
	}
	if (!written)
		fprintf(new_fp, "%s: ", nicks);
	if (*names)
		fprintf(new_fp, "\"%s\" <%s>\n",
			names, addresss);
	else
		fprintf(new_fp, "<%s>\n", addresss);
	free(names);
	free(addresss);
	free(nicks);

	while (fp && (header=maildir_readheader_nolc(fp, &value)) != 0)
		fprintf(new_fp, "%s: %s\n", header, value);

	if (fp) fclose(fp);

	if (fflush(new_fp) || ferror(new_fp))
	{
		fclose(new_fp);
		close(new_fd);
		unlink(new_name);
		free(new_name);
		error("Unable to write out new address book -- write error, or out of disk space.");
		return;
	}
	fclose(new_fp);

	rename(new_name, ADDRESSBOOK);
	free(new_name);
}

static void dodel(const char *nick, struct rfc822a *a, int n,
	const char *replace_name, const char *replace_addr)
{
char	*p;
FILE	*fp, *new_fp;
char	*new_name;
int	new_fd;
char	*header, *value;

char	*namebuf=0, *addrbuf=0;
struct	rfc822token namet, addresst;

	if (replace_name && replace_addr && n < a->naddrs)
	{
		fix_nameaddr(replace_name, replace_addr, &namebuf, &addrbuf);
		namet.token='"';
		namet.ptr=namebuf;
		namet.len=strlen(namebuf);
		namet.next=0;
		a->addrs[n].name= &namet;

		addresst.token=0;
		addresst.ptr=addrbuf;
		addresst.len=strlen(addrbuf);
		addresst.next=0;
		a->addrs[n].tokens= &addresst;
	}
	else
	{
		while (++n < a->naddrs)
			a->addrs[n-1]=a->addrs[n];
		--a->naddrs;		/* It's that simple... */
	}

	fp=fopen(ADDRESSBOOK, "r");

	new_fd=maildir_createmsg(INBOX, "addressbook", &new_name);
	if (new_fd < 0 || (new_fp=fdopen(new_fd, "w")) == 0)
	{
		if (new_fd >= 0)	close(new_fd);
		fclose(fp);
		enomem();
		return;
	}
	p=malloc(sizeof("tmp/")+strlen(new_name));
	if (!p)
	{
		unlink(new_name);
		fclose(fp);
		fclose(new_fp);
		free(new_name);
		enomem();
	}
	strcat(strcpy(p, "tmp/"), new_name);
	free(new_name);
	new_name=p;

	while (fp && (header=maildir_readheader_nolc(fp, &value)) != 0)
	{
		if (strcmp(header, nick) == 0)
		{
		char	*s, *t;

			if (a->naddrs == 0)
				continue;
			s=rfc822_getaddrs_wrap(a, 70);

			if (!s)
			{
				fclose(new_fp);
				close(new_fd);
				fclose(fp);
				unlink(new_name);
				enomem();
			}
			fprintf(new_fp, "%s: ", header);
				for (t=s; *t; t++)
				{
					putc(*t, new_fp);
					if (*t == '\n')
						fprintf(new_fp, "    ");
				}
			fprintf(new_fp, "\n");
			free(s);
			continue;
		}
		fprintf(new_fp, "%s: %s\n", header, value);
	}
	if (fp) fclose(fp);

	if (namebuf)	free(namebuf);
	if (addrbuf)	free(addrbuf);

	if (fflush(new_fp) || ferror(new_fp))
	{
		fclose(new_fp);
		unlink(new_name);
		free(new_name);
		error("Unable to write out new address book -- write error, or out of disk space.");
		return;
	}

	fclose(new_fp);
	rename(new_name, ADDRESSBOOK);
	free(new_name);
}

static void dodelall(const char *nick)
{
FILE	*fp, *new_fp;
int	new_fd;
char	*new_name, *p;
char	*header, *value;

	fp=fopen(ADDRESSBOOK, "r");

	new_fd=maildir_createmsg(INBOX, "addressbook", &new_name);
	if (new_fd < 0 || (new_fp=fdopen(new_fd, "w")) == 0)
	{
		if (new_fd >= 0)	close(new_fd);
		fclose(fp);
		enomem();
		return;
	}
	p=malloc(sizeof("tmp/")+strlen(new_name));
	if (!p)
	{
		unlink(new_name);
		fclose(fp);
		fclose(new_fp);
		free(new_name);
		enomem();
	}
	strcat(strcpy(p, "tmp/"), new_name);
	free(new_name);
	new_name=p;

	while (fp && (header=maildir_readheader_nolc(fp, &value)) != 0)
	{
		if (strcmp(header, nick) == 0)	continue;
		fprintf(new_fp, "%s: %s\n", header, value);
	}
	if (fp) fclose(fp);

	if (fflush(new_fp) || ferror(new_fp))
	{
		fclose(new_fp);
		unlink(new_name);
		free(new_name);
		error("Unable to write out new address book -- write error, or out of disk space.");
		return;
	}

	fclose(new_fp);
	rename(new_name, ADDRESSBOOK);
	free(new_name);
}

void ab_listselect()
{
	ab_listselect_fp(stdout);
}

struct abooklist {
	struct abooklist *next;
	char *name;
	} ;

static void abl_free(struct abooklist *a)
{
struct abooklist *b;

	while (a)
	{
		b=a->next;
		free(a->name);
		free(a);
		a=b;
	}
}

static int sortabook(const void *a, const void *b)
{
	return ( strcmp( (*(struct abooklist * const *)a)->name,
			(*(struct abooklist * const *)b)->name));
}

void ab_listselect_fp(FILE *w)
{
FILE	*fp;
char *header, *value;
struct	abooklist *a=0, *b, **aa;
size_t	acnt=0, i;

	if ((fp=fopen(ADDRESSBOOK, "r")) != 0)
	{
		while ((header=maildir_readheader_nolc(fp, &value)) != NULL)
		{
			if ((b=malloc(sizeof(struct abooklist))) == 0 ||
				(b->name=strdup(header)) == 0)
			{
				if (b)	free(b);
				abl_free(a);
				enomem();
			}
			b->next=a;
			a=b;
			acnt++;
		}
		fclose(fp);

		if ((aa=malloc(sizeof(struct abooklist *)*(acnt+1))) == 0)
		{
			abl_free(a);
			enomem();
		}

		for (acnt=0, b=a; b; b=b->next)
			aa[acnt++]=b;
		qsort(aa, acnt, sizeof(*aa), sortabook);

		for (i=0; i<acnt; i++)
		{
			fprintf(w, "<option value=\"");
			output_attrencoded_fp(aa[i]->name, w);
			fprintf(w, "\">");
			output_attrencoded_fp(aa[i]->name, w);
			fprintf(w, "\n");
		}
		free(aa);
		abl_free(a);
	}
}

/*
** Extract all name/address entries from the address book, for external
** processing (mostly calendaring).
*/

int ab_get_nameaddr( int (*callback_func)(const char *, const char *,
					  void *),
		     void *callback_arg)
{
	FILE	*fp;
	char *header, *value;
	int rc=0;

	if ((fp=fopen(ADDRESSBOOK, "r")) != 0)
	{
		while ((header=maildir_readheader_nolc(fp, &value)) != NULL)
		{
			struct rfc822t *t;
			struct rfc822a *a;

			if (!value)
				continue;

			t=rfc822t_alloc_new(value, NULL, NULL);
			a=t ? rfc822a_alloc(t):0;

			if (a)
			{
				int i;

				for (i=0; i<a->naddrs; i++)
				{
					char *addr=rfc822_getaddr(a, i);
					char *name;

					if (!addr)
						continue;

					name=a->addrs[i].name ?
						rfc822_getname(a, i):NULL;

					rc=(*callback_func)(addr, name,
							    callback_arg);
					if (name)
						free(name);
					free(addr);
					if (rc)
						break;
				}
			}

			if (a) rfc822a_free(a);
			if (t) rfc822t_free(t);
			if (rc)
				break;
		}
		fclose(fp);
	}
	return (rc);
}

struct ab_addrselect_s {
	struct ab_addrselect_s *next;
	char *name;
	char *addr;
} ;

static int ab_addrselect_cb(const char *a, const char *n, void *vp)
{
	struct ab_addrselect_s **p=(struct ab_addrselect_s **)vp, *q;

	if (!n || !a)
		return (0);

	while ( (*p) && strcasecmp((*p)->name, n) < 0)
		p= &(*p)->next;

	if ((q=malloc(sizeof(struct ab_addrselect_s))) == NULL)
		return (-1);

	if ((q->name=strdup(n)) == NULL)
	{
		free(q);
		return(-1);
	}

	if ((q->addr=strdup(a)) == NULL)
	{
		free(q->name);
		free(q);
		return(-1);
	}

	q->next= *p;
	*p=q;
	return (0);
}

void ab_nameaddr_show(const char *name, const char *addr)
{
	if (name)
	{
		printf("\"");
		print_safe(name);
		printf("\"&nbsp;");
	}
	printf("&lt;");
	if (addr)
		print_safe(addr);
	printf("&gt;");
}

void ab_addrselect()
{
	struct ab_addrselect_s *list=NULL, *p;

	printf("<select width=20 name=addressbookname><option value=""></option>\n");

	if (ab_get_nameaddr(ab_addrselect_cb, &list) == 0)
	{
		for (p=list; p; p=p->next)
		{
			printf("<option value=\"");
			output_attrencoded(p->addr);
			printf("\">");
			ab_nameaddr_show(p->name, p->addr);
			printf("</option>\n");
		}
	}
	printf("</select>\n");

	while ((p=list) != NULL)
	{
		list=p->next;
		free(p->name);
		free(p->addr);
		free(p);
	}
}

const char *ab_find(const char *nick)
{
FILE	*fp;
char *header, *value;

	if ((fp=fopen(ADDRESSBOOK, "r")) != 0)
	{
		while ((header=maildir_readheader_nolc(fp, &value)) != NULL)
		{
			if (strcmp(header, nick) == 0)
			{
				fclose(fp);
				return (value);
			}
		}
		fclose(fp);
	}
	return (0);
}
	
void addressbook()
{
FILE	*fp;
char    *header, *value;
const char	*nick_prompt=getarg("PROMPT");
const char	*nick_submit=getarg("SUBMIT");
const char	*nick_title1=getarg("TITLE1");
const char	*nick_title2=getarg("TITLE2");
const char	*nick_delete=getarg("DELETE");
const char	*nick_name=getarg("NAME");
const char	*nick_address=getarg("ADDRESS");
const char	*nick_add=getarg("ADD");
const char	*nick_edit=getarg("EDIT");
const char *nick1;
int	do_edit;
char	*s, *q, *r;

char	*edit_name=0;
char	*edit_addr=0;
int	replace_index=0;

	nick1=cgi("nick");
	do_edit=0;
	if (*cgi("nick.edit"))
		do_edit=1;
	else if (*cgi("nick.edit2"))
	{
		do_edit=1;
		nick1=cgi("nick2");
	}
	else if (*cgi("editnick"))
	{
		do_edit=1;
		nick1=cgi("editnick");
	}

	if (*cgi("ADDYCNT"))	/* Import from LDAP */
	{
	unsigned counter=atoi(cgi("ADDYCNT"));
	char	numbuf[NUMBUFSIZE];
	char	numbuf2[NUMBUFSIZE+10];
	unsigned	i;

		if (counter < 1 || counter > 1000)
			counter=1000;
		nick1=cgi("nick2");
		if (!*nick1)
			nick1=cgi("nick1");

		if (*nick1)
		{
			do_edit=1;
			for (i=0; i<counter; i++)
			{
			const char *addy=cgi(strcat(strcpy(numbuf2, "ADDY"),
                                        libmail_str_size_t(i, numbuf)));
			char	*addycpy;
			char	*name;

				if (*addy == 0)	continue;

				addycpy=strdup(addy);
				if (!addycpy)	enomem();

				name=strchr(addycpy, '>');
				if (!name)
				{
					free(addycpy);
					continue;
				}
				*name++=0;
				while (*name == ' ')	++name;
				addy=addycpy;
				if (*addy == '<')	++addy;
				ab_add(name, addy, nick1);
			}
		}
	}

	if (*cgi("nick.delete"))
	{
		do_edit=0;
		dodelall(cgi("nick"));
	}
	else if (*cgi("add"))
	{
	const char *newname=cgi("newname");
	const char *newaddr=cgi("newaddress");
	const char *editnick=cgi("editnick");
	const char *replacenum=cgi("replacenick");

		if (*replacenum)
		{
			if ((fp=fopen(ADDRESSBOOK, "r")) != 0)
			{
				while ((header=maildir_readheader_nolc(fp,
					&value)) != NULL)
					if (strcmp(header, editnick) == 0)
						break;
				if (header)
				{
				struct rfc822t *t;
				struct rfc822a *a;

					t=rfc822t_alloc_new(value, NULL, NULL);
					a=t ? rfc822a_alloc(t):0;

					if (a)
					{
						dodel(editnick, a,
							atoi(replacenum),
							newname,
							newaddr);
						rfc822a_free(a);
					}
					if (t)	rfc822t_free(t);
				}
				fclose(fp);
			}
		}
		else
			ab_add(newname, newaddr, editnick);
		do_edit=1;
		nick1=editnick;
	}

	printf("%s", nick_prompt);

	printf("<select name=\"nick\">\n");
	printf("<option value=\"\">\n");

	ab_listselect();

	printf("</select>\n");
	printf("%s", nick_submit);

	s=strdup(nick1);
	if (!s)	enomem();
	for (q=r=s; *q; q++)
	{
		if (isspace((int)(unsigned char)*q) ||
			strchr(",;:()\"%@<>'!", *q))
			continue;
		*r++=*q;
	}
	*r=0;

	if (do_edit && *s)
	{
		printf("%s%s%s", nick_title1, s, nick_title2);
		printf("<input type=hidden name=\"editnick\" value=\"");
		output_attrencoded(s);
		printf("\">\n");

		printf("<table border=0>");


		if ((fp=fopen(ADDRESSBOOK, "r")) != 0)
		{
			while ((header=maildir_readheader_nolc(fp, &value))
				!= NULL)
				if (strcmp(header, s) == 0)
					break;
			if (header)
			{
			struct rfc822t *t;
			struct rfc822a *a;
			char	*save_value=strdup(value);

				if (!save_value)
				{
					fclose(fp);
					free(s);
					enomem();
				}
				strcpy(save_value, value);
					/* Need copy 'cause dodel also
					** calls maildir_readheader */


				t=rfc822t_alloc_new(save_value, NULL, NULL);
				a=t ? rfc822a_alloc(t):0;

				if (a)
				{
				int	i;

					for (i=0; i<a->naddrs; i++)
					{
					char buf[100];

						sprintf(buf, "del%d", i);
						if (*cgi(buf))
						{
							dodel(s, a, i, 0, 0);
							break;
						}
						sprintf(buf, "startedit%d", i);
						if (*cgi(buf))
						{
							if (edit_name)
								free(edit_name);
							edit_name=
								rfc822_getname(
									a, i);
							if (edit_addr)
								free(edit_addr);
							edit_addr=
								rfc822_getaddr(
									a, i);
							replace_index=i;
							break;
						}
					}
				}

				if (a)
				{
				int	i;

					for (i=0; i<a->naddrs; i++)
					{
					char *s;

						if (a->addrs[i].tokens == 0)
							continue;
						printf("<tr><td align=right"
						       " class=\"nickname\">");

						s=rfc822_getname(a, i);

						if (s && a->addrs[i].name)
							/* getname defaults it
							** here.
							*/
						{
							printf("\"");
							output_attrencoded(s);
							free(s);
							printf("\"");
						}
						printf("</td><td align=left"
						       " class=\"nickaddr\">"
						       "&lt;");
						s=rfc822_getaddr(a, i);
						if (s)
						{
							output_attrencoded(s);
							free(s);
						}
						printf("&gt;</td><td><input class=mybtn type=submit name=\"startedit%d\" value=\"%s\">&nbsp;<input class=mybtn type=submit name=\"del%d\" value=\"%s\"></td></tr>\n",
							i, nick_edit,
							i, nick_delete);
					}
					rfc822a_free(a);
				}
				if (t)	rfc822t_free(t);
				free(save_value);
			}
			fclose(fp);
		}
		printf("<tr><td colspan=3><hr width=\"90%%\"></td></tr>\n");
		printf("<tr><td align=right>%s</td><td colspan=2><input class=myinput type=text name=newname", nick_name);

		if (edit_name)
		{
			printf(" value=\"");
			output_attrencoded(edit_name);
			printf("\"");
		}
		printf("></td></tr>\n");

		printf("<tr><td align=right>%s</td><td><input class=myinput type=text name=newaddress", nick_address);
		if (edit_addr)
		{
			printf(" value=\"");
			output_attrencoded(edit_addr);
			printf("\"");
		}

		printf("></td><td>");

		if (edit_name || edit_addr)
			printf("<input type=hidden name=replacenick value=%d>",
				replace_index);

		printf("<input class=mybtn type=submit name=add value=\"%s\"></td></tr>\n",
			edit_name || edit_addr ? nick_edit:nick_add);

		printf("</table>");
	}
	free(s);

	if (edit_name)
		free(edit_name);
	if (edit_addr)
		free(edit_addr);
}
