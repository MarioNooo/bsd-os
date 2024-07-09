#!/usr/bin/perl

#
# Copyright (c) 1994 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	addgroup.pl,v 1.2 1996/09/17 01:17:10 prb Exp

require '/usr/libdata/adm/adm.pl';

# Make sure we're running as root
if ($> != 0) {
        print "$0: this program must be run as root!\n";
        exit 1;
}

$| = 1;  # Make STDOUT unbuffered...

require 'getopts.pl';

if ($0 eq "rmgroup") {
	$opt_d++;
	&Getopts('v') || 
		die "Usage: $0 [group [group...]]\n";
	$action = "delete";
}
else {
	&Getopts('dg:m:v') || 
		die "Usage: $0 [-dv] [-m member[,member...]] [group [group...]]\n";
	$action = "create";
	$action = "delete" if $opt_d;
}

# read the current group file
&parsegroup;

$changed = 0;

if ($#ARGV < 0) {
	$interactive++;
	&explain;
}

restart:

while ($#ARGV < 0) {
	print <<ETX;
Please enter the name of the group you wish to $action,
or press <Enter> for a list of current groups.

ETX
	$_ = &query("Group name?", $groupname);
	print "\n";
	if (/^$/ || /^\?$/) {
		&listgroups; 
		print "\n";
		next;
	}
	if (!/^[a-zA-Z][a-zA-Z0-9]*$/ || length($_) > 8) {
		print <<ETX;
Group names must be 8 characters or fewer, must start with a letter, 
and may contain only letters and digits.

ETX
		next;
	}
	unshift(@ARGV, $_);
}

if ($opt_d) {
	# delete named groups
	while ($_ = pop(@ARGV)) {
		$groupname = $_;
		if (!defined($group{$groupname})) {
			print STDERR "$0: group $groupname not found -- ignored.\n\n";
			next;
		}
		if ($group{$groupname} < 100) {
print "Chosen group ($groupname) appears to be a system group (gid: $group{$groupname}).\n";
			$_ = &query("Remove it anyway?", "no");
			print "\n";
			/^[yY]/ || next;
		}
		if ($interactive) {
			$_ = &query("Delete group `$groupname'?", "yes");
			print "\n";
			/^[yY]/ || next;
		}
		undef $gid{$group{$groupname}};
		print "Deleting group: $groupname\n";
		$changed++;
	}
}
else {
	# create named groups
	while ($_ = pop(@ARGV)) {
		if (!/^[a-zA-Z][a-zA-Z0-9]*$/ || length($_) > 8) {
			print <<ETX;
Group names must be 8 characters or fewer, must start with a letter, 
and may contain only letters and digits.  Ignoring $_.

ETX
			next;
		}
		$groupname = $_;
		if (defined($group{$groupname})) {
			print STDERR "$0: group $groupname already exists -- ignored.\n\n";
			next;
		}
		if ($groupname =~ /^[^a-zA-Z0-9_]+$/) {
			print STDERR "$0: groupname `$groupname' contains illegal characters -- ignored.\n\n";
			next;
		}

		if ($interactive) {
			$_ = &query("Add group `$groupname' to the group file?", "yes");
			print "\n";
			next if !/^[yY]/;
		}

		# find next unused ID starting at 100
		for ($i=100; $gid{$i}; $i++) {};
		$i = $opt_g if $opt_g;
		$group{$groupname} = $i;
		$gid{$i} = $groupname;
		$gpasswd{$groupname} = "*";
		$members{$groupname} = $opt_m if ($opt_m);

		print "Creating new group: $groupname (gid: $i)\n\n";
		$changed++;
	}
	if ($interactive) {
		$_ = &query("Do you wish to add additional groups?", "yes");
		if (/^[yY]/) {
			print "\n";
			undef $groupname if defined($group{$groupname});
			goto restart;
		}
	}
}

# write out the new group file
&writegroup if $changed;

print "$0: Successful Completion\n\n";
exit 0;
