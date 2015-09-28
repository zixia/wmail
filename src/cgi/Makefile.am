#$Id: Makefile.am,v 1.1.1.1 2003/05/07 02:14:44 lfan Exp $
#
# Copyright 1998 - 2002 Double Precision, Inc.  See COPYING for
# distribution information.

AUTOMAKE = srcdir=${srcdir} @SHELL@ ${srcdir}/../automake.fix @AUTOMAKE@

noinst_LIBRARIES=libcgi.a

libcgi_a_SOURCES=cgi.h cgi.c \
	cgihttpscriptptr.c cgihttpsscriptptr.c cgirelscriptptr.c \
	cginocache.c cgiredirect.c cgiversion.c cgihasversion.c \
	cgicookie.c cgiuseragent.c
