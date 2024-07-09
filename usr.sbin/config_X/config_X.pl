#! /usr/bin/perl
#
# Copyright (c) 2001 Wind River Systems, Inc.  All rights reserved.
#
# Copyright (c) 1996, 1998 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	config_X.pl,v 1.15 2001/10/03 17:30:42 polk Exp
#
# config_X -- help the user configure Xaccel using trial-and-error

# Make sure we're running as root
if ($> != 0) {
        print "$0: this program must be run as root!\n";
        exit 1;
}

require '/usr/libdata/adm/adm.pl';
require '/usr/libdata/adm/v.pl';

$| = 1;  # Make STDOUT unbuffered...

require 'getopts.pl';
&Getopts('nv') || die "Usage: $0 -nv\n";

$TITLE="BSD/OS X11 Configuration";
&vexplain("BSD/OS X11 Configuration") if !$opt_n;

$TEXT=
"You will be asked to choose an X server from a list of servers.  Once
you have chosen a server, you must configure it.  You will need to
provide the server's configuration program with information about your
video card, your display and your mouse.  You will be asked about the
manufacturer and model of your video card and display, and the type and
device for your mouse.  Serial mice are almost always the Microsoft /
MouseMan type and typically use /dev/tty00, while PS/2 mice and laptop
touchpads are usually PS/2 type and use /dev/pcaux0.  If you don't know
the manufacturer or model for your video card or display, it's usually
best to choose 640x480 VGA mode now and customize later by running
config_X again.

";

$_ = "n";
while (!/^y/i) {
	$_ = &vquery("Continue?", "yes", ("yes", "no", "quit"));
	&abort(1) if /^quit$/i;
}

$TEXT=
"When testing a new X configuration you might be unable to move the
mouse or see the screen at all.  If that happens, simultaneously press:
        Control-Alt-Backspace
and the X server will exit and you will be given the opportunity to
try another configuration or give up.

";

$_ = "n";
while (!/^y/i) {
	$_ = &vquery("Continue?", "yes", ("yes", "no", "quit"));
	&abort(1) if /^quit$/i;
}

# We hope that the customer hasn't messed with paths or symlinks...

@Xmetro_servers = </usr/X11R6/bin/Xmetro*>;
@XF86_servers = </usr/X11R6/bin/XFree86*>;
@Xaccel_servers = </usr/X11R6/bin/Xaccel*>;

$done = 0;

while (!$done) {

    $etc_X = readlink("/etc/X");
    if (defined($etc_X)) {
	$etc_X =~ s/^\.\.//;
	$etc_X =~ s:^/usr/X11(R6)?/bin/::;
	$etc_X =~ s/_.*$//;
    }
    $etc_X = "Xmetro" if $etc_X eq "X";
    $default_X = $etc_X;

    @servers = ();

    $TEXT=
"Choose from the following available X servers:

";
    if (defined(@Xmetro_servers)) {
	$TEXT .=
"    Xmetro:  Metro Link, Inc. server
";
	$default_X = "Xmetro" if !defined($default_X);
	@servers = (@servers, "Xmetro");
    }
    if (defined(@XF86_servers)) {
	$TEXT .=
"    XFree86: The XFree86 Project, Inc. server
";
	$default_X = "XFree86" if !defined($default_X);
	@servers = (@servers, "XFree86");
    }
    if (defined(@Xaccel_servers)) {
	$TEXT .=
"    Xaccel:  Xi Graphics, Inc. server
";
	$default_X = "Xaccel" if !defined($default_X);
	@servers = (@servers, "Xaccel");
    }
    $TEXT .= "\n";
    $_ = "n";
    while (!/^(Xmetro|XFree86|Xaccel)/i) {
	$_ = &vquery("Server:", $default_X, (@servers, "quit"));
	&abort(1) if /^quit$/i;
    }
    $new_X = $_;

    if ($new_X eq $etc_X) {
	$TEXT = 
"You have selected $new_X, and the $new_X server is currently installed.
Do you wish to accept the old configuration, or start the $new_X
configuration utility and build a new one?  Type \"accept\" if you
wish to accept the old configuration, or \"change\" to build a new one.

";
	$_ = "n";
	while (!/^(accept|change)/i) {
	    $_ = &vquery("What now?", "accept", ("accept", "change", "quit"));
	    &abort(1) if /^quit/i;
	}
	$done = 1 if /^accept/i;
	next if $done;
    }

    unlink("/etc/X");
    unlink("/etc/X.config");
    unlink("/usr/X11R6/bin/X");

    $configured = 0;

    &configure_Xmetro() if $new_X =~ /^Xmetro/i;
    &configure_XF86() if $new_X =~ /^XFree86/i;
    &configure_Xaccel() if $new_X =~ /^Xaccel/i;

    $result = $configured ? "succeeded" : "failed";

    $TEXT =
"The configuration apparently $result.

Do you wish to:
    \"accept\" this configuration
    \"change\" this configuration
    \"quit\" config_X
?

";
    $result = $configured ? "accept" : "change";
    $_ = "n";
    while (!/^(change|accept)/i) {
	$_ = &vquery("What now?", $result, ("accept", "change", "quit"));
	&abort(1) if /^quit$/i;
    }
    $done = 1 if /^accept$/i;
}

