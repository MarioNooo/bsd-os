#!/usr/bin/perl

#	BSDI	v.pl,v 1.15 2000/03/07 21:47:47 polk Exp

$|=1;	# unbuffer stdout

$SLEEPTIME=5;	# at least 5 seconds between clears

$SAVETEXT="";
$DEFHELP="Sorry, no help on this topic";
$HELP=$DEFHELP;

sub v_setterm {
	if ($ENV{'TERM'} =~ /^bsdos-pc/) {
		$TNOCURSOR="[15;14_";
		$TCURSOR="[_";
		$TULC="É";
		$TURC="»";
		$TLLC="È";
		$TLRC="¼";
		$TLST="Ç";
		$TRST="¶";
		$TDBL="Í";
		$TSNGL="Ä";
		$TSIDE="º";
		$TCOL=80;
		if ($ENV{'TERM'} =~ /mono/) {
			$TBLUE="[=0G";
			$TBLACK="[=0G";
			$TSO="[=15F";
			$TSE="[=15F";
		} else {
			$TBLUE="[=1G";
			$TBLACK="[=0G";
			$TSO="[=14F";
			$TSE="[=7F";
		}
		#
		# These 4 must exist to do visual mode
		#
		$THOME="[H";
		$TSAVEC="7";
		$TRESTOREC="8";
		$TCLEAR="c";
	} elsif ($ENV{'TERM'} =~ /^xterm/ || $ENV{'TERM'} =~ /^vt100/) {
		$TNOCURSOR="";
		$TCURSOR="";
		$TULC="+";
		$TURC="+";
		$TLLC="+";
		$TLRC="+";
		$TLST="+";
		$TRST="+";
		$TDBL="=";
		$TSNGL="-";
		$TSIDE="|";
		$TCOL=80;
		$TBLUE="";
		$TBLACK="";
		$TSO="[1m";
		$TSE="[m";
		#
		# These 4 must exist to do visual mode
		#
		$THOME="[H";
		$TSAVEC="7";
		$TRESTOREC="8";
		$TCLEAR="[H[2J";
	} else {
		$TBLUE="";
		$TBLACK="";
		$TNOCURSOR="";
		$TCURSOR="";
		$TULC="+";
		$TURC="+";
		$TLLC="+";
		$TLRC="+";
		$TLST="+";
		$TRST="+";
		$TDBL="-";
		$TSNGL="-";
		$TSIDE="|";
		chop($TCOL=`tput co`);
		$TSO=`tput so`;
		$TSE=`tput se`;
		#
		# These 4 must exist to do visual mode
		#
		$THOME=`tput ho`;
		$TSAVEC=`tput sc`;
		$TRESTOREC=`tput rc`;
		$TCLEAR=`tput cl`;
	}
	$TLIN=24;


	if ($TLIN < 24 || $TCOL < 80 || $TRESTOREC eq "" || $TSAVEC eq "" || $THOME eq "" || $TCLEAR eq "") {
		$DUMBTERM=1;
		$TSO="";
		$TSE="";
		$THOME="";
		$TSAVEC="";
		$TRESTOREC="";
		$TCLEAR="";
		$TULC="";
		$TURC="";
		$TLLC="";
		$TLRC="";
		$TLST="";
		$TRST="";
		$TDBL="";
		$TSNGL="";
		$TSIDE="";
	}

	$SO="$TBLUE$TSO";
	$SE="$TBLACK$TSE";
}
&v_setterm();

$SLEEPOUT = 0;

sub v_sleep {
	if (time() < $SLEEPOUT) {
		sleep($SLEEPOUT - time());
	}
	$SLEEPOUT = time() + $SLEEPTIME;
}

sub v_clear { &v_sleep(); print "$TNOCURSOR$SO$TCLEAR"; }
sub v_home { $SLEEPOUT = time() + $SLEEPTIME; print "$THOME"; }
sub v_reset { &v_sleep(); print "$TCURSOR$SE$TBLACK$TCLEAR"; }

