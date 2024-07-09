#!/usr/bin/perl

#	BSDI	x.pl,v 1.1 1998/11/13 16:54:59 polk Exp

$|=1;	# unbuffer stdout

$SLEEPTIME=5;	# at least 5 seconds between clears

$SAVETEXT="";
$DEFHELP="Sorry, no help on this topic";
$HELP=$DEFHELP;
$XSTARTED=0;

sub v_setterm {
	if ($XSTARTED) {
		return;
	}
	$DEFHELP="";
	$HELP=$DEFHELP;
	$XSTARTED=1;
	print "Starting X11 based interactive installation utility.\n\n";
	if (pipe(X0, TOX) == 0 || pipe(FROMX, X1) == 0) {
		die "pipe";
	}
	$SFH = select(X0);
	$| = 1;
	select(X1);
	$| = 1;
	select(TOX);
	$| = 1;
	select(FROMX);
	$| = 1;
	select($SFH);

	unless (fork()) {
		close (TOX);
		close (FROMX);
		if (!open(STDOUT, ">&X1")) {
			die "duping to stdout";
		}
		if (!open(STDIN, "<&X0")) {
			die "duping to stdin";
		}
		exec "xinstall -geom ~0~0";
		exit 0
	};
	close(X0);
	close(X1);
	return;
}

&v_setterm();

$SLEEPOUT = 0;

sub v_sleep {
	if (time() < $SLEEPOUT) {
		sleep($SLEEPOUT - time());
	}
	$SLEEPOUT = time() + $SLEEPTIME;
}

sub v_clear {
	print TOX "CLEAR\n";
}

sub v_home {
	print TOX "CLEAR\n";
}

sub v_reset {
	print TOX "CLEAR\n";
	print TOX "HIDE\n";
}

sub vquery {
        local ($query, $default, @choices, $prompt, $tmp, $_) = @_;

	$prompt = "$query ";
	$fullquery=1;

	&vdisplay("query");

	$SLEEPOUT=0;

	if ($query eq "ASKPASS") {
		print TOX "QUESTION New password:\n";
		print TOX "ASKPASS\n";
	} elsif ($query eq "LIST") {
		print TOX "LIST\n";
	} elsif ($#choices > 3) {
		if ($query ne "") {
			print TOX "TEXT <P>$query</P>\n";
		}
		print TOX "ITEM\n";
		for $tmp (@choices) {
			print TOX "ITEM \"$tmp\"\n";
		}
		print TOX "LIST\n";
	} elsif ($#choices >= 0) {
		if ($query ne "") {
			print TOX "TEXT <P>$query</P>\n";
		}
		print TOX "SELECTION";
		for $tmp (@choices) {
			if ($tmp eq $default) {
				print TOX " \"+$tmp\"";
			} else {
				print TOX " \"$tmp\"";
			}
		}
		print TOX "\n";
	} else {
		print TOX "QUESTION $query\n";
		print TOX "ASK $default\n";
	}
	$_ = <FROMX>;
	chop;

	$HELP=$DEFHELP;
        return $_;
}

sub vpressenter {
    &vquery("", "", "+Press ENTER to continue");
}

sub vaskpass {
    &vquery("ASKPASS");
}

sub vaddtext {
        local ($newt) = @_;
	$TEXT=$TEXT."\n$newt";

	print TOX "TEXT <PLAIN>\n";
	$vdm="PLAIN";

	@lines = split(/\n/, $newt);
	foreach $i (@lines) {
		if ($i eq "") {
			$i="<P></P>";
		}
		if ($i =~ /^\s/) {
			if ($vdm eq "PLAIN") {
				print TOX "TEXT <PRE>\n";
				$vdm="PRE";
			}
		} else {
			if ($vdm eq "PRE") {
				print TOX "TEXT </PRE>\n";
				$vdm="PLAIN";
			}
		}
		print TOX "TEXT $i\n";
	}
	print TOX "DISPLAY\n"
}

sub vdisplay {
        local ($modes) = @_;


	print TOX "SHOW\n";
	print TOX "CLEAR\n";
	print TOX "TITLE $TITLE\n";
	print TOX "TEXT <PLAIN>\n";
	$vdm="PLAIN";

	if ($HELP ne "") {
		@lines = split(/\n/, $HELP);
		foreach $i (@lines) {
			print TOX "HELP $i\n";
		}
	}

	if ($COMMENTS ne "") {
		@lines = split(/\n/, $COMMENTS);
		foreach $i (@lines) {
			if ($i eq "") {
				$i="<P></P>";
			}
			print TOX "TEXT <I>$i</I>\n";
		}
		print TOX "TEXT <P></P>\n";
	}
	@lines = split(/\n/, $TEXT);
	foreach $i (@lines) {
		if ($i eq "") {
			$i="<P></P>";
		}
		if ($i =~ /^\s/) {
			if ($vdm eq "PLAIN") {
				print TOX "TEXT <PRE>\n";
				$vdm="PRE";
			}
		} else {
			if ($vdm eq "PRE") {
				print TOX "TEXT </PRE>\n";
				$vdm="PLAIN";
			}
		}
		print TOX "TEXT $i\n";
	}
	if ($modes =~ /query/) {
		return;
	}
	print TOX "DISPLAY\n";
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

sub vexplain {
	local ($title) = @_;

	if ($ENV{'V_EXPLAINED'} ne "yes") {
		$ENV{'V_EXPLAINED'} = "yes";
		$TEXT=
"$title will provide you with a series of questions.
If there are only a few choices you see those choices displayed
as buttons at the bottom of this screen.  You may either use the
mouse to select your choice, or use the &lt;TAB&gt; key to select
the button of your choice and the &lt;ENTER&gt; key to select the
highlighted choice.

If there are more than a few choices, a list of choices will appear.
Select your choice with the mouse and then press the ENTER button.
Use the &lt;TAB&gt; to select either the list or the ENTER button.
When the list is selected you may use the arrow keys to choose your
entry.

In places where additional help is available, a HELP button will
appear in the upper right corner.  Pressing this button will provide
additional help on the current question.";
		$HELP="That's right, this would be a HELP screen.";
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
	local ($prompt, $default) = @_;
	$INVALID="";
	$COMMENTS="";
	$HELP=
"IP addresses are expressed as a \"dotted quad\", that is, 4 numbers (each
between 0 and 255 inclusive) separated by periods (e.g, 192.0.0.23).  The
first three numbers are usually the network number (assigned to your site
by either the NIC (network information center) or your service provider).
The last number is the host address within your network and is usually
chosen locally.  The host addresses of 0 and 255 have special meanings and
should never be used for network interfaces.  The addresses between 1 and
254 (inclusive) are typically available for assignment.";
	print TOX "QUESTION $prompt\n";
	print TOX "ASKIPADDR $default\n";
	$_ = <FROMX>;
	chop;
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
