#!/usr/bin/perl

#
# Copyright (c) 1994,1995,1996 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	configure_system.pl,v 1.19 2002/09/03 22:52:08 polk Exp

system("stty erase ");
$ENV{'TERM'} = "bsdos-pc" if $ENV{'TERM'} eq "";

require '/usr/libdata/adm/adm.pl';
require '/usr/libdata/adm/v.pl';

# Make sure we're running as root
if ($> != 0) {
	print "$0: this program must be run as root!\n";
	&v_reset();
	exit 1;
}

$| = 1;  # Make STDOUT unbuffered...

&querysystem();
&v_setterm();
if ( $ENV{'TERM'} eq "ibmpc-mono" ) {
	$ENV{'TERM'} = "ibmpc3";	# XXX - this could be wrong...
}
if ( $ENV{'TERM'} eq "ibmpc3" ) {
	$ENV{'TERM'} = "bsdos-pc";
}

if (! -f "/etc/netstart") {
	open (PROTO, "/etc/netstart.proto") ||
	    die "$0: can't open /etc/netstart.proto: $!\n";
	@proto = <PROTO>;
	close PROTO;
	open (OUT, ">/etc/netstart") ||
	    die "\nCould not open /etc/netstart for writing ($!)";
	print OUT <<ETX;
#
# netstart - configure network daemons, interfaces, and routes
#

hostname=
interfaces=""
primary=""
defroute=""

# These flags specify whether or not to run the daemons (values YES or NO),
# or the flags with which to run them if another value.
routedflags=NO
timedflags=NO
rwhod=NO
rstatd=NO

ETX
	print OUT @proto;
	close OUT;
}

# determine how much memory we have
if (open(IN, "sysctl -n hw.usermem |")) {
        $memsize = <IN>;
        close(IN);
} else {
        $memsize = 16 * 1024 * 1024;
}

# XXX
# XXX  Be sure to test on a 24 line display after
# XXX  adding to any of the $TEXT fields.
# XXX

$TITLE="BSD/OS System Configuration";
if ($ENV{'TERM'} eq "ansi" || ! -f "/usr/sbin/maxim") {
	$METHOD="manual";

$TEXT="Welcome to BSD/OS System Configuration...

Before BSD/OS is made available for general use on this machine
there are items which need to be configured.  They will be
configured now.

Note that system configuration does not configure all aspects of the
system.

You may also choose to have no assistance in configuration (which
implies you will configure the system by hand)";
	
	if (&vquery("Configure the system?", "yes", ("yes", "no")) eq "no") {
		$METHOD="none";
	}
} else {
$TEXT="Welcome to BSD/OS System Configuration...

Before BSD/OS is made available for general use on this machine
there are items which need to be configured.  There are two methods
that may be used to configure the system, MaxIM and Manual.  It
should be noted that Manual configuration does not configure all
aspects of the system.
";

if ($memsize < 12 * 1024 * 1024) {
	$METHOD="manual";
	$TEXT.="
Due to the limited memory in this system, we strongly recommend
that the \"manual\" method be used due to the memory requirements
of MaxIM.  MaxIM may still be used for those who desire a user
friendly graphical interface, but operations may be very slow due
to excessive paging";
	&vquery("Press <ENTER> to continue:");
	$TEXT = "Choose a configuration method:
";
} else {
	$METHOD="maxim";
}
$TEXT.="
    MaxIM - The new, easy to use, graphical configuration tool
    Manual - Minimal text based configuration assistance.
    None - No configuration assistance.

If you do not wish to follow the normal configuration procedure,
enter \"none\" and you will be placed in the single-user shell.";
	$METHOD = &vquery("Use MaxIM or Manual Configuration?", $METHOD,
		  ("maxim", "manual", "none"));
}

if ($METHOD eq "none") {
	$TEXT=
"You will now be placed in single-user mode.  Please configure your
system.  Once you have configured your system, exit the shell to
go multi-user.  At a minimum you must specify a hostname in the
/etc/netstart file.  This line should have the form:

    hostname=myhost.mydomain.com

Please see the system release notes for more detailed information on
configuring your system.";

	&vquery("Press <ENTER> to continue:");
	&v_reset();
	exit 1
}