$V_TOP="";
$V_MID="";
$V_BOT="";
sub v_top {
	if ($V_TOP eq "") {
		$V_TOP=$TULC;
		for ($i = 0; $i < 77; ++$i) {
			$V_TOP.=$TDBL;
		}
		$V_TOP.=$TURC;
		$V_TOP.="\n";
	}
	printf $V_TOP;
}
sub v_middle {
	if ($V_MID eq "") {
		$V_MID=$TLST;
		for ($i = 0; $i < 77; ++$i) {
			$V_MID.=$TSNGL;
		}
		$V_MID.=$TRST;
		$V_MID.="\n";
	}
	printf $V_MID;
}
sub v_bottom {
	if ($V_BOT eq "") {
		$V_BOT=$TLLC;
		for ($i = 0; $i < 77; ++$i) {
			$V_BOT.=$TDBL;
		}
		$V_BOT.=$TLRC;
	}
	printf $V_BOT;
}

sub vpressenter {
    &vquery("Press <ENTER> to continue");
}

sub vquery {
        local ($query, $default, @choices, $prompt, $tmp, $_) = @_;

        if ($default ne "") {
                $prompt = "$query [$default] ";
        }
        else {
                $prompt = "$query ";
        }
	$fullquery=1;

        for (;;) {
		if ($fullquery) {
			$fullquery=0;
			if ($DUMBTERM) {
				if ($INVALID ne "") {
					print "$INVALID\n";
					$INVALID="";
				} else {
					$i = "$COMMENTS\n$TEXT\n\n";
					$i =~ s/<[Ii]>/"/g;
					$i =~ s/<\/[Ii]>/"/g;
					$i =~ s/<[^>]*>//g;
					$i =~ s/&lt;/</g;
					$i =~ s/&gt;/>/g;
					print "$i";
				}
			} else {
				&v_clear();
				&v_top();
				&v_home();
				printf("$TULC$TDBL %s \n", $TITLE);
				&v_middle();
				$NL=2;
				@lines = split(/\n/, $COMMENTS);
				foreach $i (@lines) {
					$i =~ s/<[Ii]>/"/g;
					$i =~ s/<\/[Ii]>/"/g;
					$i =~ s/<[^>]*>//g;
					$i =~ s/&lt;/</g;
					$i =~ s/&gt;/>/g;
					++$NL;
					printf("$TSIDE %-75s $TSIDE\n", $i);
				}
				if ( $COMMENTS =~ /..*/) {
					++$NL;
					&v_middle();
				}
				@lines = split(/\n/, $TEXT);
				foreach $i (@lines) {
					$i =~ s/<[Ii]>/"/g;
					$i =~ s/<\/[Ii]>/"/g;
					$i =~ s/<[^>]*>//g;
					$i =~ s/&lt;/</g;
					$i =~ s/&gt;/>/g;
					++$NL;
					printf("$TSIDE %-75s $TSIDE\n", $i);
				}
				++$NL;
				printf("$TSIDE %-75s $TSIDE\n", "");
				print "$TSAVEC";
				++$NL;
				printf("$TSIDE %-75s $TSIDE\n", "");
				++$NL;
				printf("$TSIDE %-75s $TSIDE\n", $INVALID);
				$INVALID="";
				++$NL;
				printf("$TSIDE %-75s $TSIDE\n", "");
				++$NL;
				printf("$TSIDE %-75s $TSIDE\n", "");

				while ($NL < $TLIN-1) {
					++$NL;
					printf("$TSIDE %-75s $TSIDE\n", "");
				}

				&v_bottom();
			}
		}
		$SLEEPOUT=0;
		if ($DUMBTERM) {
			print $prompt;
			$_ = <STDIN> || die "got EOF -- exiting.\n";
		} else {
			print "$TRESTOREC";
			printf("$TSIDE %75s\r", "");
			printf("$TSIDE %s%s", $prompt, $TCURSOR);
			$_ = <STDIN> || die "got EOF -- exiting.\n";
			print "$TNOCURSOR";
		}
                chop;
		if ($SAVETEXT ne "") {
			return "";
		}
                if ($default ne "" && /^$/) {
			$HELP=$DEFHELP;
                        return $default;
                }
		if (/^!shell/) {
			&v_reset();
			print "Spawning shell...\n";
			system("/bin/sh");
			$fullquery = 1;
			next
		}
		if (/^help/) {
			$SAVETEXT=$TEXT;
			$TEXT=$HELP;
			&vpressenter();
			$TEXT=$SAVETEXT;
			$SAVETEXT="";
			$fullquery = 1;
			next
		}
                last if $#choices < 0;
                for $tmp (@choices) {
			if (/^$tmp$/i) {
				$HELP=$DEFHELP;
				return $tmp;
			}
			if ($tmp eq "yes" && /^y/i) {
				$HELP=$DEFHELP;
				return $tmp;
			}
			if ($tmp eq "no" && /^n/i) {
				$HELP=$DEFHELP;
				return $tmp;
			}
                }
		print "\n$TSIDE Invalid response: $_";
		print "\n$TSIDE Valid responses are: ";
                for $tmp (@choices) {
                        print "$tmp ";
                }
		if ($DUMBTERM) {
			print "\n\n";
		}
        }
	$HELP=$DEFHELP;
        return $_;
}

