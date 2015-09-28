# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:14:59 lfan Exp $
#
# Copyright 2001-2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

CLEANFILES=sysconfdir.h uids.h calendardir.h uids

DISTCLEANFILES=pcp.libs

SUBDIRS=intl m4 po

noinst_LIBRARIES=libpcp.a
noinst_PROGRAMS=pcp pcpd

EXTRA_DIST= mkinstalldirs  README.html

BUILT_SOURCES=calendardir.h sysconfdir.h uids.h

libpcp_a_SOURCES=pcp.h pcpacl.c pcpdir.c pcpgmtime.c pcplocale.c \
	pcpaddcommon.c pcpparseymd.c pcpmksocket.c pcpnet.c uids.c

pcp_SOURCES=pcp.c
pcp_DEPENDENCIES=libpcp.a ../rfc2045/librfc2045.a ../rfc822/librfc822.a \
	../unicode/libunicode.a ../numlib/libnumlib.a 
pcp_LDADD=$(pcp_DEPENDENCIES) @LIBINTL@ @EFENCE@ @NETLIBS@

pcpd_SOURCES=pcpd.c pcpdauth.c pcpdauth.h pcpdauthme.c pcpdauthtoken.c \
	pcpdauthtoken.h pcpdtimer.c pcpdtimer.h

COMMONLIBS=libpcp.a ../rfc2045/librfc2045.a ../rfc822/librfc822.a \
	../authlib/libauth.a ../maildir/libmaildir.a \
	../random128/librandom128.a ../libhmac/libhmac.a \
	../md5/libmd5.a ../sha1/libsha1.a\
	../unicode/libunicode.a ../liblock/liblock.a ../numlib/libnumlib.a 

pcpd_DEPENDENCIES=$(COMMONLIBS) ../authlib/libauth.dep
pcpd_LDADD=$(COMMONLIBS) `cat ../authlib/libauth.dep` @LIBINTL@ @EFENCE@ @NETLIBS@

calendardir.h: config.status
	echo '#define CALENDARDIR "$(calendardir)"' >calendardir.h
	echo '#define LOCKFILE CALENDARDIR "/50pcpdlocal.lock"' >>calendardir.h
	echo '#define PIDFILE CALENDARDIR "/50pcpdlocal.pid"' >>calendardir.h
	echo '#define RANDTOKENFILE CALENDARDIR "/50pcpdlocal.tokens"' >>calendardir.h
	echo '#define PUBDIR CALENDARDIR "/public"' >>calendardir.h
	echo '#define PRIVDIR CALENDARDIR "/private"' >>calendardir.h
	echo '#define CACHEDIR "@localpcpdcachedir@"' >>calendardir.h
	echo '#define TIMEOUT @TIMEOUT@' >>calendardir.h
	echo '#define SENDIT "@SENDIT@"' >>calendardir.h
	if test ! -d $(srcdir)/../courier ; then echo '#define HOSTNAMEFILE SYSCONFDIR "/hostname"' >>calendardir.h ; echo '#define HOSTNAMEFILE2 "/etc/HOSTNAME"' >>calendardir.h ; else echo '#define HOSTNAMEFILE SYSCONFDIR "/defaultdomain"' >>calendardir.h ; echo '#define HOSTNAMEFILE2 SYSCONFDIR "/me"' >>calendardir.h ; fi

sysconfdir.h: config.status
	echo '#define SYSCONFDIR "$(sysconfdir)"' >sysconfdir.h

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(calendardir)/public
	$(mkinstalldirs) $(DESTDIR)$(calendardir)/private
	$(mkinstalldirs) $(DESTDIR)$(localpcpdcachedir)
	chmod 755 $(DESTDIR)$(calendardir)
	chmod 755 $(DESTDIR)$(calendardir)/public
	chmod 750 $(DESTDIR)$(calendardir)/private
	chmod 700 $(DESTDIR)$(localpcpdcachedir)
	-chown @localcacheowner@ $(DESTDIR)$(localpcpdcachedir)
	-chgrp -R @mailgroup@ $(DESTDIR)$(calendardir)
	echo "localcacheowner=@localcacheowner@" >uids
	echo "calendardir=$(calendardir)" >>uids
	echo "mailgroup=@mailgroup@" >>uids

install-perms:
	@$(MAKE) install-perms-recursive
	@$(MAKE) install-perms-local

uids.h: config.status
	echo '#define uid "@localcacheowner@"' >uids.h
	echo '#define gid "@mailgroup@"' >>uids.h

install-perms-recursive:
	@>permissions.dat
	@SUBDIRS="$(SUBDIRS)" ; for f in $$SUBDIRS ; do ( cd $$f && $(MAKE) $(AM_MAKEFLAGS) install-perms ) ; cat /dev/null >>$$f/permissions.dat ; cat $$f/permissions.dat >>permissions.dat ; done

install-perms-local:
	@echo $(DESTDIR)$(calendardir) 755 @localcacheowner@ @mailgroup@ >>permissions.dat
	@echo $(DESTDIR)$(calendardir)/public 755 @localcacheowner@ @mailgroup@ >>permissions.dat
	@echo $(DESTDIR)$(calendardir)/private 750 @localcacheowner@ @mailgroup@ >>permissions.dat
	@echo $(DESTDIR)$(localpcpdcachedir) 700 @localcacheowner@ @mailgroup@ >>permissions.dat

ACLOCAL_AMFLAGS = -I m4