&v_reset();
print("$0: Successful Completion\n\n");
exit(0);

sub sigusr {
    $server_started = 1;
}

sub abort {
    &v_reset();
    exit($_[0]);
}

sub test_X {

    &v_reset();

    print("Now testing X server -- press Ctrl-Alt-Backspace to abort\n");
    sleep(1);

    $server_started = 0;
    $SIG{'USR1'} = 'sigusr';
    $pid = fork();
    die "fork: $!\n" unless defined($pid);
    if ($pid == 0) {
	# child
	# X server will send SIGUSR1 when started if SIGUSR1 is ignored
	$SIG{'USR1'} = 'IGNORE';
	exec('/etc/X', ':0');
	warn("/etc/X: $!\n");
	exit(127);
    }

    # parent, waits for SIGUSR1 or a timeout
    $timeout = 5;
    sleep(1) while $timeout-- > 0 && !$server_started;
    warn("Couldn't tell if X server started, trying anyway\n")
	unless $server_started;
    $ENV{'DISPLAY'} = ':0';

    $status = system('/usr/X11/bin/xmessage', '-geometry', '+75+200',
	'-buttons', 'Yes:0,No:1',
"Does the screen look ok and the mouse work?
[press Ctrl-Alt-Backspace to abort]");
    $status = ($status>>8) & 0xff;

    # Shut down the X server.
    kill(15, $pid);
    waitpid($pid, 0);
    sleep(1);		# give X time to exit all the way (call it paranoia)

    $configured = $status == 0;
}

sub configure_Xmetro {
    # my $status;

    &v_reset();

    $status = system("/usr/X11R6/bin/configX");
    sleep(2);

    print("configX returned status $status") if $status != 0;
    sleep(2);

    unless (open(XSCRIPT, ">/usr/X11R6/bin/X")) {
	$configured = 0;
	return;
    }
    print XSCRIPT "#! /bin/sh\n\n";
    print XSCRIPT "# Plain old VGA mode doesn't work without -bestRefresh\n";
    print XSCRIPT "exec /usr/X11R6/bin/Xmetro \"\$@\" -bestRefresh\n";
    close(XSCRIPT);
    chmod(0755, "/usr/X11R6/bin/X");
    chown(0, 0, "/usr/X11R6/bin/X");

    symlink("/usr/X11R6/bin/X", "/etc/X");
    symlink("/etc/XMetroconfig", "/etc/X.config");

    &fixstaticgray_Xmetro();

    &test_X();

    # For some reason, Xmetro leaves the console in nonblocking mode.
    $flags = fcntl(STDIN, 3, 0);
    if (defined($flags) && ($flags & 0x0004) != 0) {
	$flags &= ~0x0004;
	fcntl(STDIN, 4, $flags);
    }
}

sub fixstaticgray_Xmetro {
    local(@lines, $_, $found);

    open(XMETRO, "/etc/XMetroconfig") || return;	# non-fatal
    @lines = <XMETRO>;				# snarf it
    close(XMETRO);

    # We only care if the depth is 4 bits...
    for (@lines) { if (/defaultcolordepth\s+4/i) { $found=1; last; } }
    return if !$found;

    # Now we only care if the visual isn't StaticGray
    for (@lines) { return if /visual\s+"staticgray"/i; }

    # Ask them if they really mean it...
    $TEXT = <<ETX;
It appears that you have selected a 4-bit deep display (16-color mode), 
but that you did not select the StaticGray visual.  If you do not use
the StaticGray visual with a 16-color display, you will probably be unable
to see text on some menus and graphics based applications (including
Netscape which is used for Maxim) may be unintelligible.

ETX
    $_ = &vquery("Specify the StaticGray visual?", "yes", ("yes", "no"));
    return if /no/i;

    rename("/etc/XMetroconfig", "/etc/XMetroconfig.bak");
    open(OLDXMETRO, "/etc/XMetroconfig.bak");
    open(XMETRO, ">/etc/XMetroconfig");
    chmod(0644, "/etc/XMetroconfig");
    chown(0, 0, "/etc/XMetroconfig");
    while ($_ = <OLDXMETRO>) {
	print XMETRO $_;
	if (/depth\s*4/i) {
	    print XMETRO "    Visual      \"StaticGray\"\n";
	}
    }
    close (OLDXMETRO);
    close (XMETRO);
}

sub configure_XF86 {
    # my $status;

    $TEXT =
"To configure the XFree86 server, we will next run the xf86cfg program.

In the most common case, xf86cfg will correctly identify your card
without any additional configuration.  You will typically need to set
the speed of your monitor so that you can use the best possible resolution,
and you may need to reconfigure the mouse if you are using a serial or 
two-button mouse.

When you have finished, select the \"Quit\" button and accept the default
locations for saving the new XF86Config file and the keyboard map.

";

    $_ = "n";
    while (!/^y/i) {
	    $_ = &vquery("Continue?", "yes", ("yes", "no", "quit"));
	    &abort(1) if /^quit$/i;
    }

    &v_reset();

    $status = system("/usr/X11R6/bin/xf86cfg");
    sleep(2);

    # Put everything in place...
    symlink("/etc/X11/XF86Config", "/etc/X.config");
    unlink("/etc/X");
    symlink("/usr/X11R6/bin/XFree86", "/etc/X");
    unlink("/usr/X11R6/bin/X");
    symlink("../../../etc/X", "/usr/X11R6/bin/X");
    unlink("$ENV{HOME}/XF86Config.new");

    $TEXT =
"Since xf86cfg doesn't verify that the actual /etc/X11/XF86Config
configuration file works, we need to run one more test.  If the
configuration file doesn't work, the server will complain and quit.
Otherwise, the screen will switch to graphics mode; if the screen
looks reasonable and the mouse works, please select \"yes\" with
the mouse when the menu appears, or hit ctrl-alt-backspace to abort
if nothing happens or if the mouse doesn't work.

";

    $_ = "n";
    while (!/^y/i) {
	    $_ = &vquery("Continue?", "yes", ("yes", "no", "quit"));
	    &abort(1) if /^quit$/i;
    }

    &test_X
}

sub configure_Xaccel {
    # my $status;

    &v_reset();

    $status = system("/usr/X11R6/bin/Xsetup");
    sleep(2);

    print("Xsetup returned status $status") if $status != 0;
    sleep(2);

    symlink("/usr/X11R6/bin/Xaccel", "/etc/X");
    symlink("/etc/Xaccel.ini", "/etc/X.config");
    symlink("../../../etc/X", "/usr/X11R6/bin/X");

    &fixstaticgray_Xaccel();

    &test_X();
}

sub fixstaticgray_Xaccel {
    local(@lines, $_, $found);

    open(XACCEL, "/etc/Xaccel.ini") || return;	# non-fatal
    @lines = <XACCEL>;				# snarf it
    close(XACCEL);

    # We only care if the depth is 4 bits...
    for (@lines) { if (/depth\s*=\s*4/i) { $found=1; last; } }
    return if !$found;

    # Now we only care if the visual isn't StaticGray
    for (@lines) { return if /visual\s*=\s*staticgray/i; }

    # Ask them if they really mean it...
    $TEXT = <<ETX;
It appears that you have selected a 4-bit deep display (16-color mode), 
but that you did not select the StaticGray visual.  If you do not use
the StaticGray visual with a 16-color display, you will probably be unable
to see text on some menus and graphics based applications (including
Netscape which is used for Maxim) may be unintelligible.

ETX
    $_ = &vquery("Specify the StaticGray visual?", "yes", ("yes", "no"));
    return if /no/i;

    rename("/etc/Xaccel.ini", "/etc/Xaccel.ini.bak");
    open(OLDXACCEL, "/etc/Xaccel.ini.bak");
    open(XACCEL, ">/etc/Xaccel.ini");
    chmod(0644, "/etc/Xaccel.ini");
    chown(0, 0, "/etc/Xaccel.ini");
    while ($_ = <OLDXACCEL>) {
	print XACCEL $_;
	if (/depth\s*=\s*4/i) {
	    print XACCEL "    Visual  = STATICGRAY;\n";
	}
    }
    close (OLDXACCEL);
    close (XACCEL);
}
