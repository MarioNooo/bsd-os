#! /bin/sh
#
# addclient - set up a diskless (network) boot client.

usage() {
	echo "usage: addclient type name-or-ipaddr ..." 1>&2
	echo "(where type is one of: sun4c, sun4m, sun4u)" 1>&2
	exit 1
}

case $# in
0|1) usage;;
esac

# ugly: sun4c boots from xxx.SUN4C or plain xxx; sun4m boots from xxx.SUN4M;
# but sun4u boots from xxx.  So, we have to know which kind of machine,
# and whether to generate an extra, unsuffixed name, in addition to the
# one with the supplied suffix.  (For sun4u, the suffix is empty.)

twonames=false
case "$1" in
4c|sun4c|SUN4C)
	twonames=true; target=netboot; suffix=".SUN4C";;
4m|sun4m|SUN4M)
	target=netboot; suffix=".SUN4M";;
4u|sun4u|SUN4U)
	target=netboot.v9; suffix="";;
*)
	echo "invalid type $1" 1>&2
	usage;;
esac
shift

if [ "`pwd`" != "/tftpboot" ]; then
	cat <<- 'eom'
		Client boots will look in /tftpboot for files.  You are
		not in /tftpboot, so client files added here will probably
		never be seen.  You can abort and `cd /tftpboot' and retry,
		or continue and make the links here anyway.

	eom
	echo -n "Continue? "
	read junk
	case "$junk" in y|Y|yes|Yes|YES) ;; *) exit 1;; esac
fi

status=0

for i
do
	# Check for dotted quad IP address.
	non=`echo "$i" | 
	     sed 's/[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*//'`
	if [ "$non" = "" ]; then
		# Looks like a dotted quad; take it.
		ip=$i
	else
		# Use nslookup to get the address, assuming there is
		# only one address, and hope for the best.  Need two
		# steps to squish out newlines.  There ought to be a
		# flag to just print the IP address...

		ip=`nslookup "$1" 2>/dev/null | grep Address: | tail -1`
		ip=`echo $ip | awk '{print $2}'`
		if [ "$ip" = "" -o "$ip" = 0.0.0.0 ]; then
			echo "can't figure out host $1"
			status=1
			continue
		fi
	fi

	# Convert to uppercase hex.
	x=`echo $ip |
	   awk -F. '{ printf("%02X%02X%02X%02X\n", $1, $2, $3, $4); }'`

	# Make the links.  Note that there is a combined (v8) netboot
	# for sun4c and sun4m, and a separate (v9) netboot for sun4u;
	# it would be nice to combine the v9 one, eventually ... might
	# not be totally impossible (but sure would be tough!)
	if $twonames; then
		ln -s $target $x; ln -s $target $x$suffix
		ls -l $x $x$suffix
	else
		ln -s $target $x$suffix
		ls -l $x$suffix
	fi
done

exit $status
