#!/bin/sh
#	BSDI Dialin.login,v 2.7 1997/01/16 18:12:54 prb Exp
PATH=/sbin:/usr/sbin:/bin:/usr/bin
DEBUGFILE=/var/log/ppp.debug

while [ $# -gt 0 ] ; do
	case $1 in
	-debug)	shift; exec >> $DEBUGFILE 2>&1
		echo $(date): $0 $* ; set -x ;;
        --)	shift; break ;;
        -*|+*)	shift ;;
        *)	break ;;
        esac
done

SYSTEM=$1
INTERFACE=$2

#
# Look for an address associated with this system.
# If no address is not found, then don't worry about
# configing the interface
#
LOCAL=
REMOTE=
checkf() {
	REMOTE=$(awk -v S=$1   '$1 == S { $1 = "" ; print ; exit 0; }' < $2)
}

if [ -r /etc/netscripts/addr-map ] ; then
	LOCAL=$(awk '$1 == "LOCAL" { $1 = "" ; print ; exit 0; }' \
		< /etc/netscripts/addr-map)
	if [ X"$REMOTE" = X ] ; then
		checkf $SYSTEM /etc/netscripts/addr-map
	fi
	if [ X"$REMOTE" = X ] ; then
		checkf $INTERFACE /etc/netscripts/addr-map
	fi
	if [ X"$REMOTE" = X ] ; then
		checkf $(tty) /etc/netscripts/addr-map
	fi
fi
if [ X"$LOCAL" = X ] ; then
	LOCAL=$(hostname)
fi

#
# SLIP interfaces require that the interface be configured at this point.
# If we don't have a REMOTE address, cause a failure exit so ppp(8)
# will not continue trying to set up the session.
# PPP does not require the interface to be configured yet as it may
# be getting its address from the remote end.
#
if [ X"$REMOTE" = X ] ; then
	case $INTERFACE in
	sl*)	exit 1 ;;
	*)	exit 0 ;;
	esac
fi

eval set - $REMOTE

REMOTE=$1
shift
if [ $REMOTE = DNS ] ; then
    REMOTE=$(/usr/libexec/ppputil $(echo $SYSTEM | sed -e 's/.//'))
    if [ -z $REMOTE ] ; then
	echo "Failed to find IP address for $SYSTEM" >&2
	exit 1
    fi
fi
for i in x x x; do
	route -n delete -host $REMOTE > /dev/null 2>&1
	ifconfig $INTERFACE inet set $LOCAL destination $REMOTE $* && exit 0
done
ifconfig $INTERFACE inet -remove down > /dev/null 2>&1
exit 1
