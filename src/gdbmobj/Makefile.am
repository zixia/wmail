# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:13:08 lfan Exp $
#
# Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

LIBGDBMSOURCES=gdbmobj.h gdbmobj.c gdbmobj2.c gdbmobj3.c
TESTGDBMSOURCES=testgdbm.C

CLEANFILES=testgdbm.dat

if FOUND_GDBM
noinst_LIBRARIES=libgdbmobj.a
noinst_PROGRAMS=testgdbm

libgdbmobj_a_SOURCES=$(LIBGDBMSOURCES)
testgdbm_SOURCES=$(TESTGDBMSOURCES)

testgdbm_DEPENDENCIES=libgdbmobj.a
testgdbm_LDADD=libgdbmobj.a @LIBGDBM@

check:
	$(srcdir)/testsuite | cmp -s - $(srcdir)/testsuite.txt

else
noinst_SCRIPTS=notfound

notfound:
	@exit 0

endif

EXTRA_DIST=$(LIBGDBMSOURCES) $(TESTGDBMSOURCES) testsuite testsuite.txt
