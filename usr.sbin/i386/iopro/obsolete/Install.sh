#!/bin/sh
#
#	Stick all the IOPRO driver bits in the right places and patch some of
#	the System configuration files to fit.
#
#	WARNING: I strongly suggest to read the script and carry the actions
#		 out manually.
#		 THIS SCRIPT HAS NOT BEEN TESTED ON A VIRGIN SYSTEM
#
#	This script should work with both version 1.0 and 1.1 kernels
#	Now (16 May 95) updated for 2.0 systems (I have not retro tested on 1.*
#	systems)
#
#	Install.sh,v 1.13 1995/07/28 10:42:03 rjd Exp
#

# Where the kernel source resides
KERN=/usr/src/sys
# Where system command sources reside
SBIN=/usr/src/usr.sbin
# Where secondary (not called directly by user) command sources reside
LEXEC=/usr/src/libexec

#Get the OS version as we now need to make changes based on version
OSVER=`uname -r`

# First a quick bug fix to 1.0 sys/kern/tty.c
# Two minor bugs.
#   The first is that the use DCD for output flow control flag MDMBUF was being
#     checked in the wrong field of the modem structure.
#   The second was that a VSTART character was always emitted when the input
#     buffer was drained even with software flow control turned off.
#

# Not available with 1.1 lets hope they fixed the bugs.
if [ -f $KERN/kern/tty.c -a $OSVER = "1.0" ]
then
    echo Patching $KERN/kern/tty.c
    patch -d $KERN <<'EOF'
Prereq:	1.15
*** /cdrom/usr/src/sys/kern/tty.c	Tue Mar  2 00:14:36 1993
--- kern/tty.c	Sun Jun 20 10:21:32 1993
***************
*** 258,264 ****
  		    && putc(tp->t_cc[VSTOP], &tp->t_outq) == 0) {
  			tp->t_state |= TS_TBLOCK;
  			ttstart(tp);
! 		};
  		if (tp->t_cflag&CRTS_IFLOW &&
  		    (*cdevsw[major(tp->t_dev)].d_ioctl)
  			(tp->t_dev,TIOCMBIC,(caddr_t)&rts,0,(struct proc *)NULL) == 0)
--- 258,264 ----
  		    && putc(tp->t_cc[VSTOP], &tp->t_outq) == 0) {
  			tp->t_state |= TS_TBLOCK;
  			ttstart(tp);
! 		}
  		if (tp->t_cflag&CRTS_IFLOW &&
  		    (*cdevsw[major(tp->t_dev)].d_ioctl)
  			(tp->t_dev,TIOCMBIC,(caddr_t)&rts,0,(struct proc *)NULL) == 0)
