# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:13:12 lfan Exp $
#
# Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

LIBDBOBJSOURCES=bdbobj.h bdbobj.c bdbobj2.c bdbobj3.c
TESTBDBSOURCES=testbdb.C

CLEANFILES=testbdb.dat

if FOUND_DB
noinst_LIBRARIES=libbdbobj.a
noinst_PROGRAMS=testbdb

libbdbobj_a_SOURCES=$(LIBDBOBJSOURCES)
testbdb_SOURCES=$(TESTBDBSOURCES)

testbdb_DEPENDENCIES=libbdbobj.a
testbdb_LDADD=libbdbobj.a @LIBDB@

check:
	$(srcdir)/testsuite | cmp -s - $(srcdir)/testsuite.txt
else
noinst_SCRIPTS=notfound

notfound:
	@exit 0
endif

EXTRA_DIST=$(LIBDBOBJSOURCES) $(TESTBDBSOURCES) testsuite testsuite.txt

