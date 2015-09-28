# $Id: mkmodulelist.sh,v 1.1.1.1 2003/05/07 02:14:43 lfan Exp $
#
# Copyright 2000 Double Precision, Inc.  See COPYING for
# distribution information.

cat <<EOF
#include "config.h"
#include "authstaticlist.h"

EOF

for mod in $* dummy
do
    test $mod = "dummy" || echo "extern struct authstaticinfo $mod;"
done

cat <<EOF

struct authstaticinfo *authstaticmodulelist[]={
EOF

for mod in $* dummy
do
    test $mod = "dummy" || echo "& $mod,"
done

echo "0};"
