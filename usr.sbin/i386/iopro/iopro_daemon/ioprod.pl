#!/usr/contrib/bin/perl
#
#	Chase IOPRO support daemon
#
#	R.J.Dunlop	18 Jul 93
#
#	This daemon is envoked from the system startup scripts and is
#	responsible for monitoring the power status of one of the Chase
#	Research IOPRO host interface cards.
#	When power good is detected the external units are downloaded and
#	configured and the corresponding data ports in /dev are assigned.
#
#	This is a first cut version and relies on the default powerup
#	configuration of the units as supplied for the DOS environment.
#
#	WARNING: The operation of this daemon is nothing like the "standard"
#		 Chase support daemon. Who needs silly menus anyway.
#
#	WARNING 2: This is my first ever perl script, expect sickness.
#
#	WARNING 3: The logging performed is probably far to verbose but it
#		   helps debuging.
#
#	ioprod.pl,v 1.5 1995/07/28 08:59:46 rjd Exp
#

# Constants from aimioctl.h
# XXX Couldn't figure out how h2ph was supposed to produce these
$IOPRO_ASYNC = 0;
$IOPRO_ASSIGN = 0x80000000 | ( 16 << 16 ) | ( ord ('I') << 8 ) | 0;

# Check the program arguments.
# very simple at present. One compulsory argument. The card number.

sub check_arguments
{
	if ( $#ARGV != 0 || $ARGV[0] < 0 || $ARGV[0] > 3 ) {
	    die "usage: $0 card_number";
	}

	$card_number = $ARGV[0];
}

# Open a log file if it exists and is writeable.
# If it exists but is not writable send log to STDOUT ( useful for debug ).
# if it don't exist don't create it and turn logging off.

sub setup_logfile
{
	$logfile_name = sprintf ("/var/log/ioprod_%d", $card_number );

	if ( -e $logfile_name ) {
	    if ( open ( LOGFILE, ">>" . $logfile_name )) {
	    	# Redirect stderr from subprocesses to log file as well
	    	open ( STDERR, ">&LOGFILE");
	    } else {
		# File exists but not writable direct log to stdout
		open ( LOGFILE,"-");
	    }
	    $logging_on = 1;

	    # Force unbuffered output
	    select ( LOGFILE ); $| = 1;
	} else {
	    # No log file so no logging
	    $logging_on = 0;
	}
}

# Add an entry to the log file

sub log
{
	if ( $logging_on ) {
	    # Start the line with a day.
	    # Using month names avoids confusion with the American date layout
	    @timevec = localtime;
	    printf ( LOGFILE "%2d %s %2d %2d:%02d:%02d\t", $timevec[3],
		("Jan","Feb","Mar","Apr","May","Jun"
		,"Jul","Aug","Sep","Oct","Nov","Dec")[ $timevec[4]],
		$timevec[5], $timevec[2], $timevec[1], $timevec[0]);
	    
	    printf ( LOGFILE "%s\n", join (' ', @_ ));
	}
}

# Setup device names for access.

sub setup_names
{
	$device_dir = "/dev/iopro";
	$card_dev = sprintf ("%s/c%d", $device_dir, $card_number );

	for ( $i = 0 ; $i < 4 ; $i++ ) {
	    $raw_dev[ $i ] = sprintf ("%s/c%du%dd", $device_dir, $card_number
									, $i );
	    $conf_dev[ $i ] = sprintf ("%s/c%du%d", $device_dir, $card_number
									, $i );
	}
}


# Wait for external units to be powered.
# Opening the card control port for reading will block until power is present.

sub wait_for_power_up
{
	&log ("Waiting for power");
	open ( CARD, "<" . $card_dev );
	close ( CARD );
	&log ("Got power");
	sleep ( 1 );
}

# Wait for power to be removed from the external units.
# Opening the card control port for writing will block while power is present.

sub wait_for_power_down
{
	&log ("Waiting for power down");
	open ( CARD, ">" . $card_dev );
	close ( CARD );
	&log ("Lost power");
}


# system() call with logging

sub syscmd
{
	&log ( @_ );

	# Force execution of a sub-process without a shell
	# Ripped off from somewhere, I ain't that devious normally :-)
	system split (' ', join (' ',@_ ));
}

# Download an external unit.
# This could probably be coded in Perl but I already had the C program

