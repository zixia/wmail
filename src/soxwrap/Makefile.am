#$Id: Makefile.am,v 1.1.1.1 2003/05/07 02:14:27 lfan Exp $
#
# Copyright 2000-2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

EXTRA_DIST=chkwrap.sh

BUILT_SOURCES=libsock5confdir.h

DISTCLEANFILES=soxlibs.dep
CLEANFILES=libsock5confdir.h

noinst_LIBRARIES=libsoxwrap.a

libsoxwrap_a_SOURCES=soxfuncnames.c soxwrap.c soxwrap.h soxwrapproto.h \
	sconnect.c sconnect.h

noinst_PROGRAMS=testprog

testprog_SOURCES=testprog.c
testprog_DEPENDENCIES=libsoxwrap.a config.status
testprog_LDADD=libsoxwrap.a @SOCKLIBS@ `cat soxlibs.dep`

libsock5confdir.h: config.status
	test "@libconffile@" != "" && \
		echo "CHECK(\"@libconffile@\")" >libsock5confdir.h && exit 0 ; \
		echo "CHECK(\"/etc/@CONFNAME@\")" >libsock5confdir.h ; \
		echo "CHECK(\"/usr/local/etc/@CONFNAME@\")" >>libsock5confdir.h ; \
		etcdir=$(sysconfdir) ; \
		test $$etcdir = "/etc" && exit 0; \
		test $$etcdir = "/usr/local/etc" && exit 0; \
		echo "CHECK(\"$$etcdir/@CONFNAME@\")" >>libsock5confdir.h

check:
	./testprog
