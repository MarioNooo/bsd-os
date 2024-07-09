#!/bin/sh
#	BSDI Demand.login,v 2.5 1998/03/18 16:34:17 chrisk Exp
PATH=/sbin:/usr/sbin:/bin:/usr/bin

# ifconfig the interface and set up any routes.

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
    RADDR=$(/usr/libexec/ppputil $REMOTE)
    if ! route -n get -host $RADDR 2> /dev/null | \
	    awk -v D=$RADDR -v I=$INTERFACE \
		'$1 == "destination:" && $2 != D { exit 1; } \
		$1 == "interface:" && $2 != I { exit 1; }'; then
	route -n delete -host $RADDR > /dev/null 2>&1
    fi
    LADDR=$(/usr/libexec/ppputil $LOCAL)
    if ! ifconfig $INTERFACE inet | \
	    awk -v L=$LADDR -v R=$RADDR \
		'$1 == "inet" && $3 == "->" && $2 != L && $4 != R { exit 1; } \
		END { if ( NF == 0 ) exit 1; }'; then
	ifconfig $INTERFACE inet set $LADDR destination $RADDR $* || exit 1
    else
	ifconfig $INTERFACE up $* || exit 1
    fi
#
# If there is no default route then add one through this interface
# Comment out these lines if you do not want a default route defined
#
    route -qn get default || route -n add default $REMOTE
fi
case $INTERFACE in
sl*)	exit 1 ;;
*)	exit 0 ;;
esac
