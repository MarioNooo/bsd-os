#!/bin/sh -
[ $# != 0 ] && {
	echo 'usage: hostid'
	exit 1
}
/sbin/sysctl -n kern.hostid
