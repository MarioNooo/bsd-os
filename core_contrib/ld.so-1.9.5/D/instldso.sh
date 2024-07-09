#!/bin/sh

umask 022

force=false
devfiles=false
STRIP=-s

for f in $* ; do
    case $f in
	--force) force=true ;;
	--devfiles) devfiles=true ;;
	--no-strip) STRIP= ;;
	*) echo $0: invalid option $f
	    echo "usage: $0 [--force] [--devfiles] [ --no-strip]"
	    exit 1
	    ;;
    esac
done

if [ $force != true ] ; then
    cat <<EOF
Please see the IMPORTANT NOTICES sections in the README file
and then rerun instldso.sh with the --force option.
EOF
    exit 1
fi

echo 'Expect some cache has wrong version warnings if upgrading'
echo 'from a version prior to 1.7.0.'

. ./Version.mk

if [ -z "$PREFIX" ]; then
       PREFIX=
fi

install -d -m 755 $PREFIX/etc
install -d -m 755 $PREFIX/sbin
install -d -m 755 $PREFIX/lib
install -d -m 755 $PREFIX/usr/bin
install -d -m 755 $PREFIX/usr/lib
install -d -m 755 $PREFIX/usr/man/man1
install -d -m 755 $PREFIX/usr/man/man8
install -d -m 755 $PREFIX/usr/info
if [ $devfiles = true ] ; then
    install -d -m 755 $PREFIX/usr/include
    install -d -m 755 $PREFIX/usr/man/man3
fi

#if [ -f /etc/ld.so.cache ] ; then
#	echo Deleting old /etc/ld.so.cache
#	rm -f /etc/ld.so.cache
#fi

if [ ! -f $PREFIX/etc/ld.so.conf ] ; then
	echo Creating new /etc/ld.so.conf
	for dir in /usr/local/lib /usr/X11R6/lib /usr/openwin/lib ; do
		if [ -d $PREFIX$dir ] ; then
			echo $dir >> $PREFIX/etc/ld.so.conf
		fi
	done
fi

if [ -f ld-so/ld.so ] ; then
	echo Installing ld.so
	install $STRIP ld-so/ld.so $PREFIX/lib/ld.so.$VERSION
	mv -f $PREFIX/lib/ld.so.$VERSION $PREFIX/lib/ld.so
	ln -f $PREFIX/lib/ld.so $PREFIX/lib/ld.so.$VERSION
else
	echo Not installing a.out support
fi

echo Installing ld-linux.so
install d-link/ld-linux.so $PREFIX/lib/ld-linux.so.$VERSION
if [ -n "$STRIP" ] ; then
    strip -g -K _dl_debug_state $PREFIX/lib/ld-linux.so.$VERSION
fi
ln -sf ld-linux.so.$VERSION $PREFIX/lib/ld-linux.so.$VMAJOR

echo Installing libdl.so
install $STRIP d-link/libdl/libdl.so $PREFIX/lib/libdl.so.$VERSION
if [ $devfiles = true ] ; then
    ln -sf /lib/libdl.so.$VERSION $PREFIX/usr/lib/libdl.so
    install -m 644 d-link/libdl/dlfcn.h $PREFIX/usr/include/dlfcn.h
fi

echo Installing ldd
install $STRIP util/ldd $PREFIX/usr/bin
echo Installing lddstub
install $STRIP util/lddstub $PREFIX/usr/lib/lddstub

echo Installing and running ldconfig
install $STRIP util/ldconfig $PREFIX/sbin
$PREFIX/sbin/ldconfig

echo Installing manual and info pages
install -m 644 man/ldd.1 $PREFIX/usr/man/man1
install -m 644 man/ldconfig.8 man/ld.so.8 $PREFIX/usr/man/man8
install -m 644 man/ld.so.info $PREFIX/usr/info
if [ $devfiles = true ] ; then
    install -m 644 man/dlopen.3 $PREFIX/usr/man/man3
    ln -sf dlopen.3 $PREFIX/usr/man/man3/dlsym.3
    ln -sf dlopen.3 $PREFIX/usr/man/man3/dlerror.3
    ln -sf dlopen.3 $PREFIX/usr/man/man3/dlclose.3
fi

echo Installation complete
