#! /bin/sh
#	BSDI mathlink.i386.sh,v 1.5 1999/09/19 20:37:54 polk Exp

#
# Mathlink matches the system's math libraries to
# the system's capability for hardware floating point.
#

PATH=/sbin:/usr/sbin:/bin:/usr/bin

DESTDIR=
HFP=unknown

#
# Parse options.
#
while [ $# -gt 0 ]
do
	case "$1" in
		-h)	# force i386 library
			HFP=yes
			shift
			;;
		-r)	# root of hierarchy
			DESTDIR=$2
			shift 2
			;;
		-s)	# force std library
			HFP=no
			shift
			;;
		*)
			echo mathlink: unrecognized arguments: "$@"
			echo mathlink: usage: mathlink '[-hs]' '[-r root]'
			exit 0
			;;
	esac
done

#
# Figure out the paths and suffixes of the libraries.
#
SLIB=$DESTDIR$(
	(echo 'MAP() { [ X$1 = X-lm ] && echo $5; }';
	    cat $DESTDIR/etc/shlib.map) |
		sh
)

#
# Generate the names of the std and i386 math libraries.
# If one of them fails to exist, then we can't change the link.
#
SSTD=$(echo $SLIB | sed -e 's/libm/libmstd/')
SMACH=$(echo $SLIB | sed -e 's/libm/libmi386/')
[ ! -f $SSTD -o ! -f $SMACH ] && exit 0

#
# Which library is current?
# XXX Assumes that libm_s.* is a hard link (and hence not on /usr).
#
CURLIB=unknown
if [ -f $SLIB ]
then
	ILIB=$(ls -i $SLIB | awk '{ print $1 }')
	IMACH=$(ls -i $SMACH | awk '{ print $1 }')
	ISTD=$(ls -i $SSTD | awk '{ print $1 }')
	[ $ILIB -eq $IMACH ] && CURLIB=mach
	[ $ILIB -eq $ISTD ] && CURLIB=std
fi

#
# Do we have hardware floating point?
#
if [ $HFP = unknown ]
then
	HFP=no
	DMESG=$(dmesg | egrep '^(real mem =|npx0) ' | tail -1)
	[ $(expr "$DMESG" : npx0) -eq 4 ] && HFP=yes
fi

#
# Do we already have the correct state?
# If so, we don't need to create hard links.
#
HARDLINK=yes
[ $CURLIB = std ] && [ $HFP = no ] && HARDLINK=no
[ $CURLIB = mach ] && [ $HFP = yes ] && HARDLINK=no

#
# Build the remaining filenames.
#
ALIB=$DESTDIR$(
	(echo 'MAP() { [ X$1 = X-lm ] && echo $4; }';
	    cat $DESTDIR/etc/shlib.map) |
		sh
)

# Try to pick the numerically greatest version of the dynamic libraries.
DLIB=$(echo $SLIB | sed -e 's/_s\..*$/.so/')
DVERS=$(ls $(echo $DLIB | sed -e 's/libm/libmstd/')* |
	sort -bnr -t. -k 1,1 -k 2,2 -k 3,3 |
	sed -e 's;^.*/libmstd\.so;;' -e q)
DLIB=$DLIB$DVERS

PLIB=$(dirname $ALIB)/$(basename $ALIB .a)_p.a
STUB=$(dirname $ALIB)/$(basename $SLIB).a
EXCEPT=$(dirname $ALIB)/libm.except

#
# Install the links.
#
for GENERIC in $SLIB $STUB $EXCEPT $DLIB $ALIB $PLIB 
do
	if [ $HFP = yes ]
	then
		SPECIFIC=$(echo $GENERIC | sed -e 's/libm/libmi386/')
	else
		SPECIFIC=$(echo $GENERIC | sed -e 's/libm/libmstd/')
	fi
	if [ $(expr $GENERIC : $DESTDIR/usr) -gt 0 ]
	then
		# Permit /usr to be read-only...
		# $GENERIC is already a symlink to /var/run.
		[ -n "$DESTDIR" ] &&
		    SPECIFIC=$(expr $SPECIFIC : "$DESTDIR\(.*\)")
		GENERIC=$DESTDIR/var/run/$(basename $GENERIC)
		ln -sf $SPECIFIC $GENERIC
	else
		[ $HARDLINK = yes ] && ln -f $SPECIFIC $GENERIC
	fi
done

# We need to update ld.so.cache but we don't want to update library links
# because the sysadmin may have altered links other than the math library link.
# Thus we create links ourselves, and run ldconfig -X to update ld.so.cache.
DLDIR=$(dirname $DLIB)
SONAME=$(objdump -p $DLIB | awk '$1 == "SONAME" { print $2 }')
ln -sf $DLIB $DLDIR/$SONAME
[ $SONAME = libm.so ] || ln -sf $DLDIR/$SONAME $DLDIR/libm.so
ldconfig -X /shlib

exit 0
