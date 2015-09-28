# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:15:18 lfan Exp $
#
# Copyright 1998 - 2000 Double Precision, Inc.  See COPYING for
# distribution information.

@SET_MAKE@

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

CLEANFILES=htmllibdir.h ldapsearch webgpg ldapaddressbook.dist \
	cron.cmd sqwebmail.msg
DISTCLEANFILES=webmail.authpam

SUBDIRS=images html

scriptdir=@scriptdir@
script_SCRIPTS=sendit.sh cleancache.pl ldapsearch webgpg
sysconf_DATA=ldapaddressbook.dist
pkglibexecdir=$(libexecdir)/sqwebmail

noinst_DATA=SECURITY BUGS TODO SECURITY.html BUGS.html TODO.html

#
#  Because cgibindir is not based on prefix, we must conditionally install
#  bin_PROGRAMS in install-exec-local, otherwise make distcheck will fail.
#

noinst_PROGRAMS=sqwebmail

COMMONDEPENDENCIES=$(LIBOBJS) ../pcp/libpcp.a ../maildir/libmaildir.a \
	../rfc2045/librfc2045.a ../rfc822/librfc822.a \
	../http11/libhttp11.a ../cgi/libcgi.a \
	../authlib/libauth.a \
	../ldapaddressbook/libaddressbook.a \
	../gpglib/libgpg.a \
	../unicode/libunicode.a \
	../random128/librandom128.a ../md5/libmd5.a ../numlib/libnumlib.a \
	@dblibrary@
COMMONLDADD=$(COMMONDEPENDENCIES) `cat ../authlib/libauth.dep` `cat ../pcp/pcp.libs` @LIBDB@ @LIBGDBM@

sqwebmail_LDADD=$(COMMONLDADD) @LIBVCHKPW@ @NETLIBS@
sqwebmail_DEPENDENCIES=$(COMMONDEPENDENCIES)

ISPELLSOURCES=sqispell.c ispell.c ispell.h

BUILT_SOURCES=htmllibdir.h

EXTRA_DIST=$(noinst_DATA) sqwebmail.pamconf sqwebmail-system-auth.pamconf\
           strdup.c strcasecmp.c strncasecmp.c \
           $(ISPELLSOURCES)

#CFLAGS += -Dmalloc=my_malloc -Drealloc=my_realloc -Dfree=my_free -Dstrdup=my_strdup -Dcalloc=my_calloc

if ISPELL
SQISPELLSOURCES=$(ISPELLSOURCES)
endif


sqwebmail_SOURCES=addressbook.c addressbook.h args.c attachments.c \
		auth.c auth.h autoresponse.c autoresponse.h buf.c buf.h \
		filter.c filter.h folder.c folder.h gpg.c gpg.h html.c html.h \
		ldaplist.c logindomainlist.c logindomainlist.h \
		maildir.c maildir.h mailfilter.c \
		mailfilter.h mailinglist.c mailinglist.h newmsg.c newmsg.h \
		newmsg_create.c	newmsg_newdraft.c pcp.c pcp.h pref.c pref.h \
		rfc2646html.c rfc2646html.h sqconfig.c sqconfig.h sqwebmail.c \
		sqwebmail.h token.c token.h $(SQISPELLSOURCES)


HTML2TXT=`which lynx 2>/dev/null && echo "lynx -dump -nolist" && exit; echo "links -dump"`

SECURITY: SECURITY.html
	rm -f SECURITY
	$(HTML2TXT) $(srcdir)/SECURITY.html >SECURITY

BUGS: BUGS.html
	rm -f BUGS
	$(HTML2TXT) $(srcdir)/BUGS.html >BUGS

TODO: TODO.html
	rm -f TODO
	$(HTML2TXT) $(srcdir)/TODO.html >TODO

install-perms:
	@$(MAKE) install-perms-recursive
	@$(MAKE) install-perms-local

install-perms-recursive:
	@>permissions.dat
	@SUBDIRS="$(SUBDIRS)" ; for f in $$SUBDIRS ; do ( cd $$f && $(MAKE) $(AM_MAKEFLAGS) install-perms ) ; cat /dev/null >>$$f/permissions.dat ; cat $$f/permissions.dat >>permissions.dat ; done