sub download
{
	&syscmd ("/usr/libexec/iopro_dload","/usr/libdata/iopro.dl",
					$raw_dev[ $unit ]);

	return $? == 0;
}


# Set up and look for timeouts

sub catch_timeout
{
	$timeout_occurred = 1;
}

sub set_timeout
{
	$SIG{'ALRM'} = 'catch_timeout';
	$timeout_occurred = 0;
	alarm ( @_ );
}

sub clear_timeout
{
	alarm ( 0 );

	return $timeout_occurred;
}


# Get a number from the firmware.

sub get_fw_number
{
	&log ("->", @_ );

	&set_timeout ( 5 );
	printf ( CFOUT "%s\n", join (' ', @_ ));
	sysread ( CFIN, $fwresp, 32 ) if ! $timeout_occurred;
	if ( &clear_timeout ) {
	    $fwresp = "TIMEOUT";
	}

	@fwrespvec = split ( /[ \n]/, $fwresp );

	&log ("<-", join ( ' ', @fwrespvec ));

	if ( $fwrespvec[0] eq "Ok" ) {
	    return $fwrespvec[1];
	} else {
	    return -1;
	}
}


# Find the minor device number for $dev_name

sub find_minor
{
	return -1 if ! -c $dev_name;

	@statvec = stat ( $dev_name );

	# XXX really should find the right macro for extracting the minor part
	#     of a device name.
	return $statvec[ 6 ] & 0xFF;
}

# Assign external unit page to a device in /dev

sub assign_port
{
	( $type, $phys_number, $page_number ) = @_;

	# Calc the device name
	$dev_name = sprintf ("/dev/%s%c%c%c", $type, ord ('E') + $card_number
		, ord ('0') + $unit, ord ('a') + $phys_number);

#	if ( ! -c $dev_name ) {
#	    &create_dev;
#	}

	$minor = &find_minor;

	if ( $minor >= 0 ) {
	    &log ("Device", $dev_name,"minor", $minor,"assigned to", $unit
								, $page );
	    $iarg = pack ("iIIi", $IOPRO_ASYNC, $minor, $page_number, 0 );
	    if (( $retval = ioctl ( CFOUT, $IOPRO_ASSIGN, $iarg )) != 0 ) {
		&log ("ERROR: Assignement ioctl faied error code", $retval );
	    }
	} else {
	    &log ("ERROR: Device", $dev_name,"does not exist");
	}
}


# Configure a single unit that has already been downloaded and started.
# This is a very cut down version it just assigns ports to the default firmware
# startup configuration.

sub configure_unit
{
	&log ("Configuring unit", $unit );

	# Open the config port
	open ( CFIN, "<" . $conf_dev[ $unit ]);
	open ( CFOUT,">" . $conf_dev[ $unit ]);
	select ( CFOUT ); $| = 1;

	# Ask firmware for number of physical ports
	$num_ports = &get_fw_number ("Rp Pt");
	if ( $num_ports > 0 )
	{
	    &log ( $num_ports,"ports detected");

	    # Link port to device in /dev
	    for ( $port = 0 ; $port < $num_ports ; $port++ ) {
		$page = &get_fw_number ("Rp Pt", $port );
		if ( $page > 0 ) {
		    &assign_port ("tty", $port, $page );
		} else {
		    &log ("ERROR: Failed to get page for port", $port );
		}
	    }
	} else {
	   &log ("ERROR: Unit", $unit,"failed to respond to config enquiry");
	}

	# Tidy up
	close ( CFIN );
	close ( CFOUT );
}


#	Program starts here
#	===================

&check_arguments;

# Put ourself in background ( but stay attached to terminal )
exit 0 if fork;

&setup_logfile;
&log ("Started");

&setup_names;


# Loop forever waiting for card to power cycle
main: while ( 1 )
{
	&wait_for_power_up;

	# Download all the external units
	for ( $unit = 0 ; $unit < 4 ; $unit++ ) {
	    last if ! &download;
	}
	$num_units = $unit;
	&log ( $unit," units found");

	# Wait for startup to complete
	sleep ( 2 );

	# Configure each unit in turn
	for ( $unit = 0 ; $unit < $num_units ; $unit++ ) {
	    &configure_unit;
	}

	&wait_for_power_down;
}

# Never reached
&log ("Exit from forever loop !");
die "Exit from forever loop !"
