#!/bin/sh
#
#	ccat.sh	4.1	83/02/11
#
for file in $*
do
	/usr/old/uncompact < $file
done
