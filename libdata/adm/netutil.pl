#!/usr/bin/perl

# Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved.

# Copyright (c) 1994 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	netutil.pl,v 1.31 2001/10/03 17:30:41 polk Exp


$NETSTART_PROTO = "/etc/netstart.proto";
@NETSTART_QUERYLIST = ("ether", "token_ring", "fddi", "802.11");
$NETOUT = 15;
$NOCAR_RETRY = 3;

if (defined $MAXIM) {
    $NETSTART = "/etc/netstart";
    $NETSTART_TMP = "$NETSTART.tmp";
} else {
    if ( ! -f "/.install-floppy" ) {
	$NETSTART = "/etc/netstart";
	$NETSTART_TMP = "$NETSTART.tmp";
	$INSTALL="Network Config";
	require "/usr/libdata/adm/v.pl";
	if (open(IN, "sysctl -n kern.hostname |")) {
		$_ = <IN>;
		chop;
		$netutil{"HOSTNAME"} = $_;
		close(IN);
	}
    } else {
	$NETSTART = "/a/etc/netstart";
	$NETSTART_TMP = "$NETSTART.tmp";
    }
}

sub query_interfaces {
    local($i, $iface, %linktypes);

    @networks = ();
    @configured_networks = ();
    %inet = ();
    %linktype = ();
    %media_active = ();
    %media_choices = ();
    %media_current = ();
    %media_options = ();
    %media_status = ();

    foreach $_ (@_) {
	$linktypes{$_} = $_;
    }

    if (open(IFCONFIG, "ifconfig -am|") == 0) {
	&v_reset();
	die "unable to determine interfaces present.\n";
    }
    while (<IFCONFIG>) {
	if (/^([a-zA-Z]+[0-9]+): flags=/) {
	    $iface = $1;
	} elsif (/^\s+link\s+type\s+(\S+)\s+.*$/) {

	    # If link types specified, only return those
	    if (%linktypes && !defined($linktypes{$1})) {
		undef $iface;
		next;
	    }

	    push(@networks, $iface);
	    $linktype{$iface} = $1;
	} elsif (defined($iface)) {
	    if (/^\s+media\s+options\s+(.*)\s*$/) {
		# Ignore choices with loopback
		local(@options) = grep(!/^loopback$/, split(/\s+/, $1));
		$media_options{$iface} = 
		    join(' ', @options,
			 split(' ', $media_options{$iface}));
	    } elsif (/^\s+media\s+choice\s+(.*)\s*$/) {
		# Ignore choices with loopback
		if ($1 =~ /\bloopback\b/) {
		    next;
		}
		if (defined($media_choices{$iface})) {
		    $media_choices{$iface} .= '%' . $1;
		} else {
		    $media_choices{$iface} = $1;
		}
	    } elsif (/^\s+media\s+([^\(]+)(\(([^\)]+)\))?\s+status\s+(.*)$/) {
		$media_current{$iface} = join(' ', split(/\s+/, $1));
		if ($3 ne "") {
		    $media_active{$iface} = join(' ', split(/\s+/, $3));
		} else {
		    $media_active{$iface} = undef;
		}
		$media_status{$iface} = join(' ', split(/\s+/, $4));
	    } elsif (/^\s+media\s+([^\(]+)(\(([^\)]+)\))?$/) {
		$media_current{$iface} = join(' ', split(/\s+/, $1));
		if ($3 ne "") {
		    $media_active{$iface} = join(' ', split(/\s+/, $3));
		} else {
		    $media_active{$iface} = undef;
		}
		$media_status{$iface} = undef;
	    } elsif (/^\s+inet\s+(.*)$/) {
		local ($tmp) = $1;
		$tmp =~ s/-->/destination/;
		if (defined($inet{$iface})) {
		    $inet{$iface} .= '%' . $tmp;
		} else {
		    $inet{$iface} = $tmp;
		    $inet{$iface} =~ /^([^\s]+)\s/;
		    if ($netutil{"LADDR"} eq undef) {
		    	$netutil{"LADDR"} = $1;
		    }
		    if ($netutil{"IFACE"} eq undef) {
		    	$netutil{"IFACE"} = $iface;
		    }
		}
	    }
	}
    }
    close(IFCONFIG);

    # If at least one interface is configured with an IP
    # address life is alot simpler.
    @configured_networks = grep(defined($inet{$_}), @networks);
}

