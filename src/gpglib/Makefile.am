#
# Copyright 2001-2002 Double Precision, Inc.  See COPYING for
# distribution information.
#
# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:14:54 lfan Exp $

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

CLEANFILES=gpg.h

noinst_SCRIPTS=webgpg

noinst_LIBRARIES=libgpg.a
noinst_PROGRAMS=testgpg mimegpg

libgpg_a_SOURCES=libgpg.c gpglib.h export.c fork.c genkey.c import.c \
		list.c delete.c mimegpgfork.c mimegpgfork.h \
		sign.c checksign.c rfc2045.c

testgpg_SOURCES=testgpg.c
testgpg_LDADD=libgpg.a ../unicode/libunicode.a ../numlib/libnumlib.a
testgpg_DEPENDENCIES=$(testgpg_LDADD)

mimegpg_SOURCES=mimegpg.c mimegpgheader.c mimegpgheader.h \
		mimegpgstack.c mimegpgstack.h \
		tempname.c tempname.h

mimegpg_LDADD=libgpg.a
mimegpg_DEPENDENCIES=libgpg.a
BUILT_SOURCES=gpg.h mimegpg.html mimegpg.1
EXTRA_DIST=mimegpg.html mimegpg.1 README.html

if HAVE_SGML

mimegpg.html: mimegpg.sgml ../docbook/sgml2html
	../docbook/sgml2html mimegpg.sgml mimegpg.html

mimegpg.1: mimegpg.sgml ../docbook/sgml2man
	../docbook/sgml2man mimegpg.sgml mimegpg.1

endif

gpg.h: config.status
	echo '#define GPG "@GPG@"' >gpg.h
