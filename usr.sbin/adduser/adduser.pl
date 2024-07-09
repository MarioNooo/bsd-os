#!/usr/bin/perl

#
# Copyright (c) 1994,1995 Berkeley Software Design, Inc.
# All rights reserved.
#
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	adduser.pl,v 1.11 2000/01/20 19:54:33 polk Exp

require '/usr/libdata/adm/adm.pl';


# to64() -- based on the version in usr.bin/passwd/passwd.c
#  0 ... 63 => ascii - 64 
$toa64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
sub to64 {
        $v = shift; 
        $n = shift;
	my $s;
	my $i;

	for ($i=0; $i < $n; $i++) {
                substr($s, $i, 1) = substr($toa64, $v & 0x3f, 1);
                $v >>= 6;
        }

	return $s;
}

# Call crypt() appropriately for both regular and long passwords --
# 	based on code in usr.bin/passwd/passwd.c
sub encrypt {
	$passwd = shift;
	my $salt;

	if (length($passwd) > 8) {
		$salt = "_";	# _PASSWORD_EFMT1
		$salt .= to64(29 * 25, 4);
		$salt .= to64(rand 65536, 4);
	}
	else {
		$salt = &to64(rand 65536, 2);
	}
	
	return crypt($passwd, $salt);
}

# Make sure we're running as root
if ($> != 0) {
        print "$0: this program must be run as root!\n";
        exit 1;
}

$| = 1;  # Make STDOUT unbuffered...

require 'getopts.pl';

if ($0 eq "rmuser") {
	$opt_d++;
	&Getopts('v') || die "Usage: $0 [user [user...]]\n";
	$action = "delete";
}
else {
	&Getopts('c:C:dDe:g:G:h:H:m:np:P:s:S:u:v') || do {
		print STDERR <<ETX;
Usage: $0 [-dDv] 
	  [ -c change_time ]
	  [ -C login_class ]
	  [ -e expire_time ]
	  [ -g primary_group ]
	  [ -G gecos ]
	  [ -h home_directory_basedir ]
	  [ -H home_directory ]
	  [ -m home_directory_mode ]
	  [ -p password ]
	  [ -P encrypted_password ]
	  [ -s shell ]
	  [ -S skeleton_directory ]
	  [ -u uid ]
ETX
		exit 1;		
	};
	$action = "create";
}

umask 0;

# Override $SKEL with $opt_S
$SKEL=$opt_S if $opt_S ne "";

# read the defaults file for newuser
&readsimple($PASSWD_DEF, 0);

# read the current group file
&parsegroup;

# seed the random number generator with the current time
srand;

$allowsig = 1;
$SIG{'INT'} = 'handler';
$SIG{'HUP'} = 'handler';
$SIG{'TSTP'} = 'IGNORE';
$SIG{'QUIT'} = 'handler';

# read the current passwd file
&parsepasswd(0);

$changedpw = 0;
$changedgrp = 0;

# Do a simple sanity check on arguments

$opterrs = 0;
if ($opt_g && !defined($group{$opt_g})) {
	if ($opt_g !~ /^\d+$/) {
		print STDERR "$opt_g: Invalid group name.\n";
		$opterrs++;
	}
}
if ($opt_c && $opt_c !~ /^\d+$/) {
	print STDERR "$opt_c: invalid change time.\n";
	$opterrs++;
}
if ($opt_e && $opt_e !~ /^\d+$/) {
	print STDERR "$opt_e: invalid expire time.\n";
	$opterrs++;
}
if ($opt_u && $opt_u !~ /^\d+$/) {
	print STDERR "$opt_u: invalid UID.\n";
	$opterrs++;
}
if ($opt_m && $opt_m !~ /^0?[0-7]{3,3}$/) {
	print STDERR "$opt_m: invalid home directory mode.\n";
	$opterrs++;
}
$opt_m = (defined $opt_m) ? oct $opt_m : $DEF_DIRMODE;
exit 1 if $opterrs > 0;

if ($opt_D) {
	$fields{"GROUP"} = $opt_g if $opt_g;
	$fields{"HDIR"} = $opt_h if $opt_h;
	$fields{"SHELL"} = $opt_s if $opt_s;
	$fields{"CHANGE"} = $opt_c if $opt_c;
	$fields{"EXPIRE"} = $opt_e if $opt_e;
	$fields{"NUID"} = $opt_u if $opt_u;
	&writesimple($PASSWD_DEF, %fields);
	exit 0;
}

if ($#ARGV < 0) {
	$interactive = 1;
	&explain;
}

# We'll come back here if we're in interactive mode and he wants more users...
restart:

