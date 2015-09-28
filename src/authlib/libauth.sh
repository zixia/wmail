# $Id: libauth.sh,v 1.1.1.1 2003/05/07 02:14:42 lfan Exp $

SHELL="$1"
srcdir="$2"

tr ' ' '\012' | $SHELL ${srcdir}/libauth1.sh | $SHELL ${srcdir}/libauth2.sh \
	| $SHELL ${srcdir}/libauth1.sh