***************
*** 726,732 ****
  	register struct tty *tp;
  {
  
! 	if ((tp->t_state&TS_WOPEN) == 0 && (tp->t_lflag&MDMBUF)) {
  		/*
  		 * MDMBUF: do flow control according to carrier flag
  		 */
--- 726,732 ----
  	register struct tty *tp;
  {
  
! 	if ((tp->t_state&TS_WOPEN) == 0 && (tp->t_cflag&MDMBUF)) {
  		/*
  		 * MDMBUF: do flow control according to carrier flag
  		 */
***************
*** 1321,1327 ****
  	 * the input queue has gone down.
  	 */
  	if (tp->t_state&TS_TBLOCK && tp->t_rawq.c_cc < TTYHOG/5) {
! 		if (cc[VSTART] != _POSIX_VDISABLE &&
  		    putc(cc[VSTART], &tp->t_outq) == 0) {
  			tp->t_state &= ~TS_TBLOCK;
  			ttstart(tp);
--- 1321,1327 ----
  	 * the input queue has gone down.
  	 */
  	if (tp->t_state&TS_TBLOCK && tp->t_rawq.c_cc < TTYHOG/5) {
! 		if (tp->t_iflag&IXOFF && cc[VSTART] != _POSIX_VDISABLE &&
  		    putc(cc[VSTART], &tp->t_outq) == 0) {
  			tp->t_state &= ~TS_TBLOCK;
  			ttstart(tp);
EOF
fi


# Create any new directories
for d in $KERN/i386/isa/fw $LEXEC/iopro_dload $LEXEC/iopro_dload/firmware \
	 $LEXEC/iopro_wait $LEXEC/ioprod $SBIN/iopro_debug $SBIN/iopro_dump
do
    if [ ! -d $d ]
    then
	echo Adding directory $d
	mkdir -p $d
    fi
done

# Now add our files to the right places.
# I don't put the debug enabling command anywhere on the grounds it shouldn't
# be needed :-)
# XXX It's a pet hate that install does not install missing sub directories or
#     have a file name echo option.
echo Installing files
set -x
install -c -m 755 utils/MAKEDEV.iopro			/dev
install -c -m 444 dd/aim_channel.h			$KERN/i386/isa/fw
install -c -m 444 dd/aim_master.h			$KERN/i386/isa/fw
install -c -m 644 dd/aimctl.c				$KERN/i386/isa
install -c -m 644 dd/aimdata.c				$KERN/i386/isa
install -c -m 644 dd/aimioctl.h				$KERN/i386/isa
install -c -m 644 dd/aimioctl.h				/usr/include/sys
install -c -m 644 dd/aimreg.h				$KERN/i386/isa
install -c -m 644 dd/aimvar.h				$KERN/i386/isa
#install -c -m 644 chase.h				$KERN/i386/isa
install -c -m 644 utils/iopro_dload/Makefile		$LEXEC/iopro_dload
install -c -m 644 utils/iopro_dload/iopro_dload.c	$LEXEC/iopro_dload
install -c -m 644 utils/iopro_dload/iopro_dload.8	$LEXEC/iopro_dload
install -c -m 644 utils/iopro_dload/firmware/iopro.dl	$LEXEC/iopro_dload/firmware
install -c -m 644 utils/iopro_wait/Makefile		$LEXEC/iopro_wait
install -c -m 644 utils/iopro_wait/iopro_wait.c		$LEXEC/iopro_wait
install -c -m 644 utils/iopro_wait/iopro_wait.8		$LEXEC/iopro_wait
install -c -m 644 ioprod/Makefile			$LEXEC/ioprod
install -c -m 644 ioprod/ioprod.pl			$LEXEC/ioprod
install -c -m 644 ioprod/ioprod.8			$LEXEC/ioprod
install -c -m 644 utils/iopro_debug/iopro_debug.c	$SBIN/iopro_debug
install -c -m 644 utils/iopro_debug/Makefile		$SBIN/iopro_debug
install -c -m 644 utils/iopro_debug/iopro_debug.8	$SBIN/iopro_debug
install -c -m 644 utils/iopro_dump/Makefile		$SBIN/iopro_dump
install -c -m 644 utils/iopro_dump/iopro_dump.c		$SBIN/iopro_dump
install -c -m 644 utils/iopro_dump/iopro_dump.8		$SBIN/iopro_dump
set +x

## Create the perl header files
#echo Creating perl headers
#h2ph sys/aimioctl.h

# Add our files to the kernel config list
CFG_LST=$KERN/i386/conf/files.i386
IN_KERN=FALSE
if grep -s aimctl $CFG_LST > /dev/null
then
    : Entry already exists in file
    IN_KERN=TRUE
else
    echo Updating $CFG_LST
    case "$OSVER" in
    1.*)
	echo "i386/isa/aimctl.c	optional aimc device-driver" >> $CFG_LST
	echo "i386/isa/aimdata.c	optional aim device-driver" >> $CFG_LST
	;;
    2.0)
	cat <<'EOF' >> $CFG_LST

# Chase Research IOPRO/IOLITE serial comms
pseudo-device	aim
device	aimc at isa
file	i386/isa/aimdata.c	aim | aimc device-driver always-source needs-count
file	i386/isa/aimctl.c	aimc | aim device-driver always-source needs-flag
EOF
	;;
    2.*)
	: After 2.0 we are in the kernel as standard
	IN_KERN=TRUE
	;;
    *)
	echo "OPPS! unknown OS version $OSVER"
	echo "Cannot update configuration files, giving up"
	exit 1
	;;
    esac
fi


# Create our kernel config file
CONF=$KERN/i386/conf
# Try to be compatable for people who added drivers before we became part of the
# standard kernel
if [ $IN_KERN = TRUE ]
then
    if [ -f $CONF/IOPRO ]
    then
	DST=IOPRO
    else
	DST=GENERIC
    fi
else
    DST=IOPRO
fi

if [ -f $CONF/$DST ]
then
    : Config file already exists
else
    if [ -f $CONF/LOCAL ]
    then
	SRC=$CONF/LOCAL
    else
	SRC=$CONF/GENERIC
    fi

    echo Creating $CONF/$DST from $SRC

    case "$OSVER" in
    1.*)
	DPREF="device	"
	;;
    2.*)
	DPREF=""
	;;
    esac

    cp $SRC $CONF/$DST
    echo "" >> $CONF/$DST
    echo "# Chase Research IOPRO control driver" >> $CONF/$DST
    echo "# Memory can be any 2K boundary that does not conflict" >> $CONF/$DST
    for IO in 180 190 200 240 280 2E0 300 100
    do
	echo "${DPREF}aimc0 at isa? port 0x$IO iomem 0xCC000 iosiz 2048" >> $CONF/$DST
    done
    echo "# EISA variants" >> $CONF/$DST
    for IO in 1 2 3 4 5 6 7 8 9 A B C D E F
    do
	echo "${DPREF}aimc0 at isa? port 0x${IO}C00" >> $CONF/$DST
    done
    echo "" >> $CONF/$DST
    echo "# Chase Research IOPRO data driver" >> $CONF/$DST
    echo "pseudo-device	aim 64" >> $CONF/$DST

    # Change the ident if we ain't generic
    if [ $SRC = $CONF/LOCAL ]
    then
	# Lose any error as it ain't important
    	ed -s $CONF/$DST <<'EOF' > /dev/null 2>&1
/^ident/s/LOCAL/IOPRO/
w
q
EOF
    fi
fi


#Add our device switch entry to the right places
case "$OSVER" in
1.0)
    echo Patching $KERN/i386/i386/conf.c
    patch -d $KERN <<'EOF'
Prereq:	1.17
*** /cdrom/usr/src/sys/i386/i386/conf.c	Thu Mar 11 17:22:07 1993
--- i386/i386/conf.c	Tue Jun 29 13:26:58 1993
***************
*** 329,334 ****
--- 329,347 ----
  #include "ms.h"
  cdev_decl(ms);
  
+ #include "aimc.h"
+ cdev_decl(aimc);
+ 
+ /* open, close, read, write, ioctl */
+ #define cdev_aimc_init(c,n) { \
+ 	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
+ 	dev_init(c,n,write), dev_init(c,n,ioctl), \
+ 	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, 0, \
+ 	seltrue, (dev_type_map((*))) enodev, 0 }
+ 
+ #include "aim.h"
+ cdev_decl(aim);
+ 
  struct cdevsw	cdevsw[] =
  {
  	cdev_cn_init(1,cn),		/* 0: virtual console */
***************
*** 357,362 ****
--- 370,377 ----
  	cdev_disk_init(NMCD,mcd),       /* 23: Mitsumi CD-ROM */
  	cdev_tty_init(NMS,ms),		/* 24: Maxpeed Async Mux */
  	cdev_lms_init(NLMS,lms),	/* 25: Logitec Bus Mouse */
+ 	cdev_aimc_init(NAIMC,aimc),	/* 26: Chase Research IOPRO control */
+ 	cdev_tty_init(NAIM,aim),	/* 27: Chase Research IOPRO data */
  };
  
  int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);
