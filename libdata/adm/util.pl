#!/usr/bin/perl

#
# Copyright (c) 1994 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	util.pl,v 1.5 1998/11/13 16:54:58 polk Exp

# Parse a generic record
sub parserec {
	local ($entry, $field, $val, @lines) = @_;
	@lines = split(/\n/,$entry);
	%fields = ();
	while (defined($_ = pop(@lines))) {
		next if /^$/;
		next if /^#/;
		($field, $val) = split(/:[ 	]+/, $_, 2);
		while ($fields{$field}) { $field .= "+"; }
		$fields{$field} = $val;
	}
	1;
}

# Read a file of generic records (in paragraph mode)
sub readfile {
	local ($file, $fatal) = @_;
	if (!open (IN, "$CONFIGPATH/$file")) {
		die "$0: can't open $CONFIGPATH/$file\n" if $fatal;
		print STDERR "$0: Warning: can't open $CONFIGPATH/$file\n";
		return 0;
	}
	@file=();
	$/ = "";		# Set paragraph mode for input
	@file=<IN>;
	close(IN);
	$/ = "\n";		# Back to normal
	1;
}

# Write out a generic record
sub writerec {
	local($FILE, %fields, @tmp, $field) = @_;
	@tmp=();
	for $_ (keys(%fields)) {
		push (@tmp, $_) if defined($fields{$_});
	}
	@tmp = sort @tmp;
	for $_ (@tmp) {
		/\+$/ && next;
		$field = $_;
		while (defined($fields{$_})) {
			print $FILE "$field:\t$fields{$_}\n";
			$_ .= "+";
		}
	}
	print $FILE "\n";
	1;
}

# Put a generic record into a string
sub sprintrec {
	local(%fields, @tmp, $field, $str) = @_;
	$str = "";
	@tmp = ();
	for $_ (keys(%fields)) {
		push (@tmp, $_) if defined($fields{$_});
	}
	@tmp = sort @tmp;
	for $_ (@tmp) {
		/\+$/ && next;
		$field = $_;
		while (defined($fields{$_})) {
			$str .= "$field:\t$fields{$_}\n";
			$_ .= "+";
		}
	}
	$str .= "\n";
	return $str;
}

# Read a file as a single generic record
sub readsimple {
	local ($file, $fatal, $lines) = @_;
	if (!open (IN, "$CONFIGPATH/$file")) {
		die "$0: can't open $CONFIGPATH/$file\n" if $fatal;
		print STDERR "$0: Warning: can't open $CONFIGPATH/$file\n";
		return 0;
	}
	undef $/;		# Snarf entire file
	$lines=<IN>;
	close(IN);
	$/ = "\n";		# Back to normal
	&parserec($lines);
	1;
}

# Write a file from a single %fields array
sub writesimple {
	local ($file, %fields, $fatal) = @_;
	if (!open (OUT, ">$CONFIGPATH/$file")) {
		die "$0: can't open $CONFIGPATH/$file\n" if $fatal;
		print STDERR "$0: Warning: can't open $CONFIGPATH/$file\n";
		return 0;
	}
	print OUT <<ETX;
#
# Format is: <fieldname>:<tab><fieldvalue>
#
ETX
	&writerec("OUT", %fields);
	close(OUT);
	1;
}

