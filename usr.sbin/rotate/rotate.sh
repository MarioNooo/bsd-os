#!/bin/sh
#
#	BSDI	rotate.sh,v 1.1 1996/11/12 21:28:36 sanders Exp
#
# rotate -- rotate log files
#
# SECURITY NOTE: Take care using the -s option.
#
cmd="`basename \"$0\"`"
usage="rotate [-c|-z] [-m mode] [-o owner] [-r rot] [-s] file [suffix ...]"
usage() {
    [ $# -gt 0 ] && echo ${cmd}: "$*" 1>&2
    echo Usage: "$usage" 1>&2
}

# protect the file on creation, then fix the modes
umask 022

### Parse arguments.
cflag=""
zflag=""
sflag=""
mode="0644"
owner=""
rot=""
while [ $# -ne 0 ]; do
    case "$1" in
      -c) cflag="1";;
      -m) shift; mode="$1";;
      -o) shift; owner="$1";;
      -r) shift; rot="$1";;
      -s) sflag="1";;
      -z) zflag="1";;
      --) shift; break;;
      -\?) usage; exit 0;;
      -*) usage "illegal option -- $1"; exit 1;;
      *)  break;;
    esac
    shift
done

### Check arguments.
if [ "$zflag" = "1" -a "$cflag" = "1" ]; then
    usage "only one of -c and -z options allowed"
    exit 1
fi

if [ $# -eq 0 ]; then
    usage "file argument required"
    exit 1
fi

### Next remaining argument is the file to rotate.
file="$1"; shift

if [ "X$rot" != "X" ]; then
    if [ $# -ne 0 ]; then
        usage "cannot specify -r and give suffix arguments" 1>&2
        exit 1
    fi
    set -- `jot - "$rot" 0 -1`
fi

if [ $# -eq 0 ]; then
    usage "must specify -r or give suffix arguments" 1>&2
    exit 1
fi

### Rotate the files according to the remaining arguments.
# i    - the "current" suffix
# j    - the "next" suffix (null if last; which gets the real file)
while [ $# -gt 0 ]; do
    i="$1"; shift;

    # Figure out what j should be.
    if [ $# -eq 0 ]; then j=""; else j=".$1"; fi

    # Delete the old file and any compressed counterparts.
    rm -f "$file.$i" "$file.$i.gz" "$file.$i.Z"

    # Move the files around.
    [ -f "$file$j.gz" ] && mv -f "$file$j.gz" "$file.$i.gz"
    [ -f "$file$j.Z" ] && mv -f "$file$j.Z" "$file.$i.Z"
    [ -f "$file$j" ] && mv -f "$file$j" "$file.$i"

    # now handle the final case ($j should be null at this point)
    if [ $# -eq 0 -a -f "$file.$i" ]; then
        # Truncate the log file quickly and set permissions
        cp /dev/null "$file"
        chmod "$mode" "$file"
        if [ "X$owner" != "X" ]; then chown "$owner" "$file"; fi

        # We'll run a $file.scan program on the freshly archived data.
        # This lets you fix up the file permissions and process the data
        # as needed before it's compressed.
        [ "$sflag" = "1" -a -x "$file.scan" ] && $file.scan "$file.$i" "$file"

        # Compress it appropriately.
        if [ "$zflag" = "1" ]; then gzip "$file.$i"; fi
        if [ "$cflag" = "1" ]; then compress "$file.$i"; fi
    fi
done