EOF
    ;;
1.1)
    echo Patching $KERN/i386/i386/conf.c
    patch -d $KERN <<'EOF'
Prereq:	1.22
*** /cdrom/usr/src/sys/i386/i386/conf.c	Mon Feb  7 15:41:12 1994
--- i386/i386/conf.c	Wed Mar 30 13:22:55 1994
***************
*** 353,358 ****
--- 353,371 ----
  #include "si.h"
  cdev_decl(si);
  
+ #include "aimc.h"
+ cdev_decl(aimc);
+ 
+ /* open, close, read, write, ioctl */
+ #define cdev_aimc_init(c,n) { \
+ 	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), \
+ 	dev_init(c,n,write), dev_init(c,n,ioctl), \
+ 	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, 0, \
+ 	seltrue, (dev_type_map((*))) enodev, 0 }
+ 
+ #include "aim.h"
+ cdev_decl(aim);
+ 
  struct cdevsw	cdevsw[] =
  {
  	cdev_cn_init(1,cn),		/* 0: virtual console */
***************
*** 384,389 ****
--- 397,404 ----
  	cdev_tty_init(NDIGI,digi),	/* 26: DigiBoard PC/X[ei] */
  	cdev_tty_init(NSI,si),		/* 27: Specialix multiplexor */
  	cdev_sb_init(NSB,sb),		/* 28: SoundBlaster Pro */
+ 	cdev_aimc_init(NAIMC,aimc),	/* 29: Chase Research IOPRO control */
+ 	cdev_tty_init(NAIM,aim),	/* 30: Chase Research IOPRO data */
  };
  
  int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);