install-perms-local: install-perms-cache
	@echo "$(scriptdir)/webgpg 755 bin bin" >>permissions.dat
	@echo "$(scriptdir)/sendit.sh 755 bin bin" >>permissions.dat
	@echo "$(scriptdir)/ldapsearch 755 bin bin" >>permissions.dat
	@echo "$(sysconfdir)/ldapaddressbook.dist 644 root bin config" >>permissions.dat

htmllibdir.h: config.status
	echo '#define HTMLLIBDIR "@htmldir@"' >htmllibdir.h
	echo '#define SENDITSH "$(scriptdir)/sendit.sh"' >>htmllibdir.h
	echo '#define NOCHANGINGFROM "$(sysconfdir)/nochangingfrom"' >>htmllibdir.h
	echo '#define USEXSENDER "$(sysconfdir)/usexsender"' >>htmllibdir.h
	echo '#define CALENDARMODE "$(sysconfdir)/calendarmode"' >>htmllibdir.h
	echo '#define LOGINDOMAINLIST "$(sysconfdir)/logindomainlist"' >>htmllibdir.h
	echo '#define MAXMSGSIZE @MAXMSGSIZE@' >>htmllibdir.h
	echo '#define IMGPATH "@imageurl@"' >>htmllibdir.h
	echo '#define NODSN "$(sysconfdir)/nodsn"' >>htmllibdir.h
	if test -d $(srcdir)/../courier \
		; then \
		echo '#define HOSTNAMEFILE "$(sysconfdir)/defaultdomain"' >>htmllibdir.h \
		; echo '#define MAKEMIME "$(bindir)/makemime"' >>htmllibdir.h \
		; echo '#define MIMEGPG "$(bindir)/mimegpg"' >>htmllibdir.h \
		; else \
		echo '#define HOSTNAMEFILE "$(sysconfdir)/hostname"' >>htmllibdir.h \
		; echo '#define MAKEMIME "$(pkglibexecdir)/makemime"' >>htmllibdir.h \
		; echo '#define MIMEGPG "$(pkglibexecdir)/mimegpg"' >>htmllibdir.h \
		; fi
	echo '#define LDAPADDRESSBOOK "$(sysconfdir)/ldapaddressbook"' >>htmllibdir.h
	echo '#define LDAPSEARCH "$(scriptdir)/ldapsearch"' >>htmllibdir.h
	echo '#define MODULEFILE "$(sysconfdir)/authmodulelist"' >>htmllibdir.h
	echo '#define NOIMAGES "$(sysconfdir)/noimages"' >>htmllibdir.h

ldapsearch: ../ldapaddressbook/ldapsearch
	cp ../ldapaddressbook/ldapsearch .

webgpg: ../gpglib/webgpg
	cp ../gpglib/webgpg .

ldapaddressbook.dist: ../ldapaddressbook/ldapaddressbook.dist
	cp $(srcdir)/../ldapaddressbook/ldapaddressbook.dist .

install-data-local: cache-reminder

configure-check:
	@echo "URL to the image directory is @imageurl@"

install-perms-cache:
	@echo "$(scriptdir)/cleancache.pl 755 bin bin" >>permissions.dat
	@echo "@cachedir@ 700 @cacheowner@ bin" >>permissions.dat

cache-reminder:
	-$(mkinstalldirs) $(DESTDIR)@cachedir@
	test -w /etc || exit 0; chown @cacheowner@ $(DESTDIR)@cachedir@
	@rm -f cron.cmd
	@echo '#! @SHELL@' >cron.cmd
	@echo 'su -p -c "$(scriptdir)/cleancache.pl" @cacheowner@' >>cron.cmd
	@chmod 555 cron.cmd
	@cp /dev/null sqwebmail.msg
	@echo '-----------------------------------------------------------' >>sqwebmail.msg
	@echo 'You must now create the following cron job that runs at' >>sqwebmail.msg
	@echo 'regular intervals:' >>sqwebmail.msg
	@echo '' >>sqwebmail.msg
	@echo 'su -c "$(scriptdir)/cleancache.pl" @cacheowner@' >>sqwebmail.msg
	@echo '' >>sqwebmail.msg
	@echo '-----------------------------------------------------------' >>sqwebmail.msg
	@echo '(note - your su command may use slightly different arguments)' >>sqwebmail.msg
	@echo '' >>sqwebmail.msg
	@echo '' >>sqwebmail.msg

