#!/bin/sh
#	BSDI Dialout.login,v 2.9 1997/04/23 15:51:28 prb Exp
PATH=/sbin:/usr/sbin:/bin:/usr/bin

while [ $# -gt 0 ] ; do
	case $1 in
	-debug)	shift; echo $(date): $0 $* ; set -x ;;
        --)	shift; break ;;
        -*|+*)	shift ;;
        *)	break ;;
        esac
done

SYSTEM=$1
INTERFACE=$2

LOCAL=
REMOTE=

if [ -r /etc/netscripts/addr-map ] ; then
	LOCAL=$(awk '$1 == "LOCAL" { $1 = "" ; print ; exit 0; }' \
		< /etc/netscripts/addr-map)
	if [ X"$LOCAL" = X ] ; then
		LOCAL=$(hostname)
	fi
	REMOTE=$(awk -v S=$SYSTEM '$1 == S { $1 = "" ; print ; exit 0; }' \
		< /etc/netscripts/addr-map)
	if [ X"$REMOTE" = X ] ; then
		case $INTERFACE in
		sl*)	exit 1 ;;
		*)	exit 0 ;;
		esac
	fi
	eval set -- $REMOTE
	REMOTE=$1
	shift
	if [ $REMOTE = DNS ] ; then
	    REMOTE=$(/usr/libexec/ppputil $(echo $SYSTEM | sed -e 's/.//'))
	    if [ -z $REMOTE ] ; then
		echo "Failed to find IP address for $SYSTEM" >&2
		exit 1
	    fi
	fi
	route -n delete -host $REMOTE > /dev/null 2>&1
	echo ifconfig $INTERFACE inet set $LOCAL destination $REMOTE $*
	ifconfig $INTERFACE inet set $LOCAL destination $REMOTE $* && exit 0
fi
case $INTERFACE in
sl*)	exit 1 ;;
*)	exit 0 ;;
esac