EOF
    ;;
2.0)
    # 2.0 (and later I hope :-) have an easier device switch file
    CFG_SW=$KERN/i386/conf/ioconf.c.i386
    if grep -s IOPRO $CFG_SW > /dev/null
    then
	: Entries already exist
    else
	echo "Adding device switch entries to $CFG_SW"
	ed -s $CFG_SW <<'EOF'
/^};/i
	%DEVSW(aimc),		/* 29 = Chase IOPRO control driver */
	%DEVSW(aim),		/* 30 = Chase IOPRO data driver */
.
w
q
EOF
    fi
    ;;
2.*)
    : From here on we are already in the kernel
    ;;
esac

# Now setup the ioprod startup in /etc/rc.local
RCFILE=/etc/rc.local
if grep -s ioprod $RCFILE > /dev/null
then
	: Entry already exists in file
else
    	echo "Adding ioprod startup to $RCFILE"
    	ed -s $RCFILE <<'EOF'
/daemons:/a

# Start Chase IOPRO download and configuration daemon
if [ -c /dev/iopro/all ]
then
    # Lose error message from head if we are running the wrong kernel
    cards=`head -1 /dev/iopro/all 2> /dev/null`
    if [ ! "X$cards" = "X" ]
    then
	while [ $cards -gt 0 ]
	do
	    cards=`expr $cards - 1`
	    echo -n " ioprod";	/usr/libexec/ioprod $cards
	done
    fi
fi

.
w
q
EOF
fi

# Finally tell the user what to do next
echo "Now you need to rebuild everything and install it."
echo
echo "	cd $KERN/i386/conf ; config $DST"
echo "	cd $KERN/compile/$DST ; make depend ; make"
echo "	mv /bsd /bsd.old	#I'm nervous even if you ain't"
echo "	mv bsd /bsd"
echo "	cd $LEXEC/iopro_dload ; make ; make install"
echo "	cd $LEXEC/iopro_wait ; make ; make install"
echo "	cd $LEXEC/ioprod ; make ; make install"
echo "	cd /dev ; ./MAKEDEV.iopro"
echo "	reboot"
echo
echo -n "Do you wish to run this sequence now ? (y/n) "
read yn junk
if [ X$yn = Xy ]
then
    echo Running sequence will stop at first error
    set -x -e
    cd $KERN/i386/conf ; config $DST
    cd $KERN/compile/$DST ; make depend ; make
    if [ ! -f /bsd.old ]
    then
	mv /bsd /bsd.old
    fi
    mv bsd /bsd
    cd $LEXEC/iopro_dload ; make ; make install
    cd $LEXEC/iopro_wait ; make ; make install
    cd $LEXEC/ioprod ; make ; make install
    cd /dev ; ./MAKEDEV.iopro
    set +x +e
    echo
    echo -n "Do you wish to build the IOPRO debug utilities ? (y/n) "
    read yn junk
    if [ X$yn = Xy ]
    then
	set -x -e
	cd $SBIN/iopro_debug ; make ; make install
	cd $SBIN/iopro_dump ; make ; make install
	set +x +e
    fi

    # Create a log file for daemon debug output
    if [ ! -f /var/log/ioprod_0 ]
    then
	echo
	echo -n "Do you wish to create a IOPRO daemon log file ? (y/n) "
	read yn junk 
	if [ X$yn = Xy ]
	then
	    touch /var/log/ioprod_0
	    chmod 666 /var/log/ioprod_0
	fi
    fi

    echo
    echo "You now need to reboot the system."
fi

exit 0
