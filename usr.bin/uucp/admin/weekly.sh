#!/bin/sh

# vixie 01dec93 [original]
#
# weekly.sh,v 2.1 1995/02/03 13:16:10 polk Exp

LogDir=/var/log/uucp
Nvers=4

#
# age log files
#

if [ -d $LogDir ]; then
    cd $LogDir
    for subdir in audit uucico uucp uux uuxqt xferstats; do
	if [ -d $subdir ]; then
	    (	cd $subdir
    		for file in *; do
		    case $file in
		    '*') ;;
		    *.[0-9]*) ;;
		    *) /usr/libexec/uuage $file $Nvers ;;
		    esac
    		done
	    )
	fi
    done
fi

#
# should generate some kind of report here
#

exit 0
