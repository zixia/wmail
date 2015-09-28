#
# Copyright 2001-2002 Double Precision, Inc.
# See COPYING for distribution information.
#
# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:13:19 lfan Exp $

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

if HMACC
HMAC=hmac.c
else
HMAC=
endif

noinst_LIBRARIES=libsha1.a

libsha1_a_SOURCES=sha1.c sha1.h sha1_hash.c $(HMAC)

noinst_PROGRAMS=testsuite

testsuite_SOURCES=testsuite.c
testsuite_DEPENDENCIES=libsha1.a
testsuite_LDADD=$(testsuite_DEPENDENCIES)

EXTRA_DIST=testsuite.txt hmac.c

check:
	./testsuite | cmp -s - $(srcdir)/testsuite.txt