sub vaddtext {
        local ($newt) = @_;
	$TEXT=$TEXT."\n$newt";

	if ($DUMBTERM) {
		print "$newt\n";
	} else {
		&vdisplay("refresh");
	}
}

sub vdisplay {
        local ($modes) = @_;
	if ($DUMBTERM) {
		print "$COMMENTS\n";
		print "$TEXT\n";
		return 1;
	}
	if ($modes =~ /refresh/) {
		&v_home();
	} else {
		&v_clear();
	}
	&v_top();
	&v_home();
	printf("$TULC$TDBL %s \n", $TITLE);
	&v_middle();
	$NL=2;
	@lines = split(/\n/, $COMMENTS);
	foreach $i (@lines) {
		$i =~ s/<[Ii]>/"/g;
		$i =~ s/<\/[Ii]>/"/g;
		$i =~ s/<[^>]*>//g;
		$i =~ s/&lt;/</g;
		$i =~ s/&gt;/>/g;
		++$NL;
		printf("$TSIDE %-75s $TSIDE\n", $i);
	}
	if ( $COMMENTS =~ /..*/) {
		++$NL;
		&v_middle();
	}
	@lines = split(/\n/, $TEXT);
	foreach $i (@lines) {
		$i =~ s/<[Ii]>/"/g;
		$i =~ s/<\/[Ii]>/"/g;
		$i =~ s/<[^>]*>//g;
		$i =~ s/&lt;/</g;
		$i =~ s/&gt;/>/g;
		++$NL;
		printf("$TSIDE %-75s $TSIDE\n", $i);
	}
	if ($modes =~ /savecursor/) {
		++$NL;
		printf("$TSIDE %-75s $TSIDE\n", "");
		++$NL;
		printf("$TSIDE $TSAVEC%-75s $TSIDE\n", "");
	}
	while ($NL < $TLIN-1) {
		++$NL;
		printf("$TSIDE %-75s $TSIDE\n", "");
	}
	&v_bottom();
	if ($modes =~ /savecursor/) {
                print "$TRESTOREC";
	}
}