while ($#ARGV < 0) {
	print <<ETX;
Login names are up to 16 characters and consist of upper or lower
case alphabetic characters or digits.  They must start with an
alphabetic character and should generally be all lower case.

Please enter the login name for the user you wish to $action or 
press just <Enter> for a list of current users.

ETX
	$_ = &query("Login name", $login);
	print "\n";
	if (/^$/ || /^\?$/) {
		&listusers;
		print "\n";
		next;
	}
	if (!/^[a-zA-Z][a-zA-Z0-9]*$/ || length($_) > 16) {
		print <<ETX;
Login names must be 16 characters or fewer, must start with a letter, 
and may contain only letters and digits.

ETX
		next;
	}
	unshift(@ARGV, $_) 
}

if ($opt_d) {
	# delete named user(s)
	@home_dirs=();
	while ($_ = pop(@ARGV)) {
		$login = $_;
		if (!defined($login{$login})) {
			print STDERR "$0: user $login not found -- ignored.\n\n";
			next;
		}

		# ask for confirmation if he typed it...
		if ($interactive) {
			$_ = &query("Delete user `$login'?", "yes");
			print "\n";
			next if !/^[yY]/;
		}

		$tmp = $login{$login};
		while ($uid{$tmp} ne $login) {
                        $tmp .= "+";
                }
		$uid{$tmp} = "";	# set it to empty string

		print "Deleting user: $login\n";
		push(@home_dirs, $home_dir{$login});
		$changedpw++;

		# Remove user from group file?
		for $_ (keys %group) {
			if ($members{$_} =~ /\b${login}\b/) {
				$members{$_} =~ s/\b$login\b//;
				$members{$_} =~ s/,,/,/;
				$members{$_} =~ s/,$//;
				$members{$_} =~ s/^,//;
				print "Removed `$login' from group $_\n" if $opt_v;
				$changedgrp++;
			}
		}
		print "\n";
	}
	print <<ETX;
You should remove the old home directory(ies) if appropriate.  One 
possible command to do that would be:

	rm -r @home_dirs

Be sure that each directory is a regular user's home
directory and not an important system directory before issuing
this command.

ETX
}
else {
	# create named user
	while ($_ = pop(@ARGV)) {
		if (!/^[a-zA-Z][a-zA-Z0-9]*$/ || (length($_) > 16)) {
			print STDERR "$0: illegal name `$_' -- ignored.\n\n";
			next;
		}
		if (defined($login{$_})) {
			print STDERR "$0: user `$_' exists -- ignored.\n\n";
			next;
		}
		print "Adding user: $_\n\n";
		if ($opt_u) {
			$uid = $opt_u;
		}
		else {
			# find next unused ID starting at the last one or 100
			$i = 100;
			$i = $fields{"NUID"} if $fields{"NUID"};
			for (; $uid{$i}; $i++) {};
			$uid = $fields{"NUID"} = $i;
			$fields{"NUID"}++;
		}

		# fill in the easy parts
		$login = $_;
		$login{$login} = $uid;
	        $_ = $uid;
	        while (defined($uid{$_})) { $_ .= "+"};
	        $uid{$_} = $login;
	        $class{$login} = $opt_C if $opt_C;
	        $change{$login} = 0;
		$change{$login} = $fields{"CHANGE"} if $fields{"CHANGE"};
		$change{$login} = $opt_c if $opt_c;
	        $expire{$login} = 0;
		$expire{$login} = $fields{"EXPIRE"} if $fields{"EXPIRE"};
	        $expire{$login} = $opt_e if $opt_e;

		# Get the encrypted passwd
		if ($opt_p) {
	        	$passwd{$login} = &encrypt($opt_p);
		}
		elsif ($opt_P) {
	        	$passwd{$login} = $opt_P if $opt_P;
		}
		else {
			local ($tries);
			for (;;) {
				local($tmp);
				print <<'ETX';
For security purposes, no characters are printed when entering passwords.

ETX
				$tmp = &getpass("Password: ");
				if (length($tmp) < 5 && ++$tries < 2) {
					print "Please use a longer password.\n";
					print "\n";
					next;
				}
				if (++$tries < 2 && $tmp =~ /^[a-z]+$/) {
					print <<ETX;
Please don't use an all-lower case password.
Unusual capitalization, control characters, or digits are suggested.

ETX
					next;
				}
				$_ = &getpass("Retype new password: ");
				last if $_ eq $tmp;
				if ($interactive) {
				    print "\nThe two passwords did not match; please type the password again\n";
				}
				else {
				    print STDERR "$0: Password mismatch.\n";
				}
				print "\n";
			}
	        	$passwd{$login} = &encrypt($_);
			print "\n";
		}

		# Primary group
		if ($opt_g) {
			if (defined($group{$opt_g})) {
	        		$pwgid{$login} = $group{$opt_g};
			}
			else {
	        		$pwgid{$login} = $opt_g;
			}
		}
		else {
			if (defined($fields{"GROUP"}) && 
				$fields{"GROUP"} =~ /^\d+$/) {
				$fields{"GROUP"} = $gid{$fields{"GROUP"}} 
					if defined($gid{$fields{"GROUP"}});
			}
			for (;;) {
				print "Enter primary group as either a name defined in the group file\n";
				print "or a numeric gid.\n\n";
				$_ = &query("Primary group (? for list of choices)", $fields{"GROUP"}, ());
				print "\n";
				&listgroups && print "\n" && next if $_ eq "?";
				last if defined($group{$_});
				last if /^[0-9]+$/;
				print <<ETX;
Group name `$_' must be either defined in the list of groups (add new
groups using the `addgroup' command) or a numeric group id.

ETX
				$tmp = &query("Add new group $_ to groups file?", "no");
				if ($tmp =~ /^[yY]/) {
					system("addgroup $_") && 
						die "$0: addgroup $_ failed\n";
					&parsegroup;
					last;
				}
			}
			if (defined($group{$_})) {
				$pwgid{$login} = $group{$_};
			}
			else {
	        		$pwgid{$login} = $_;
			}
			$fields{"GROUP"} = $_;
		}

		# GECOS field
		if ($opt_G) {
			$gecos{$login} = $opt_G;
		}
		else {
			local ($name, $office, $ophone, $hphone) = 
					split(/,/, $gecos{$login}, 4);
			for (;;) {
				print <<ETX;
You will be prompted for the user's full name, office, office phone,
and home phone.  The values for these fields may not contain commas.
To leave a field blank, simply press <Enter> when prompted for the value.

ETX
				$name = &query("Full name", $name);
				print "\n";
				$office = &query("Office", $office);
				print "\n";
				$ophone = &query("Office Phone", $ophone);
				print "\n";
				$hphone = &query("Home Phone", $hphone);
				print "\n";
				$_ = join('', $name, $office, $ophone, $hphone);
				if (/,/ || /:/) {
					print "No commas or colons allowed in these fields.\n\n";
					next;
				}
				$gecos{$login} = join(',', $name, $office, 
							$ophone, $hphone);
				last;
			}
		}

		# Home directory
		if ($opt_h) {
			$opt_h .= "/" if $opt_h =~ /[^\/]$/;
	        	$home_dir{$login} = "$opt_h$login";
		}
		elsif ($opt_H) {
	        	$home_dir{$login} = "$opt_H";
		}
		else {
			for (;;) {
				$_ = &query("Home directory", 
					defined($fields{"HDIR"}) ? 
					"$fields{'HDIR'}$login" : "");
				print "\n";
	 		       	$home_dir{$login} = $_;
				if (/:/) {
					print "No colons allowed in password file fields.\n\n";
					next;
				}
				if (-d $_ && $interactive) {
					print "Directory ($_) exists.\n";
					$_ = &query ("Use anyway?", "no");
					print "\n";
					last if /^[yY]/;
					next;
				}
				s#(.*/)[^/]*#$1#;
				last if -d $_;
				$pdir = $_;
				print "Parent directory ($_) doesn't exist.\n";
				$_ = &query ("Use anyway?", "no");
				print "\n";
				if (/^[yY]/) {
					$pdir =~ s#/$##;
					system("mkdir -p $pdir") && 
						die "$0: mkdir $pdir failed\n";
					last;
				}
			}
		}
		$fields{"HDIR"} = $home_dir{$login};
		$fields{"HDIR"} =~ s#(.*/)[^/]+$#$1#;

		# Shell
		if ($opt_s) {
	        	$shell{$login} = $opt_s;
		}
		else {
			for (;;) {
				$_ = &query("Login shell (? for list of choices)", $fields{"SHELL"}, ());
				print "\n";
				if ($_ eq "?") {
					if (!open(TMP, "/etc/shells")) {
						print STDERR "$0: can't open /etc/shells for a listing of shells\n";
						next;
					}
					while (<TMP>) {
						chop;
						next if /^#/;
						next if /^$/;
						push (@_, $_);
					}
					close(TMP);
					&printlist("Available Shells:\n", @_);
					print "\n";
					next;
				}
				s/:/_/;		# just in case
				$_ = "/bin/" . $_ if !/^\//;
				last if -x $_;	# let it pass if executable
				print "Invalid shell `$_'\n";
			}
	        	$fields{"SHELL"} = $shell{$login} = $_;
		}

		if ($interactive) {
			local ($name, $office, $ophone, $hphone) = 
				split(/,/, $gecos{$login}, 4);
			print "Login name:\t$login\n";
			print "Primary group:\t$fields{\"GROUP\"}\n";
			print "Full name:\t$name\n";
			print "Office:\t\t$office\n";
			print "Office phone:\t$ophone\n";
			print "Home phone:\t$hphone\n";
			print "Home directory:\t$home_dir{$login}\n";
			print "Shell:\t\t$shell{$login}\n";
			print "\n";
			$_ = &query("Add this user to the password file?", "yes");
			print "\n";
			if (!/^[yY]/) {
				undef $uid{$login{$login}};
				undef $login{$login};
				next;
			}
		}

		print "Cached passwd entry for new user: $login (uid: $uid)\n";
		print "  the entry will be written when you are done adding users.\n\n";
		$changedpw++;

		# Add user to other groups?
		for (;$interactive;) {
			print <<ETX;
Generally, users are members of a single group.  Add users to special
groups for special privileges; here is a list:
    dialer     users who can access modems
    wheel      users who can ever become superuser

ETX
			$_ = &query("Add `$login' to other secondary groups?", 
					"no");
			print "\n";
			/^[nN]/ && last;
			$_ = &query("Enter group name (? for list)");
			print "\n";
			/^\?$/ && do { &listgroups; print "\n"; next; };
			/^$/ && next;
			if (!defined($group{$_})) {
				print STDERR "$0: specified group `$_' does not exist\n\n";
				next;
			}
			if (($members{$_} =~ /$login/) || 	
					($group{$_} == $pwgid{$login})) {
				print STDERR "$0: `$login' is already a member of group `$_'\n\n";
				next;
			}
			$members{$_} =~ s/[ \t]+//g;
			if ($members{$_} =~ /^[ \t]*$/){
				$members{$_} = $login;
			}
			else {
				$members{$_} .= ",$login";
			}
			$changedgrp++;
		}

		# create home directory and copy in files...
		if ($changedpw && ! -d $home_dir{$login}) {
			print "Copying $SKEL files into $home_dir{$login}..." 
				if $opt_v;
			mkdir ($home_dir{$login}, $opt_m) || 
				die "\n$0: can't mkdir $home_dir{$login}: $!\n";
			chown ($login{$login}, $pwgid{$login}, 
				$home_dir{$login}) ||
				die "\n$0: can't chown $home_dir{$login}: $!\n";
			&install_skel ($SKEL, $home_dir{$login}, $login{$login},
			    $pwgid{$login}, 'default', sprintf("0%o",$opt_m || 0755));

			print "DONE.\n" if $opt_v;
		}
	}

	if ($interactive) {
		$_ = &query("Do you wish to add additional users?", "no");
		if (/^[yY]/) {
			print "\n";
			undef $login if defined($login{$login});
			goto restart;
		}
	}
}

