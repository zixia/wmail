
# $Id: libauth1.sh,v 1.1.1.1 2003/05/07 02:14:42 lfan Exp $

while read L
do
   REVLIST="$L $REVLIST"
done
echo $REVLIST | tr ' ' '\012'