sub inet_aton {
    local($a) = @_;

    if ($a =~ /^0x[0-9a-fA-F]{8}$/) { 
	return hex($a);
    } elsif ($a =~ /^([0-9]*)\.([0-9]*)\.([0-9]*)\.([0-9]*)$/) {
	return (($1 << 24) | ($2 << 16) | ($3 << 8) | $4);
    } elsif ($a =~ /^([0-9]*)\.([0-9]*)\.([0-9]*)$/) {          
	return (($1 << 24) | ($2 << 16) | $3);  
    } elsif ($a =~ /^([0-9]*)\.([0-9]*)$/) {          
	return (($1 << 24) | $2);  
    } elsif ($a =~ /^([0-9]*)$/) {          
	return ($1);             
    }
}

sub same_subnet {
    local($laddr, $raddr, $netmask) = @_;

    return (((&inet_aton($laddr) ^ &inet_aton($raddr)) 
	     & &inet_aton($netmask)) == 0);
}

sub same_host {
    local($laddr, $raddr, $netmask) = @_;

    return (((&inet_aton($laddr) ^ &inet_aton($raddr)) 
	     & ~&inet_aton($netmask)) == 0);
}

sub xlate_media {
    local($media) = @_;

    if ($media eq "") {
	return $media;
    }

    return "media " . join(',', split(/\s+/, $media));
}

sub unconfigure_interface {
    local($iface, $laddr, $netmask, $media, $router) = @_;

    if ($router ne "") {
	system("route -n delete default > /dev/null 2>&1");
    }

    if ($iface ne "") {
	system("ifconfig $iface -remove > /dev/null 2>&1");
    }
}

sub configure_interface {
    local($iface, $laddr, $netmask, $media, $router) = @_;

    &unconfigure_interface($iface, $laddr, $netmask, $media, $router);

    system("ifconfig $iface set $laddr netmask $netmask " . &xlate_media($media));
    if ($? != 0) {
	&v_reset();
	die "Unable to configure interface $iface";
    }
	
    if ($router ne "") {
	system("route -n add default $router");
	if ($? != 0) {
	    &v_reset();
	    die "Unable to add default route";
	}
    }
}

sub command_alarm {
    kill(1, $pid);
}

sub run_command {
    local($pid, @text);

    $SIG{'ALRM'} = 'command_alarm';
    if (($pid = open(CMD, "-|")) == 0) {
	open(STDERR, ">&STDOUT");
	exec(@_) || exit(-1);
    }
    if (!defined($pid)) {
	&v_reset();
	die("Unable to execute @_: $!");
    }
    alarm($NETOUT);
    chop(@text = <CMD>);
    close(CMD);
    unshift(@text, $?);

    alarm(0);
    $SIG{'ALRM'} = 'DEFAULT';

    return @text;
}

