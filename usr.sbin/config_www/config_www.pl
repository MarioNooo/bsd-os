#!/usr/bin/perl

#
# Copyright (c) 1994 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	config_www.pl,v 2.5 2000/01/10 17:16:39 polk Exp

require '/usr/libdata/adm/adm.pl';
require '/usr/libdata/adm/v.pl';

$WWWHOME = '/var/www/docs';
$WWWPAGE = 'index.html';
$WWWTEMPLATE = '/var/www/conf/homepage.template';
$WWWSTART = '/usr/contrib/bin/apachectl restart';

# Make sure we're running as root
if ($> != 0) {
        print "$0: this program must be run as root!\n";
        exit 1;
}

if ( !`grep ^www $PASSWD` || !`grep ^www $GROUP`) {
	print<<ETX;
The user www (uid: 51) and group www (gid: 84) must exist
in the password and group files before $0 is run.  You can 
use the adduser and addgroup commands to add them.

ETX
	die "$0: www user and group must exist -- aborting!\n";
}

$| = 1;  # Make STDOUT unbuffered...

require 'getopts.pl';
&Getopts('vkn') || die "Usage: $0 [-vk]\n";

$TITLE="BSD/OS WWW Configuration";
&vexplain("BSD/OS WWW Configuration") if !$opt_n;

restart:

$TEXT=
"Enter the name of your organization exactly as you want it
to appear on your www home page.  For example, BSDI usually
uses: Berkeley Software Design, Inc.";
$www{"ORG"} = &vquery("Enter your organization name:", $www{"ORG"});

$TEXT=
"Enter the address of your organization on a single line with 
semicolons in place of the line breaks.  For example:

        1234 Your Street; YourCity, YS 98765

will be taken to mean:

        1234 Your Street
        YourCity, YS 98765
";
$www{"ADDR"} = &vquery("Enter your address:", $www{"ADDR"});

$TEXT=
"Enter your phone number exactly as you wish it to appear in the
home page.  You will probably want to include the +1 and your area
code at the beginning of the number if you are in the United States
(or + your country code if you are outside the US).";
$www{"PHONE"} = &vquery("Enter phone number:", $www{"PHONE"});

$TEXT=
"Enter your FAX number exactly as you wish it to appear in the
home page.  You will probably want to include the +1 and your area
code at the beginning of the number if you are in the United States
(or + your country code if you are outside the US).";
$www{"FAX"} = &vquery("Enter fax number:", $www{"FAX"});

$TEXT=
"Enter your EMAIL address exactly as you wish it to appear in the
home page (e.g., login\@MYDOMAIN.COM).";
$www{"EMAIL"} = &vquery("Enter email address:", $www{"EMAIL"});


# Show the current settings
$TEXT="You current choices are:\n\n";
$TEXT.="$www{\"ORG\"}\n";
split(/;\s*/, $www{"ADDR"});
for $_ (@_) {
	$TEXT.="$_\n";
}
$TEXT.= "Phone: $www{\"PHONE\"}\n";
$TEXT.= "FAX: $www{\"FAX\"}\n";
$TEXT.= "Email: $www{\"EMAIL\"}\n";

$_ = &vquery("Is this correct?", "yes", ("yes", "no"));
if (!/^[yY]/) {
	goto restart;
}

# Create the new home page
&v_reset();
print "Writing new home page ($WWWHOME/$WWWPAGE)..." if $opt_v;
$omask = umask 002;
open(OUT, ">$WWWHOME/$WWWPAGE.tmp") || 
			die "\n$0: can't open $WWWHOME/$WWWPAGE.tmp: $!\n";
umask $omask;
&copysub("$WWWTEMPLATE", OUT) || 
			die "\n$0: can't copy/substitute $WWWTEMPLATE\n";
close(OUT);
rename("$WWWHOME/$WWWPAGE.tmp", "$WWWHOME/$WWWPAGE") || 
			die "\n$0: can't rename $WWWHOME/$WWWPAGE.tmp: $!\n";
print "DONE.\n" if $opt_v;

# Start (or restart) httpd
if (!$opt_k) {
	print "Starting WWW server..." if $opt_v;
	system($WWWSTART);
	print "DONE.\n" if $opt_v;
}

print "$0: Successful Completion\n\n";
exit 0;

sub copysub {
        local ($file, $out) = @_;
        
        if (!open(IN, "$file")) {
                print STDERR "$0: can't open $file: $!\n";
                return 0;
        }
        while (<IN>) {
                s/\@ORGANIZATION\@/$www{"ORG"}/g;
		if (/\@ADDR\@/) {
			split(/;\s*/, $www{"ADDR"});
			for $_ (@_) {
				print $out "<ADDRESS>$_</ADDRESS>\n";
			}
			next;
		}
		next if /\@PHONE\@/ && $www{"PHONE"} =~ /^\s*$/;
                s/\@PHONE\@/$www{"PHONE"}/g;
		next if /\@FAX\@/ && $www{"FAX"} =~ /^\s*$/;
                s/\@FAX\@/$www{"FAX"}/g;
		next if /\@EMAIL\@/ && $www{"EMAIL"} =~ /^\s*$/;
                s/\@EMAIL\@/$www{"EMAIL"}/g;
                print $out $_;
        }
        close(IN);
        1;
}
