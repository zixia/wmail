#$Id: Makefile.am,v 1.1.1.1 2003/05/07 02:14:14 lfan Exp $
#
# Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

noinst_LIBRARIES=libmaildir.a


DOCS=	deliverquota.html.in deliverquota.8.in \
	maildir.html maildir.5 \
	maildirmake.html.in maildirmake.1.in maildirquota.html maildirquota.7

if HAVE_SGML
BUILT_SOURCES=maildirsharedrc.h maildirfilterconfig.h quotawarnmsg.h \
	mailbot.h autoresponsequota.h $(noinst_DATA) $(DOCS)
else
BUILT_SOURCES=maildirsharedrc.h maildirfilterconfig.h quotawarnmsg.h \
	mailbot.h autoresponsequota.h $(noinst_DATA)
endif

noinst_DATA=deliverquota.html maildirmake.html deliverquota.8 maildirmake.1 \
	maildir.libdeps

libmaildir_a_SOURCES=autoresponse.c autoresponse.h \
	maildircache.c maildircache.h \
	maildircreate.c maildircreate.h \
	maildirfilename.c maildirgetnew.c \
	maildirfilter.c maildirfilter2.c \
	maildirfilter.h maildirfiltertypelist.h\
	maildirflags.c maildirmkdir.c \
	maildirgetquota.c maildirgetquota.h maildirlist.c \
	maildiropen.c maildirparsequota.c \
	maildirpath.c maildirpurgetmp.c maildirmisc.h \
	maildirrename.c \
	maildirsearch.c maildirsearchC.cpp maildirsearch.h \
	maildirshared.c maildirshared2.c maildirdelfolder.c\
	maildirquota.c maildirquota.h maildirrequota.c maildirrequota.h \
	maildirwatch.c maildirwatch.h

noinst_PROGRAMS=deliverquota maildirmake testmaildirfilter

deliverquota_SOURCES=deliverquota.c
deliverquota_DEPENDENCIES=libmaildir.a ../rfc822/librfc822.a \
				../numlib/libnumlib.a
deliverquota_LDADD=libmaildir.a ../rfc822/librfc822.a ../numlib/libnumlib.a

maildirmake_SOURCES=maildirmake.c
maildirmake_DEPENDENCIES=libmaildir.a ../numlib/libnumlib.a \
			../rfc822/librfc822.a
maildirmake_LDADD=libmaildir.a ../numlib/libnumlib.a \
			../rfc822/librfc822.a

testmaildirfilter_SOURCES=maildirfiltertypelist.h testmaildirfilter.c
testmaildirfilter_DEPENDENCIES=libmaildir.a ../numlib/libnumlib.a
testmaildirfilter_LDADD=libmaildir.a ../numlib/libnumlib.a

EXTRA_DIST=README.maildirquota.html README.maildirquota.txt \
	README.maildirfilter.html \
	README.sharedfolders.html README.sharedfolders.txt \
	testsuite testsuite.txt quotawarnmsg $(DOCS)

HTML2TXT=`which lynx 2>/dev/null && echo "lynx -dump -nolist" && exit; echo "links -dump"`

README.maildirquota.txt: README.maildirquota.html
	$(HTML2TXT) README.maildirquota.html >README.maildirquota.txt

README.sharedfolders.txt: README.sharedfolders.html
	$(HTML2TXT) README.sharedfolders.html >README.sharedfolders.txt

mailbot.h: config.status
	echo '#define MAILBOT "@MAILBOT@"' >mailbot.h

maildirsharedrc.h: config.status
	echo '#define MAILDIRSHAREDRC "$(sysconfdir)/maildirshared"' >maildirsharedrc.h

maildirfilterconfig.h: config.status
	echo '#define MAILDIRFILTERCONFIG "$(sysconfdir)/maildirfilterconfig"' >maildirfilterconfig.h

autoresponsequota.h: config.status
	echo '#define AUTORESPONSEQUOTA "$(sysconfdir)/autoresponsesquota"' >autoresponsequota.h

quotawarnmsg.h: config.status
	echo '#define QUOTAWARNMSG "$(sysconfdir)/quotawarnmsg"' >quotawarnmsg.h
maildir.libdeps: config.status
	echo @LIBFAM@ >maildir.libdeps

clean-local:
	rm -rf maildir.libdeps testmd

check:
	@SHELL@ $(srcdir)/testsuite 2>&1 | cmp - $(srcdir)/testsuite.txt

if HAVE_SGML

deliverquota.html.in: deliverquota.sgml ../docbook/sgml2html
	../docbook/sgml2html deliverquota.sgml deliverquota.html.in

deliverquota.8.in: deliverquota.sgml ../docbook/sgml2html
	../docbook/sgml2man deliverquota.sgml deliverquota.8.in

maildirmake.html.in: maildirmake.sgml ../docbook/sgml2html
	../docbook/sgml2html maildirmake.sgml maildirmake.html.in

maildirmake.1.in: maildirmake.sgml ../docbook/sgml2html
	../docbook/sgml2man maildirmake.sgml maildirmake.1.in

maildirquota.html: maildirquota.sgml ../docbook/sgml2html
	../docbook/sgml2html maildirquota.sgml maildirquota.html

maildirquota.7: maildirquota.sgml ../docbook/sgml2man
	../docbook/sgml2man maildirquota.sgml maildirquota.7

maildir.html: maildir.sgml ../docbook/sgml2html
	../docbook/sgml2html maildir.sgml maildir.html

maildir.5: maildir.sgml ../docbook/sgml2man
	../docbook/sgml2man maildir.sgml maildir.5

endif

deliverquota.html: deliverquota.html.in
	CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

deliverquota.8: deliverquota.8.in
	CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

maildirmake.html: maildirmake.html.in
	CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status

maildirmake.1: maildirmake.1.in
	CONFIG_FILES=$@ CONFIG_HEADERS= $(SHELL) ./config.status


# autoresponse.c: autoresponsequota.h

DISTCLEANFILES=$(BUILT_SOURCES)