sub netstart_write {
    local($iface, $laddr, $netmask, $destination, $router);
    local(@proto);

    # Query interfaces again
    &query_interfaces(@NETSTART_QUERYLIST);

    open (TMP, ">$NETSTART_TMP") || 
	die "$0: can't open $NETSTART_TMP: $!\n";
    print TMP <<ETX;
#
# netstart - configure network daemons, interfaces, and routes
#

ETX

    printf TMP qq/hostname=%s\n/, $netutil{"HOSTNAME"};
    printf TMP qq/nis_domain=\n/;
    printf TMP qq/interfaces="%s"\n/, join(' ', @configured_networks);
    printf TMP qq/primary="%s"\n/, $netutil{"IFACE"};
    printf TMP qq/defroute="%s"\n/, $netutil{"ROUTER"};
    foreach $iface (@configured_networks) {
	local(@addrs);

	@addrs = split('%', $inet{$iface});
	$_ = pop(@addrs);
	
	if (/^\s*([\d.]+)\s+netmask\s+([\d.]+)\s+broadcast\s+([\d.]+)\s?$/) {
	    $laddr = $1;
	    $netmask = $2;
	    $broadcast = $3;
	    if (&same_subnet($laddr, $3, $netmask) &&
		&same_host($3, "255.255.255.255", $netmask)) {
		undef $broadcast;
	    }
	} elsif (/^\s*([\d.]+)\s+destination\s+([\d.]+)\s+netmask\s+([\d.]+)\s?$/) {
	    $laddr = $1;
	    $destination = $2;
	    $netmask = $3;
	}

	printf TMP qq/# %s::\n/, $iface;
	printf TMP qq/ipaddr_%s="%s"\n/, $iface, $laddr;
	if ($netmask ne "") {
	    printf TMP qq/netmask_%s="%s"\n/, $iface, $netmask;
	} else {
	    printf TMP qq/netmask_%s=""\n/, $iface;
	}
	printf TMP qq/linkarg_%s="%s"\n/, $iface, &xlate_media($media_current{$iface});
	if (defined($broadcast)) {
	    printf TMP qq/additional_%s="broadcast %s"\n/, $iface, $broadcast;
	} elsif (defined($destination)) {
	    printf TMP qq/additional_%s="destination %s"\n/, $iface, $destination;
	} else {
	    printf TMP qq/additional_%s=\n/, $iface;
	}
    }
    print TMP <<ETX;

# DO NOT DELETE THIS LINE (V3.0) -- make local changes below here

ETX

    open (PROTO, "$NETSTART_PROTO") ||
	die "$0: can't open $NETSTART_PROTO: $!\n";
    @proto = <PROTO>;
    close PROTO;
    print TMP @proto;
    close TMP;
    rename ("$NETSTART_TMP", "$NETSTART") || 
    	die "$0: can't rename $NETSTART_TMP into place: $!\n";
}

