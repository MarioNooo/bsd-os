#!/bin/sh

slipunit=$1; shift
speed=$1; shift
login=$1; shift
localaddr=$1; shift
remoteaddr=$1; shift
netmask=$1; shift
opt_args=$*;

/sbin/ifconfig sl${slipunit} inet $localaddr $remoteaddr netmask $netmask up
logger -s -t slip "Attached unit $slipunit for $login (speed: $speed) (args: $opt_args)"
exit 0
