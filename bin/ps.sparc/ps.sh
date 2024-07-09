#! /bin/sh
arch=`arch`
case $arch in
sparc) suf=v8;;
sparc_v9) suf=v9;;
*) echo "unknown architecture $arch" 1>&2; exit 1;;
esac
exec $0.$suf ${1+"$@"}
