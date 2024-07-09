#!/bin/sh
#	BSDI MAKEDEV.iopro.sh,v 1.3 1995/10/24 21:16:49 cp Exp
#
#	Create IOPRO control ports in /dev/iopro
#
#	MAKEDEV.iopro,v 1.4 1994/03/30 16:12:30 rjd Exp
#
NCARDS=${NCARDS:-4}
NUNITS=${NUNITS:-4}

MAJOR=29
DATAMAJOR=30



# Tell the user
echo Creating IOPRO control devices for $NCARDS host interface cards supporting
echo $NUNITS external units each.

[ ! -d iopro ] && mkdir iopro


# All devices control port
mknod iopro/all c $MAJOR 192


# Process each host card in turn
card=0
while [ $card -lt $NCARDS ]
do
	# Per host card control port
	mknod iopro/c$card c $MAJOR `expr 128 + 8 '*' $card`

	# Process individual external units
	unit=0
	while [ $unit -lt $NUNITS ]
	do
		# Per unit configuration port
		mknod iopro/c${card}u$unit c \
					$MAJOR `expr 64 + 8 '*' $card + $unit`

		# Per unit download port
		mknod iopro/c${card}u${unit}d c \
					$MAJOR `expr 8 '*' $card + $unit`

		unit=`expr $unit + 1`
	done
	card=`expr $card + 1`
done

# Make the whole lot private to wheel group
chgrp wheel	iopro	iopro/*
chown root	iopro	iopro/*
chmod 660	iopro/*
chmod 775	iopro


#	Now do the data ports
#	=====================

# This is a kludge until I can get the support daemon sorted.
# It should create the ports the first time we see each piece of hardware.

DCARDS=${DCARDS:-1}
DUNITS=${DUNITS:-2}

# Tell the user
echo Creating IOPRO data devices for $DCARDS host interface cards supporting
echo $DUNITS external units each.


minor=0

# Process each host card in turn
card=0
while [ $card -lt $DCARDS ]
do
	# Get the list of unit letters
	case $card in
	    0)	card_name=E ;;
	    1)	card_name=F ;;
	    2)	card_name=G ;;
	    3)	card_name=H ;;
	esac
	unit_list=
	for sub_unit in 0 1 2 3 4 5 6 7
	do
	    unit_list="$unit_list $card_name$sub_unit"
	done

	# Process individual external units
	unit=0
	for unit_id in $unit_list
	do
	    if [ $unit -lt $DUNITS ]
	    then

		# Assume 16 port per unit
		for port_id in a b c d e f g h i j k l m n o p
		do
		    name=tty${unit_id}${port_id}

		    mknod $name c $DATAMAJOR $minor
		    chmod 600 $name
		    chown root $name
		    chgrp wheel $name

		    minor=`expr $minor + 1`
		done
	    fi

	    unit=`expr $unit + 1`
	done


	card=`expr $card + 1`
done
