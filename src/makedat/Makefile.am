# $Id: Makefile.am,v 1.1.1.1 2003/05/07 02:13:57 lfan Exp $
#
# Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

noinst_PROGRAMS=makedatprog

makedatprog_SOURCES=makedatprog.c
makedatprog_DEPENDENCIES=@dblibrary@
makedatprog_LDADD=@dblibrary@ @LIBGDBM@ @LIBDB@