$allowsig=0;	# Don't let us get interrupted

&writepasswd if $changedpw;
&writesimple($PASSWD_DEF, %fields) if $changedpw && !$opt_n;
&writegroup if $changedgrp;

$allowsig=1;	# Signals are OK again

print "$0: Successful Completion (new password and group files written)\n\n";

exit 0;

sub handler {
	local($sig) = @_;

	if ($allowsig) {
		print "\n$0: caught SIG$sig -- cleaning up\n";
		system("stty echo");
		exit 1;
	}
}

### install_skel ($skel, $where, $user, $group, $class, $mode)
###
### We just have to run a make in the correct directory and it
### installs the skel files for us.
sub install_skel {
    local ($skel, $where, $user, $group, $class, $mode) = @_;
    local ($status, @output);
    local (*CMD, $pid);
    local (@cmd) = ('make', "HOMEDIR=$where",
			    "OWNER=$user",
			    "GROUP=$group",
			    "LOGIN_CLASS=$class",
			    "DIRMODE=$mode", 'install');

    $pid = open(CMD, '-|');
    die "pipe: $!\n" unless defined $pid;

    if ($pid == 0) {
	### child
	open(STDERR, '>& STDOUT');
	chdir($skel) || die "chdir $skel: $!\n";
	exec @cmd;
	die "exec @cmd: $!\n";
    }
    else {
	### parent
	### collect @output and status from child
	@output = <CMD>;
	close(CMD);
	$status = ($? >> 8) & 0xff;
    }
    die join('', @output) unless $status == 0;
}