# Print a header and list in columnar form
sub printlist {
	local ($header, @items, $max, $rows, $cols, $start, $i, $screen) = @_;
	
	print "$header";
	grep ((length > $max) && ($max = length), @items);
	$cols = int (($COLS+$GUTTER)/($max+$GUTTER));
	$rows = int (($#items+1+$cols-1)/$cols);
	for ($start=0; $start<$rows; $start++) {
		for ($i=$start; $i<=$#items;$i+=$rows) {
			print " " x $GUTTER if $i != $start;
			printf "%-${max}s", $items[$i];
		}
		printf "\n";
		if ($screen++ > 18) {
			&query("\nPress <Enter> to continue");
			print "\n";
			$screen=0;
		}
	}
	print "(none)\n" if ($#items < 0);

	1;
}

# Query the user
sub query {
	local ($query, $default, @choices, $prompt, $tmp, $_) = @_;
	
	if ($default) {
		$prompt = "$query [$default]: ";
	}
	else {
		$prompt = "$query: ";
	}
prompt:
	for (;;) {
		print $prompt;
		$_ = <STDIN> || die "$0: got EOF -- exiting.\n";
		chop;
		return $default if $default && /^$/;
		last if $#choices < 0;
		for $tmp (@choices) {
			last prompt if $_ eq $tmp;
		}
		print "\nInvalid resonse: $_\n";
		print "Valid responses are: ";
		for $tmp (@choices) {
			print "$tmp ";
		}
		print "\n\n";
	}
	return $_;
}

# Query for a hostname
sub query_name {
	local ($prompt, $default) = @_;
	for (;;) {
	        $_ = &query($prompt, $default);
        	last if /^[-A-Za-z0-9_\.]+$/;
		last if /^\?$/ || /^$/;
		print <<ETX;

Invalid entry.

Host/domain names must consist of letters, digits, `-', `.', and `_' only.

ETX
	}
	return $_;
}

# Query for an address
sub query_addr {
	local ($prompt, $default, $min_fields, $max_fields, 
		$pattern, $fields) = @_;
	$min_fields = 4 if ! $min_fields;
	$max_fields = 4 if ! $max_fields;
	$pattern = "\\d{1,3}\\." x ($min_fields - 1);
	$pattern .= "\\d{1,3}";
	for (;;) {
	        $_ = &query($prompt, $default);
		$fields = split(/\./);
		last if /$pattern/ && $fields <= $max_fields;
		print "\nInvalid entry.\n";
		if ($min_fields == 4) {
			print <<ETX;

IP addresses consist of 4 numbers (each between 0 and 255 inclusive) 
separated by periods (e.g., 192.0.0.23).

ETX
		}
		else {
			print <<ETX;

IP network addresses consist of up to 3 numbers (each between 0 and 255
inclusive) separated by periods. The numbers are the network portion of 
IP addresses on your local network.  For example, a class C network 
consists of three numbers e.g.: 192.0.0

ETX
		}
	}
	return $_;
}

# getpass() Read a password from the tty
sub getpass {
	local ($prompt) = @_;
	system("stty -echo");
	print "$prompt";
	$_ = <STDIN>;
	chop;
	print "\n";
	system("stty echo");
	return $_;
}

# Parse group file
sub parsegroup {
	local($group, $passwd, $gid, $members, $_);
	%group=();
	%gid=();
	%gpasswd=();
	%members=();
	print "Reading current group file information\n" if $opt_v;
	open (GROUP, "$GROUP") || die "$0: can't open $GROUP\n";
	while (<GROUP>) {
		chop;
		next if /^$/;
		($group, $passwd, $gid, $members) = split(/:/);
		$group{$group} = $gid;
		$gid{$gid} = $group;
		$gpasswd{$group} = $passwd;
		$members{$group} = $members;
	}
	close(GROUP);
	1;
}

# Write out new group file
sub writegroup {
	local(@tmp, $name);
	@tmp=();
	for $i (keys(%gid)) {
		next if !defined($gid{$i});
		push(@tmp, $i);
	}
	@tmp = sort {$a <=> $b} @tmp;
	print "Writing new group file..." if $opt_v;
	local ($o_umask) = umask(022);
	open (GROUP, ">${GROUP}.tmp") || 
		die "\n$0: can't open ${GROUP}.tmp for writing\n";
	for $i (@tmp) {
		$name = $gid{$i};
		print GROUP "$name:$gpasswd{$name}:$i:$members{$name}\n";
	}
	close(GROUP);
	umask($o_umask);
	print "DONE.\n" if $opt_v; 
	print "Fixing ownership and modes.\n" if $opt_v;
	chown (0, 0, "$GROUP.tmp") || 
		die "$0: can't chown temporary file to root.wheel\n";
	chmod (0644, "$GROUP.tmp") || 
		die "$0: can't chmod temporary file to 0644\n";
	print "Renaming new group file into place.\n" if $opt_v;
	rename("$GROUP.tmp", "$GROUP") ||
		die "$0: can't rename temporary file to $GROUP\n";
	1;
}

# List existing groups
sub listgroups {
	local (@tmp);
	@tmp = sort keys(%group);
	&printlist("Current groups:\n", @tmp);
	1;
}

# Parse master.passwd file
sub parsepasswd {
	local($close) = @_;
	local($login, $passwd, $uid, $pwgid, $class, $change, $expire, $gecos,
		$home_dir, $shell, $tmp);
	%login = %passwd = %uid = %pwgid = %class = %change =
		%expire = %gecos = %home_dir = %shell = ();

	# definitions for flock
	$LOCK_SH=1;
	$LOCK_EX=2;
	$LOCK_NB=4;
	$LOCK_UN=8;

	open (PASSWD, "$PASSWD") || die "$0: can't open $PASSWD\n";

	flock(PASSWD, $LOCK_EX | $LOCK_NB) ||
                die "$0: the password file is currently busy: $!\n";

	print "Reading current master.passwd file information..." if $opt_v;
	while (<PASSWD>) {
		chop;
		($login, $passwd, $uid, $pwgid, $class, $change, $expire, 
			$gecos, $home_dir, $shell) = split(/:/);
		$login{$login} = $uid;
		$tmp = $uid;
		while (defined($uid{$tmp})) { $tmp .= "+"};
		$uid{$tmp} = $login;
		$passwd{$login} = $passwd;
		$pwgid{$login} = $pwgid;
		$class{$login} = $class;
		$change{$login} = $change;
		$expire{$login} = $expire;
		$gecos{$login} = $gecos;
		$home_dir{$login} = $home_dir;
		$shell{$login} = $shell;
	}
	close(PASSWD) if $close;
	print "DONE.\n" if $opt_v;
	1;
}

# Write out new passwd file
sub writepasswd {
	local(@tmp, $login, $uid);
	@tmp=();
	for $i (keys(%uid)) {
		next if $i =~ /\+/;
		next if !defined($uid{$i});
		push(@tmp, $i);
	}
	@tmp = sort {$a <=> $b} @tmp;
	print "Writing new master.passwd file..." if $opt_v;
	local ($o_umask) = umask(066);	# no read or write for others
	open (PASSWD, ">${PASSWD}.tmp") || 
		die "\n$0: can't open ${PASSWD}.tmp for writing\n";
	for $i (@tmp) {
		$uid = $i;
		while (defined($uid{$i})) {
			$login = $uid{$i};
			$i .= "+";
			next if $login eq "";		# ignore removed users
			print PASSWD "$login:$passwd{$login}:$uid:$pwgid{$login}:$class{$login}:$change{$login}:$expire{$login}:$gecos{$login}:$home_dir{$login}:$shell{$login}\n";
		}
	}
	close(PASSWD);
	umask($o_umask);
	print "DONE.\n" if $opt_v;
	print "Rebuilding password databases..." if $opt_v;
	system("$PWD_MKDB -p ${PASSWD}.tmp") &&   
       		die "\n$0: can't rebuild databases: $!\n";
	print "DONE.\n" if $opt_v;
	1;
}

# List existing users
sub listusers {
	local (@tmp);
	@tmp = sort keys(%login);
	&printlist("Current users:\n", @tmp);
	1;
}

# Explain queries/defaults
sub explain {
	print <<'ETX';

Please supply answers to the series of questions below.  When a
`default answer' is available, it will follow the question in square
brackets.  For example, the question:

	What is your favorite color? [blue]:

has the default answer `blue'.  Accept the default (without any
extra typing!) by pressing the <Enter> key -- or type your answer
and then press <Enter>.

Use the <Backspace> key to erase and aid correction of any mistyped
answers -- before you press <Enter>.  Generally, once you press
<Enter> you move onto the next question.

Once you've proceeded through all the questions, you will be given
the option of modifying your choices before any files are updated.

ETX

	&query("Press <Enter> to continue");
	print "\n";
	1;
}

1;
