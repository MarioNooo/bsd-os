#!/usr/bin/perl

#
# Copyright (c) 1995 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	installfloppy.pl,v 1.2 1998/11/20 18:25:20 polk Exp

require '/usr/libdata/adm/adm.pl';

# Make sure we're running as root
if ($> != 0) {
        print "$0: this program must be run as root!\n";
        exit 1;
}

$| = 1;  # Make STDOUT unbuffered...

$opt_d = "/a";
require 'getopts.pl';
&Getopts('d:') || die "Usage: $0 [ -d dir ]\n";

if ( ! -d $opt_d && ! mkdir($opt_d, 0755) ) {
	die "\n$0: $opt_d does not exist and could not be created: $!\n";
}

print <<ETX;

Insert the floppy you wish to install into the fd0 (A:) drive.

ETX
&query("Press <Enter> to continue");
print "\n";

print "Mounting floppy on $opt_d...";
system("mount -o ro /dev/fd0a $opt_d") && 
	die "\n$0: mount failed -- aborting\n";
print "DONE.\n\n";

if ( -x "$opt_d/scripts/install" ) {
	print "Executing floppy installation script...\n";
	sleep 2;
	$status = system("$opt_d/scripts/install $opt_d");
}
elsif ( -f "$opt_d/PACKAGES/PACKAGES" ) {
	$status = system("installsw -E -c $opt_d -L -m floppy");
}
else {
	print <<ETX;

This floppy does not appear to be installable by $0.

ETX
	$status = 1;
}

if ($status != 0) {
	print <<ETX;

The installation appears to have failed for some reason.  You may
need to reinstall this floppy after correcting the reported problems.

ETX
	&umount;
	exit 1;
}

&umount;
print <<ETX;

Installation completed successfully.  

It is now safe to remove the floppy from the drive.

ETX
exit 0;

# -------------------------------------------------------------------------

sub umount {
	print "Making sure floppy is unmounted from $opt_d...";
	# XXX -- doesn't check return because umount always exits 1???
	# XXX -- this be a duplicate umount if install script did it already
	system("umount /dev/fd0a");
	print "DONE.\n";
}