sub v_addlist {
	local (@list) = @_;

	$col = 6;
	foreach $file (@list) {
		if (int(76 / (length($file)+1)) < $col) {
			$col = int(76 /(length($file)+1));
		}
	}
	$fw = int(76 / $col) - 1;

	$m = int(($#list+$col)/$col);
	for ($i = 0; $i < $m; ++$i) {
		for ($j = 0; $j < $col - 1; ++$j) {
			$TEXT.=sprintf("%-${fw}s ", @list[$i+$j*$m]);
		}
		$TEXT.="@list[$i+$j*$m]\n";
	}
}

$EXPLAIN_INSTALL_HACK="";
sub vexplain {
	local ($title) = @_;

	if ($ENV{'V_EXPLAINED'} ne "yes") {
		$ENV{'V_EXPLAINED'} = "yes";
		$TEXT="$EXPLAIN_INSTALL_HACK".
"$title will provide you with a series of questions.
When a `default answer' is available, it will follow the question
in square brackets.  For example, the question:

        Would you like coffee, tea, or milk? [coffee]:

has the default answer `coffee'.  Accept the default (without any
extra typing!) by pressing the &lt;Enter&gt; key -- or type your answer
and then press &lt;Enter&gt;.

Use the &lt;Backspace&gt; key to erase and aid correction of any mistyped
answers -- before you press &lt;Enter&gt;.  Generally, once you press
&lt;Enter&gt; you move onto the next question.";
		$HELP="You need to press the key marked &lt;ENTER&gt;";
		&vpressenter();

		$TEXT=
"In most places where a question is asked you can request more information
by typing \"help\" followed by &lt;ENTER&gt; (do not type the \" marks).";
		$HELP="That's right, this would be a HELP screen";
		&vpressenter();
	}
}

# Query for a hostname
sub vquery_name {
	local ($prompt, $default) = @_;
	$INVALID="";
	$COMMENTS="";
	for (;;) {
	        $_ = &vquery($prompt, $default);
		$INVALID="";
        	last if /^[-A-Za-z0-9_\.]+$/;
		last if /^\?$/ || /^$/;
		$INVALID="Invalid name";
		$COMMENTS=
"Host/domain names must consist of letters, digits, `-', `.', and `_' only.";
	}
	$COMMENTS="";
	return $_;
}

# Query for an address
sub vquery_addr {
	local ($prompt, $default, $min_fields, $max_fields, 
		$pattern, $fields) = @_;
	$min_fields = 4 if ! $min_fields;
	$max_fields = 4 if ! $max_fields;
	$pattern = "\\d{1,3}\\." x ($min_fields - 1);
	$pattern .= "\\d{1,3}";
	$INVALID="";
	$COMMENTS="";
	for (;;) {
		$HELP=
"IP addresses are expressed as a \"dotted quad\", that is, 4 numbers (each
between 0 and 255 inclusive) separated by periods (e.g, 192.0.0.23).  The
first three numbers are usually the network number (assigned to your site
by either the NIC (network information center) or your service provider).
The last number is the host address within your network and is usually
chosen locally.  The host addresses of 0 and 255 have special meanings and
should never be used for network interfaces.  The addresses between 1 and
254 (inclusive) are typically available for assignment.";
	        $_ = &vquery($prompt, $default);
		last if /^none$/;
		last if /^done$/ && $_ eq $default;
		$INVALID="";
		$fields = split(/\./);
		last if /^$pattern$/ ||
			($min_fields < $max_fields &&
			 $fields <= $max_fields && /^$pattern/);
		$INVALID="Invalid address";
		if ($min_fields == 4) {
			$COMMENTS=
"IP addresses consist of 4 numbers (each between 0 and 255 inclusive) 
separated by periods (e.g., 192.0.0.23).";
		} else {
			$COMMENTS=
"IP network addresses consist of up to 3 numbers (each between 0 and 255
inclusive) separated by periods. The numbers are the network portion of 
IP addresses on your local network.  For example, a class C network 
consists of three numbers e.g.: 192.0.0";
		}
	}
	$COMMENTS="";
	return $_;
}

# Query for an address
sub vquery_mask {
	local ($prompt, $default) = @_;

	$INVALID="";
	for (;;) {
		$HELP=
"A \"netmask\" needs to be specified for the network.

A netmask determines which part of an IP address represents the network
as opposed to the individual host.  Most sites use the final number of
the IP address as the host.  This implies the default netmask of
255.255.255.0.  $INSTALL requires that the netmask be
defined as a standard dotted quad with the number expressed in decimal
or a 16 digit hexadecimal number (e.g., 0xffffff00).  If you are not
certain what the netmask should be, enter \"default\" to use the
default netmask for this address.";

		$_ = &vquery($prompt, $default);
		last if /^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$/;
		last if /^0x[0-9a-fA-F]{8}$/;
		last if /^default$/;
		$INVALID = "Invalid entry: $_";
	}
	if ($_ eq "default") {
		return "";
	}
	return $_;
}

require '/usr/libdata/adm/query.pl';
1;
