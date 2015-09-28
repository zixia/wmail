
# $Id: libauth2.sh,v 1.1.1.1 2003/05/07 02:14:42 lfan Exp $

while read L
do
	LL=`echo $L | tr '/.-' 'AB_'`

	var='flag=`echo x$flag'$LL'`'
	eval $var
	if test "$flag" = "x"
	then
		var="flag$LL"=1
		eval $var

		case "$L" in
		-*)
			libs="$libs $L"
			;;
		*)
			mods="$mods $L"
			;;
		esac
	fi
done

echo $libs $mods | tr ' ' '\012'
