#!/usr/bin/perl

#
# Copyright (c) 1994, 1995 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	config_anonftp.pl,v 2.8 2000/10/02 19:16:25 chrisk Exp

$ENV{'PATH'} = "/sbin:/usr/sbin:/bin:/usr/bin:/usr/contrib/bin";

require '/usr/libdata/adm/adm.pl';

# Make sure we're running as root
if ($> != 0) {
        print "$0: this program must be run as root!\n";
        exit 1;
}

$| = 1;  # Make STDOUT unbuffered...

# These will be needed when we're willing to configure ftpaccess...

#require 'getopts.pl';
#&Getopts('d') || die "Usage: $0\n";

# Read the data from last time
#&readsimple($FTP_DEF, 0);
#%ftp = %fields;

#
# This is real crufty for now (quick and dirty)...
#

# Force the home directory and ownerships
$ftp{"DIR"}="/var/spool/ftp";
$owner=0;	# owned by root
$group=0;	# group wheel


# Get the user name from /etc/passwd
$_ = `grep ^ftp: /etc/master.passwd`;
if ($? == 0) {
	print <<ETX;
It appears that this host is already configured for anonymous ftp.  
Disable anonymous ftp by removing the user named `ftp' from 
the password file (e.g., with a command like ``rmuser ftp'').

ETX
	exit 1;
}

print "Setting up home directory for anonymous ftp...";

# Make sure the home directory exists
&check_or_mkdir($ftp{"DIR"}, 0755);

# Make sure pub and hidden dirs exist with correct perms
&check_or_mkdir("$ftp{\"DIR\"}/pub", 0755);
&check_or_mkdir("$ftp{\"DIR\"}/hidden", 0711);

# Make an empty etc/pwd.db file to keep ftpd happy
&check_or_mkdir("$ftp{\"DIR\"}/etc", 0711);
if (! -f "$ftp{\"DIR\"}/etc/pwd.db") {
	$curdir = `/bin/pwd`;
	if (chdir("$ftp{\"DIR\"}/etc") && open(MASTER,">master.$$")) {
		close(MASTER);
		system("/usr/sbin/pwd_mkdb -d -l master.$$");
		unlink("spwd.db","master.$$");
		chdir($curdir);
	}
}

print "DONE.\n\n";

# XXX -- we should do these here when this can handle fancy config
# Generate /etc/ftpaccess
# Make sure inetd.conf specifies -a instead of -A

# Make sure we've got an ftp user in the passwd file
# XXX -- just add it for now
@cmd=();
push (@cmd, "adduser");
push (@cmd, "-u"); push (@cmd, "50");
push (@cmd, "-g"); push (@cmd, "nogroup");
push (@cmd, "-G"); push (@cmd, "Anonymous FTP,,,");
push (@cmd, "-P"); push (@cmd, "*");
push (@cmd, "-s"); push (@cmd, "/dev/null");
push (@cmd, "-H"); push (@cmd, $ftp{"DIR"});
push (@cmd, "ftp");
system(@cmd) && die "$0: failed to add anonymous ftp user to password file\n";

print <<ETX;
Anonymous ftp is now configured for use with the standard BSD/OS FTP
Daemon, ftpd.  Any files placed in the $ftp{"DIR"}/pub directory and
made world-readable will be available for anonymous transfers.  Any files
placed in the $ftp{"DIR"}/hidden directory will be available for anonymous
transfers, but the person retrieving the file will need to know the name
(the `ls' and `dir' commands will not work for that directory).

No anonymous uploads are permitted in the normal installation.  If 
you wish to enable uploading of files, see the section on anonymous
ftp in the release notes or the various ftp daemon related
man pages (ftpd(8), ftpconfig(5), etc.).

To disable anonymous ftp, remove the `ftp' user from the password
file (e.g., with a command like: ``rmuser ftp'').

ETX

###################################################################

# Make sure the directory exists with the specified mode
sub check_or_mkdir {
	local($dir, $mode) = @_;

	if ( ! -d $dir ) {
		mkdir($dir, 0777) || die "\n$0: mkdir $dir failed\n";
	}
	chown($owner, $group, $dir) == 1 || die "\n$0: chown $dir failed\n";
	chmod($mode, $dir) == 1 || die "\n$0: chmod $dir failed\n";
}

# Copy the specified file to the specified dir fixing up owner and modes
sub copy_file {
	local($srcdir, $file, $destdir, $mode) = @_;

	system("cp $srcdir/$file $destdir/$file") && 
		die "\n$0: failed to copy $srcdir/$file to $destdir/$file\n";
	chown($owner, $group, "$destdir/$file") == 1 || 
		die "\n$0: chown $destdir/$file failed\n";
	chmod($mode, "$destdir/$file") == 1 || 
		die "\n$0: chmod $destdir/$file failed\n";
}