if ($METHOD eq "maxim") {
	$TEXT=
"MaxIM requires a console with at least VGA support and a mouse.
The X11 Window System must also be configured to run MaxIM.  Please
review the section in the release notes on X11 configuration before
continuing.  If this system does not have the required hardware to
run the X11 Window System, or you are unsure as to what hardware
is in this machine, we recommend you do not use MaxIM at this time.";
	if (&vquery("Continue on with MaxIM?", "yes", ("yes", "no")) eq "no") {
		$METHOD="manual";
	}
}
if ($METHOD eq "maxim") {
	&v_reset();
	exec "/usr/sbin/maxim -i";
	die "Unable to execute /usr/sbin/maxim\n";
}

if (! -f "/etc/license") {
	&set_license;
}

$COMMENTS="";
&set_timezone;

$COMMENTS="";
require '/usr/libdata/adm/netutil.pl';
&config_netstart;

&v_reset();

exit 0;

sub set_timezone {
	local($_, @TZ_DIRS, @TZ_MISC, @_);

	$zone="..";
	while ($zone eq "..") {
		@TZ_DIRS=("Misc");
		@TZ_MISC=("..");
		foreach $file (split(/^/, `ls $TZDIR`)) {
			$_=$file;
			chop;
			next if /^[^A-Z]/;
			next if /^SystemV/;
			push(@TZ_DIRS, $_) if (-d "$TZDIR/$_");
			push(@TZ_MISC, $_) if (-f "$TZDIR/$_");
		}
		$INVALID="";
		$COMMENTS="Setting time zone";
		$HELP=
	"Each of the areas listed contains a list of cities and or timezones.
	Select the area that you are in.  The region \"Etc\" contains standard
	timezones and \"Misc\" contains a random assortment of timezones.";
		$TEXT="The following regions are available:\n\n";
		&v_addlist(sort @TZ_DIRS);

		$tregion = "__None__";
		while (grep(/^$tregion/i, @TZ_DIRS) != 1) {
			$tregion = &vquery("Which region is this machine located in?",
				"US");
			$INVALID="Invalid response.  Please choose from the list.";
		}
		$INVALID="";

		foreach $_ (@TZ_DIRS) {
			if ($_ =~ /^$tregion/i) {
				$region=$_;
			}
		}

		if ($region eq "Misc") {
			$region="";
			@TZ_LIST=@TZ_MISC;
		} else {
			@TZ_LIST=("..");
			foreach $file (split(/^/, `ls $TZDIR/$region`)) {
				$_=$file;
				chop;
				push(@TZ_LIST, $_) if -f "$TZDIR/$region/$_";
			}
		}
		$TEXT="The following zones or cities are defined for this region:\n\n";
		$HELP=
	"Please select a city which is in the same timezone as this machine
	is located, or if listed, the actual timezone.

	Select \"..\" to return to the list of regions.";
		&v_addlist(sort @TZ_LIST);

		$tzone = "__None__";
		while (grep(/^$tzone/i, @TZ_LIST) != 1) {
			$tzone = &vquery("Which time zone (\"..\" to return to the list of regions)?");
			$INVALID="Invalid response.  Please choose from the list.";
			last if $tzone eq "..";
		}
		$INVALID="";

		if ($tzone eq "..") {
			$zone="..";
		} else {
			foreach $_ (@TZ_LIST) {
				if ($_ =~ /^$tzone/i) {
					$zone=$_;
				}
			}
			if (system("cp $TZDIR/$region/$zone /etc/localtime")) {
				$HELP="";
				$TEXT=
"Could not copy timezone information into place.  This is not good.
The probable reason is that your root file system is not writable.";
				&vquery("Press <ENTER> to continue anyhow: ");
			}
		}
	}
}

sub set_license {
    $COMMENTS="";
    while (1) {
	$TEXT=
"BSD/OS now supports licenses to help administrators determine if they
are exceeding their licensed number of users.  Licenses from BSDI also
include a hostid which is unique among all BSDI licensed machines.
While a license is not required to install and use the BSD/OS operating
system, entering your license now will prevent several warning messages
from being printed each time the system is brought up as well as when
more than a single unique user is using the machine.  To continue without
a license, enter the license of \"none\".";

	$HELP=$TEXT;
	$LICENSE = &vquery("Please enter the BSD/OS license:");
	$LICENSE =~ s/\ //g;
	if ($LICENSE eq "none") {
		return;
	}
	if (!system("sysctl -w kern.license=\"$LICENSE\" > /dev/null 2>&1")) {
		system("sysctl -n kern.license > /etc/license");
		return;
	}
        $COMMENTS="The license \"$LICENSE\" is invalid.";
    }
}