sub netstart_init {
    local(@pccard, @pocket, @hardwired);

    $netutil{"LADDR"} = undef;
    $netutil{"IFACE"} = undef;
    &query_interfaces(@NETSTART_QUERYLIST);
    if ($idata{"LADDR"} ne "" && $netutil{"LADDR"} ne undef) {
	$netutil{"LADDR"} = $idata{"LADDR"};
    }
    if ($idata{"IFACE"} ne "" && $netutil{"IFACE"} ne undef) {
	$netutil{"IFACE"} = $idata{"IFACE"};
    }
    if (! @networks) {
	return 0;
    }

    $netutil{"MEDIA"} = undef;
    $netutil{"MASK"} = "255.255.255.0";
    $netutil{"RADDR"} = undef;
    $netutil{"ROUTER"} = undef;
    $netutil{"RUSER"} = "root";
    $netutil{"DIRECTORY"} = "/cdrom";

    if ($idata{"IFACE"} ne "") { $netutil{"IFACE"} = $idata{"IFACE"}; }
    if ($idata{"NETMASK"} ne "") { $netutil{"MASK"} = $idata{"NETMASK"}; }
    if ($idata{"RADDR"} ne "") { $netutil{"RADDR"} = $idata{"RADDR"}; }
    if ($idata{"ROUTER"} ne "") { $netutil{"ROUTER"} = $idata{"ROUTER"}; }
    if ($idata{"RUSER"} ne "") { $netutil{"RUSER"} = $idata{"RUSER"}; }
    if ($idata{"DIRECTORY"} ne "") { $netutil{"DIRECTORY"} = $idata{"DIRECTORY"}; }

    if (@configured_networks) {
	return 1;
    }	

    # Choose the default interface

    # Seperate interfaces by type, PCCARD, Pocket (i.e. || port) or hardwired
    foreach $_ (@networks) {
	if ($bus{$_} eq "pcmcia") {
	    # Ignore PCCARD interfaces that are not the first instance
	    if (/\D+(\d+)/ && $1 == 0) {
		push(@pccard, $_);
	    }
	} elsif ($bus{$_} eq "lp") {
	    push(@pocket, $_);
	} else {
	    push(@hardwired, $_);
	}
    }

    # If there are notebook (PCCARD and || port) interfaces and
    # there are hardwired based interfaces, only present the later.
    if (@hardwired) {
	@networks = @hardwired;
    } else {
	@networks = (@pccard, @pocket);
    }

    # Assume the first interface is the default
    $netutil{"IFACE"} = $networks[$[];

    return 1;
}

sub netstart_setup {
    local($how) = @_;
    local($linktype, $media_active, $media_current, $media_status, $text);
    local(@media_choices);

    if ($how ne "install") {
	for ($i=$[; $i < @networks; $i++) {
	    system("ifconfig $networks[$i] -remove > /dev/null 2>&1");
	}
    }
    while ($how ne "install") {
	    $TEXT=
"This machine must have a host name assigned to it.

The host name for a machine should be the fully qualified
name of the machine (e.g., myhost.mydomain.com).  Host names
and domain names can consist of letters, numbers, and `-'.";
	    $_ = &vquery("Host name for this machine?", $netutil{"HOSTNAME"});
	    if (!/\./) {
		    $COMMENTS=
"The hostname MUST be the fully qualified name (including the domain part).";
		    $INVALID="Invalid entry: $_";
		    next;
	    } 
	    last if /^([a-z0-9][-a-z0-9]*[a-z0-9])(\.[a-z0-9][-a-z0-9]*[a-z0-9])+$/i;
	    $COMMENTS=
"Host/domain names must consist of letters, digits, `-', `.', and `_' only.";
	    $INVALID="Invalid entry: $_";
    }
    if ($how ne "install") {
    	$netutil{"HOSTNAME"} = $_;
    }

    if ($how ne "install" || (@configured_networks == 0 && @networks > 1)) {
	local($iface);

	# Prompt for network interface (IFACE)
	$TEXT=
"$INSTALL has discovered the following network interfaces:

";
	for ($i=$[; $i < @networks; $i++) {
	    $iface = $networks[$i];
	    ($type) = $iface =~ /^(.*)[1-9]*[0-9]$/;
	    $TEXT .= sprintf("    %-8s%s [%s]\n", $iface, $desc{$type}, $bus{$iface});
	}
	$TEXT .= sprintf("    %-8s%s\n", "none", "no network interface installed");
	$TEXT=$TEXT."
Please select one of these interfaces";

	if ($how eq "install") {
		$TEXT=$TEXT." or remove the BSD/OS Installation
floppy diskette from the diskette drive and restart your installation after
either adding a recognized CD-ROM or network interface";
	}


    	push(@networks, "none");
	$iface = &vquery("Which interface?", $netutil{"IFACE"}, @networks);
	pop(@networks);

	if ($iface eq "none") {
	    	if ($how ne "install") {
			$netutil{"IFACE"} = undef;
			$LIST="Hostname          : $netutil{\"HOSTNAME\"}\n";
			return 0;
		}
		$TEXT=
"A network connection is required to continue this installation.

To install BSD/OS you must either have a recognized local CD-ROM or
a recognized network interface which is connected to a network on
which there is an accessible CD-ROM.";
		&hang("");
	}

	if ($iface ne $netutil{"IFACE"}) {
	    $netutil{"MEDIA"} = undef;
	    $netutil{"IFACE"} = $iface;
	}
    }
    $linktype = $linktype{$netutil{"IFACE"}};
    $media_active = $media_active{$netutil{"IFACE"}};
    $media_current = $media_current{$netutil{"IFACE"}};
    $media_status = $media_status{$netutil{"IFACE"}};

    # Prompt for local and remote IP addresses.  If they do
    # not appear to be on the subnet, prompt for netmask and/or
    # gateway.  If the answers are not acceptable, try again.

    $iface = $netutil{"IFACE"};
    $TITLE=$TITLE." for $iface";
    $gethostname = 0;
    for (;;) {
	$LIST="";
    	if ($how ne "install") {
	    $_ = $netutil{"HOSTNAME"};
	    while ($gethostname) {
		    $TEXT=
"This machine must have a host name assigned to it.

The host name for a machine should be the fully qualified
name of the machine (e.g., myhost.mydomain.com).  Host names
and domain names can consist of letters, numbers, and `-'.";
		    $_ = &vquery("Host name for this machine?", $netutil{"HOSTNAME"});
		    if (!/\./) {
			    $COMMENTS=
"The hostname MUST be the fully qualified name (including the domain part).";
			    $INVALID="Invalid entry: $_";
			    next;
		    } 
		    last if /^([a-z0-9][-a-z0-9]*[a-z0-9])(\.[a-z0-9][-a-z0-9]*[a-z0-9])+$/i;
		    $COMMENTS=
"Host/domain names must consist of letters, digits, `-', `.', and `_' only.";
		    $INVALID="Invalid entry: $_";
	    }
	    $gethostname = 1;
	    $netutil{"HOSTNAME"} = $_;
	    $LIST.="Hostname          : $netutil{\"HOSTNAME\"}\n";
    	}
	if ($how ne "install" || @configured_networks == 0) {
	    # Prompt for local IP address (netutil{"LADDR"})
	    $TEXT=$LIST."
$INSTALL needs to know what IP address will be assigned
to this network interface.  (Enter \"help\" for more information)";
	    $netutil{"LADDR"} = &vquery_addr("IP address for $iface?",
					     $netutil{"LADDR"});
	    $LIST.="Local IP Address  : $netutil{\"LADDR\"}\n";
	}

    	if ($how eq "install") {
		$TEXT=$LIST."
$INSTALL needs to know the IP address of the machine that has
the BSD/OS BINARY CD-ROM mounted.";


		$netutil{"RADDR"} = &vquery_addr("IP address of remote machine?",
						 $netutil{"RADDR"});
		$LIST.="Remote IP Address : $netutil{\"RADDR\"}\n";

		if (@configured_networks > 0) {
		    last;
		}

		if (&inet_aton($netutil{"LADDR"}) == &inet_aton($netutil{"RADDR"})) {
		    $TEXT =
"You specified the same IP address for this system and the remote system.
Please double check the addresses you specified.

";
		    &vquery("Press <ENTER> to continue");
		    next;
		}
	}

	$TEXT=$LIST."
$INSTALL now needs the netmask for this network.
(Enter \"help\" for more information)";

	$netutil{"MASK"} = &vquery_mask("Netmask for this $desc{$linktype}?", $netutil{"MASK"});
	$LIST.="Network Mask      : $netutil{\"MASK\"}\n";

	if ($how ne "install" || !&same_subnet($netutil{"LADDR"}, $netutil{"RADDR"}, $netutil{"MASK"})) {
	    if ($how eq "install") {
		$TEXT=$LIST."
Since $netutil{\"RADDR\"} does not appear to be on the same network this
machine is connected to, you will need to specify the IP address of a
gateway machine.";
	    } else {
		$TEXT=$LIST."
If this machine needs to be able to communicate with machines on other
networks, the IP address of the gateway machine must be provided.";
	    }

	    if ($how ne "install" && $netutil{"ROUTER"} eq "") {
		$netutil{"ROUTER"} = "none";
	    }
	    $netutil{"ROUTER"} = &vquery_addr("IP address of gateway machine?", $netutil{"ROUTER"});
	    $LIST.="Gatway IP Address : $netutil{\"ROUTER\"}\n";
      
	    if ($netutil{"ROUTER"} eq "none") {
		$netutil{"ROUTER"} = "";
	    } elsif (!&same_subnet($netutil{"LADDR"}, $netutil{"ROUTER"}, $netutil{"MASK"})) {
		$TEXT=
"The specified router does not appear to be on the same network as this
machine!";
		&vquery("Press <ENTER> to continue");
		next;
	    }
	}

	last;
    }

    if ($how eq "install") {
	# Prompt for remote user (netutil{"RUSER"})
	for (;;) {
	    $TEXT = $LIST."
$INSTALL needs to know the account name with which to access
$netutil{\"RADDR\"}. (Enter \"help\" for more information)";

		$HELP=
"The remote machine will need to trust this machine for rsh access
either as root or as some other remote user name.  To enable rsh
access, the remote machine will need to have this machine's name
in the remote user's .rhosts file.  Root's .rhosts file is named
/root/.rhosts on newer BSD systems or /.rhosts on older BSD systems.

User names must be 16 characters or fewer, must start with a letter,
and may contain only letters and digits.";

	    $_ = &vquery("Remote user name", $netutil{"RUSER"});
	    if (/^$/ || /^\?$/) {
		next;
	    }
	    if (!/^[a-zA-Z][a-zA-Z0-9]*$/ || length($_) > 16) {
		$INVALID="Invalid login name: $_";
		next;
	    }
	    $netutil{"RUSER"} = $_;
	    $LIST.="Remote User Name  : $netutil{\"RUSER\"}\n";
	    last;
	}
    }

    @media_choices = ($netutil{"MEDIA"});
    if (($how ne "install" || @configured_networks == 0) && $media_choices[$[] eq undef) {
	local($ask_user, $iface, $default_config, $i);
	local(@choices, @c);

	$iface = $netutil{"IFACE"};

	if ($media_choices{$iface} eq "") {
		$media_choices{$iface} = $media_current{$iface};
	}
	# Parse the choices
	@choices = split('%', $media_choices{$iface});

	# Propagate the options onto the choices
	@c = @choices;
	foreach $choice (@c) {
	    foreach $_ (split(' ', $media_options{$iface})) {
		push(@choices, $choice . " " . $_);
	    }
	}

	# There is no point in trying any ``nomedia'' choices
	@choices = grep(! /\bnomedia\b/, @choices);

	# During the install process, remove all choices that involve
	# forcing ``full_duplex'' as that could trash the network.
	# During the configuration process, let the user make the
	# decision.
	if ($how eq "install") {
		@choices = grep(! /\bfull_duplex\b/, @choices);
	}

	if (@choices == 0) {
	    # No choices left, make sure there is a dummy entry
	    # in the list.

	    @media_choices = ("");
	} elsif (@choices == 1) {
	    # One choice available

	    @media_choices = @choices;

	    if ($choices[$#choices] eq "manual") {
		# Manual configuration, inform the user

		$TEXT=
"This network interface may have hardware configuration jumpers or
DIP switches to set the interface type.  You will need to configure
them correctly for the interface to work properly.

You might also need to us a utility that came with the card to
configure its interface type.

Please refer to your hardware documentation for further information.";

    	    	if ($how eq "install") {
		    $TEXT.="

If you need to adjust any hardware settings or run any configuration
utilities, remove the floppy disk from the floppy drive and power
off your machine.  Make any changes needed then restart this
installation process.";
		}
		&vquery("Press <ENTER> to continue");
	    }
	} elsif (@choices > 1) {
	    # Multiple choices

    	    if ($how ne "install") {
		# If configuring the system, let the user make all
		# the decisions.

		$ask_user = 1;
	    } elsif (grep(/\bauto\b/, @choices) != 0) {
		# If we have auto mode(s), use them

		@media_choices = grep(/\bauto\b/, @choices);
	    } elsif ($linktype eq "ether") {
		local($ten, $hundred) = (0, 0);

		# Trying 10MB and/or 100MB on the wrong network can hang the network

		foreach $_ (@choices) {
		    if (/\b10\D/ || /\baui\b/ || /\bcoax\b/ || /\bbnc\b/) {
			$ten++;
		    } elsif (/\b100\D/) {
			$hundred++;
		    }
		}

		if (($ten != 0 && $hundred != 0)) {
		    # Prompt for the speed to narrow the list

		    $TEXT =
"This network interface supports both 10MB and 100MB networks.
Configuring it in the wrong mode may cause problems for other
systems on the network.";
		    $rate = &vquery("What rate is your network running at?",
				    10, (10, 100));
		    @media_choices = grep(/\b$rate\D/, @choices);
		} else {
		    @media_choices = @choices;
		}
	    } elsif ($linktype eq "token_ring") {
		local($etr, $stp, $utp, $four, $sixteen) = (0, 0, 0, 0, 0, 0);

		# Mixing STP and UTP, 4MB and 16MB, or ETR and !ETR
		# can trash a network
		foreach $_ (@choices) {
		    if (/\betr\b/) {
			$etr++;
		    }
		    if (/\b[us]tp4\b/) {
			$four++;
		    } elsif (/\b[us]tp16\b/) {
			$sixteen++;
		    }
		    if (/\bstp[416]+\b/) {
			$stp++;
		    } elsif (/\butp[416]+\b/) {
			$utp++;
		    }
		}
		if (($etr > 0 && $etr < @choices) ||
		    ($stp > 0 && $utp > 0) ||
		    ($four > 0 && $sixteen > 0)) {
		    # Ask the user to decide on the configuration to use.

		    $ask_user = 1;
		} else {
		    @media_choices = @choices;
		}
	    }
	}

	# If not doing an install, ask the user to make the media
	# choices.  During the install procedure we try all reasonable
	# possibilities.
	if ($ask_user != 0 && @choices > 0) {
	    local(@enumlist, $i, @word);

	    # Choose a default configuration
	    if (defined($netutil{"MEDIA"})) {
		# Use the configuration from the previous run

    		for ($i = $[; $i <= @choices; $i++) {
		    if ($netutil{"MEDIA"} eq $choices[$i]) {
			$default_config = $i + 1;
			last;
		    }
		}
	    } elsif (defined($media_current)) {
		# Use the current device configuration as the default
		# Use the configuration from the previous run

    		for ($i = $[; $i <= @choices; $i++) {
		    if ($media_current eq $choices[$i]) {
			$default_config = $i + 1;
			last;
		    }
		}
	    }

	    # Build a list of valid configurations and prompt for it.
	    $HELP=
"There are several types of media for networks.  The available types are:

";
	    foreach $choice (@choices) {
		foreach $word (split(/\s+/, $choice)) {
		    $words{$word}++;
		}
	    }
	    foreach $word (sort keys %words) {
		$HELP .= sprintf("%-12s %s\n", $word,
				 defined($desc{$word})
				   ? $desc{$word}
				   : "(no description available)");
	    }

	    $HELP.="
Please see the documentation that came with your network interface
for more information.";

	    $TEXT=
"This network interface supports more than one type of media.  In order
to properly configure the card, $INSTALL must be told which
media to use.  The following types of media are available:

";

	    foreach $num ($[ .. $#choices) {
		$enumlist[$num] = sprintf("%2u - %s", $num + 1, $choices[$num]);
	    }
	    &v_addlist(@enumlist);
	    $_ = &vquery("Media type to use:", 
		         $default_config, 1 .. @choices);
		
	    $default_config = $choices[$_ - 1];
	    $netutil{"MEDIA"} = $default_config;

	    @media_choices = ($default_config);
	}
    }

    if ($how ne "install") {
	if ($netutil{"MEDIA"} ne "") {
        	$LIST.="Media Type        : $netutil{\"MEDIA\"}\n";
	}
	&configure_interface($netutil{"IFACE"}, $netutil{"LADDR"},
			     $netutil{"MASK"}, $netutil{"MEDIA"}, 
			     $netutil{"ROUTER"});
	return 1;
    }

    # Try each of the various media choices
    &vaddtext("");
    $text = $TEXT;
  media_loop:
    foreach $media (@media_choices) {
	local(@rc);

	if ($media ne "") {
	    &vaddtext("Trying media $media ...");
	}

	if (@configured_networks == 0) {
	    &configure_interface($netutil{"IFACE"}, $netutil{"LADDR"},
				 $netutil{"MASK"}, $media, 
				 $netutil{"ROUTER"});

	    # If ifconfig reports status for this interface and
	    # it starts with ``no-'', then we can skip this configuration.
	    # It is not true that if it reports ``carrier'' or ``in-ring''
	    # that we can assume it works, we'll let the rsh test determine
	    # that.
	 status_check:
	    for ($i = $NOCAR_RETRY; ; sleep(5)) {
	        if (open(IFCONFIG, qq/ifconfig $netutil{"IFACE"}|/)) {
		    local(@ifconfig);

		    @ifconfig = <IFCONFIG>;
		    close(IFCONFIG);
		    if (grep(/^\s+media/ && /status\s+no-/, @ifconfig)) {
			if (--$i == 0) {
			    if ($media ne "") {
				$TEXT = $text;
				&vaddtext("Trying media $media ... no carrier");
			    }
			    &unconfigure_interface($netutil{"IFACE"}, 
						   $netutil{"LADDR"},
						   $netutil{"MASK"}, $media, 
						   $netutil{"ROUTER"});
			    next media_loop;
			}
			next status_check;
		    }
		    last status_check;
		}
	    }
	}
	
	@rc = &run_command("rsh", "-nK", "-l", $netutil{"RUSER"}, 
			   $netutil{"RADDR"}, "date");
	if (shift(@rc) == 0) {
            if ($media ne "") {
		$TEXT = $text;
		&vaddtext("Trying media $media ... succeeded");
	    }
	    $netutil{"MEDIA"} = $media;
    	    return &getcd();
	}

	if (@configured_networks == 0) {
	    &unconfigure_interface($netutil{"IFACE"}, $netutil{"LADDR"},
				   $netutil{"MASK"}, $media, 
				   $netutil{"ROUTER"});
	}
	    
	if (grep(/Permission denied\.$/ || /Login incorrect\.$/, @rc)) {
	    $netutil{"MEDIA"} = $media;
	    $TEXT=
"Remote access failed (permission denied).

The remote host specified is not correctly configured
for remote access by $netutil{\"RUSER\"}.  The installation process will now
return to the point where you started configuring the network.

The server machine will need to trust this machine for rsh access
either as $netutil{\"RUSER\"} or as some other remote user name.
To enable rsh access, the remote machine will need to have this
machine's name in the remote user's .rhosts file.  Root's .rhosts file
is named /root/.rhosts on newer BSD systems or /.rhosts on older
BSD systems.";
	    $HELP=
"If you cannot resolve the problem, an installation from a local
CD-ROM may be a better option for you.

To install from a local CD-ROM, remove the floppy disk from the
floppy drive and power down this machine.  Make sure a properly
configured and supported CD-ROM drive is installed in this machine
and then restart the installation process.";
	    &vquery("Press <ENTER> to continue");
	    return 0;
	}

	if ($media ne "") {
    	    $TEXT = $text;
            &vaddtext("Trying media $media ... no response");
	}
    }

    $TEXT=
"Remote access failed.

Either the network is incorrectly configured, the remote host was
incorrectly specified, or the remote host is not correctly configured
for remote access by $netutil{\"RUSER\"}.  The installation process will now
return to the point where you started configuring the network.

The server machine will need to trust this machine for rsh access
either as $netutil{\"RUSER\"} or as some other remote user name.
To enable rsh access, the remote machine will need to have this
machine's name in the remote user's .rhosts file.  Root's .rhosts file
is named /root/.rhosts on newer BSD systems or /.rhosts on older
BSD systems.";
    $HELP=
"If you cannot resolve the problem, an installation from a local
CD-ROM may be a better option for you.

To install from a local CD-ROM, remove the floppy disk from the
floppy drive and power down this machine.  Make sure a properly
configured and supported CD-ROM drive is installed in this machine
and then restart the installation process.";
    &vquery("Press <ENTER> to continue");

    return 0;
}

sub getcd {
    $TEXT = $LIST."
$INSTALL needs to know the directory on the remote machine
on which the BSD/OS BINARY CD-ROM mounted.";

    $HELP=
"In order to access the CD-ROM which is mounted on the remote
machine $INSTALL must be told where that is.  It is typically
/cdrom or /mnt.";

    $netutil{"DIRECTORY"} = &vquery("Directory ?", $netutil{"DIRECTORY"});

    $LIST.="CD-ROM Mounted On : $netutil{\"DIRECTORY\"}\n";

    $TEXT = $LIST;

    $HELP=
"Review the options you have entered.  If they are incorrect and it
is not possible to contact the remote host you will be given a
chance to correct them.";

    $_ = &vquery("Are these correct?", "yes", ("yes", "no"));
    if ($_ =~ /yes/) {
	return 1;
    }
    return 0;
}

sub config_netstart {
    $TITLE="BSD/OS Network Configuration";
    if (&netstart_init() == 0) {
	return 0;
    }
    for (;;) {
	&netstart_setup("config");
        $TEXT = $LIST;
	$_ = &vquery("Are these correct?", "yes", ("yes", "no"));
	if ($_ =~ /yes/) {
	    last;
	}       
    }
    &netstart_write;
    if ($netutil{"IFACE"} ne "") {
	&unconfigure_interface($netutil{"IFACE"}, $netutil{"LADDR"},
			 $netutil{"MASK"}, $netutil{"MEDIA"},
			 $netutil{"ROUTER"});
    }
}
