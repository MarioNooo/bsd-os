#!/usr/bin/perl
#
# Basic dump analysis, builds an info file to send to support.
#
# BSDI kanal.perl,v 1.3 1999/06/25 23:09:46 chrisk Exp
#

require "newgetopt.pl";

# -x	- Display chatter with subprocs
# -n	- Don't ask standard questions
# -s	- Slow (but complete) bsd.gdb/bsd compare
# -v	- Print more details

&NGetOpt("x","n","s","v");
$timeout = 120;
$editor = "prompt";
$gdbprompt = '\(gdb\) ';

$usage = "Usage: kanal [flags] dumpno\n";

$ENV{"PATH"} .= ":/usr/contrib/bin:/sbin";

if ($#ARGV == -1) {
	die $usage;
}
$dumpno = shift;
if ($#ARGV != -1) {
	die $usage;
}

$infofile = "infodir.$dumpno";
$bsd = "bsd.$dumpno";
$core = "bsdcore.$dumpno";

unlink("$infofile/log");
$d = `date`;
chop $d;
&addlog('********** Kanal kanal.perl,v 1.3 1999/06/25 23:09:46 chrisk Exp **********' . "\n");
&addlog("$d: $core\n");

#
# Gather basic config info
#
&addfile("dmesg.boot", "/var/db/dmesg.boot");
&addfile("patchlog", "/sys/PATCHLOG");
&addfile("install.log", "/var/db/install.log");
&addfile("rc.local", "/etc/rc.local");
&addfile("netstart", "/etc/netstart");

#
# See if the dump is readable
#
if (! -r "$core") {
	&addlog("$core not readable: $!\n");
	&done(1);
}

#
# Check for the kernel file, the dump is pretty useless without it.
#
if (!-r "$bsd") {
	$bsd = "bsd.gdb";
	if (!-r "$bsd") {
		&addlog("Can't read $bsd: $!\n");
		&done(1);
	}
	# speed things up a bit
	if (!-r "bsd.$dumpno.tmp") {
		&addlog("creating stripped version of $bsd to speed analysis\n");
		system("strip -d -o bsd.$dumpno.tmp $bsd");
	}
	$bsd = "bsd.$dumpno.tmp";
}

#
# Check bsd.N against bsdcore.N
#
&addlog("Check if $bsd and $core match\n");
$output = &runcmd("bpatch -s -N $bsd osrelease");
if ($output =~ /([0-9\.]+)/) {
	$bsdrel = $1;
} else {
	&addlog("Can't get osrelease from $bsd: $output\n");
	&done(1);
}
&startsub("gdb -k $bsd $core");
$x = &exp($gdbprompt);
print WH "x/s &osrelease\n";
$sout = &exp($gdbprompt);
print WH "quit\n";
close(C);
if ($sout =~ /:\s+\"([0-9\.]+)\"/) {
	$corerel = $1;
} else {
	&addlog("Can't get osrelease from $core: $sout\n");
	&done(1);
}
if ($bsdrel != $corerel) {
	&addlog("$bsd doesn't appear to match $core: bsdrel=$bsdrel corerel=$corerel\n");
	&done(1);
}

#
# Gather info that doesn't need bsd.gdb
#
&addlog("Gathering console output\n");
&addcmd("dmesg", "dmesg -M $core -N $bsd");
$dmesg = $out;
&addlog("Gathering network information\n");
&addcmd("netstat_in", "netstat -in -M $core -N $bsd");
&addcmd("netstat_rn", "netstat -rn -M $core -N $bsd");
&addcmd("netstat_m", "netstat -m -M $core -N $bsd");
&addcmd("netstat_s", "netstat -s -M $core -N $bsd");

&addlog("Gathering open file information\n");
&addcmd("fstat", "fstat -M $core -N $bsd -v");

&addlog("Gathering VM information\n");
&addcmd("vmstat_s", "vmstat -M $core -N $bsd -s");
&addcmd("vmstat_m", "vmstat -M $core -N $bsd -m");

&addlog("Gathering swap space information\n");
&addcmd("pstat_s", "pstat -M $core -N $bsd -s");
&addlog("Gathering active vnode details\n");
&addcmd("pstat_T", "pstat -M $core -N $bsd -T");
&addcmd("pstat_v", "pstat -M $core -N $bsd -v");
&addlog("Gathering tty information\n");
&addcmd("pstat_t", "pstat -M $core -N $bsd -t");

&addlog("Running ps to get proc listing\n");
&addcmd("ps", "ps -M $core -N $bsd -a -x -e -l -O sl,start,sig,flags -w -w");

&setup_bins();
&addlog("Checking kernel text integrity\n");
$out = &runcmd("./idiff -M $core -N $bsd | head -100");
unlink("$infofile/idiff");
&addinfo("idiff", $out);
@lines = split(/\n/, $out);
foreach $_ (@lines) {
	if (/^(0xf.+):/) {
		&addlog("XXX Kernel text miscompare\n");
		last;
	}
}

# Screen dmesg output
&addlog("Screening dmesg output\n");
@lines = split(/\n/, $dmesg);
$pan = "";
$trap = "";
foreach $_ (@lines) {
	if (/(trap type.*)$/) {
		$trap = $1;
		&addlog("$trap\n");
	}
	if (/(panic: .*)$/) {
		$pan = $1;
		&addlog("$pan\n");
	}
}
$eip = "";
$cr2 = "";
if ($trap ne "") {
	$trap =~ /eip (0x[^\s]+).*cr2 (0x[^\s]+)/;
	$eip = $1;
	$cr2 = $2;
}

# Do basic check on VM structures
&addlog("Checking VM structures\n");
if (defined($opt_v)) {
	$out = &runcmd("./fdump -M $core -N $bsd -t 0 -v");
} else {
	$out = &runcmd("./fdump -M $core -N $bsd -t 0");
}
unlink("$infofile/fdump");
&addinfo("fdump", $out);
@lines = split(/\n/, $out);
foreach $_ (@lines) {
	if (/XXX/) {
		&addlog("XXX VM structure problem: $_\n");
	}
}

# Do some pmap system checks
&addlog("Checking PMAP system\n");
$out = &runcmd("./pmap -M $core -N $bsd -b");
unlink("$infofile/pmap");
&addinfo("pmap", $out);

# Dump the trace buffer
&addlog("Dumping kernel trace buffer\n");
$out = &runcmd("tdump -M $core -N $bsd");
unlink("$infofile/tdump");
&addinfo("tdump", $out);

#
# Find a matching bsd.gdb
#
&getbsdgdb();

#
# If we have a bsd.gdb do some in depth analysis
#
if ($bsdgdb ne "") {
	&gdbanal();
}

# Ask user questions
&askqs();
&done(0);

exit 0;

sub done {
	($rc) = @_;
	print "Building file to send to support...\n";
	system("shar -z $infofile > info.$dumpno");
	print "Please upload the file info.$dumpno to:\n\tftp.bsdi.com:/bsdi/support/incoming/XXXXX.info.$dumpno\n";
	print "Where XXXXX is one of:\n";
	print "	- support request number (ex: 12345.info.1)\n";
	print "	- customer ID if no support number available (ex: G1234.info.1)\n";
	print "	- your initials (ex: ewv.info.1)\n";
	print "Note: this will not automatically generate a support request,\n";
	print "      send seperate email to support\@bsdi.com to open a new support request\n";
	# offer to send email?
	if ($rc != 0) {
		print "\nProblems were detected during analysis, the output may be incomplete!\n";
		unlink("bsd.$dumpno.tmp");
	}
	exit $rc;
}

sub startsub {
	local($cmd) = @_;
	close(RH);
	close(WH);
	close(S);
	pipe(RH, WH);
	select(WH); $| = 1;
	if (open(S, "-|") == 0) {
		open(STDIN, "<&RH") || die;
		open(STDERR, ">&STDOUT") || die;
		exec "$cmd";
		die "$cmd";
	}
	select(STDOUT); $| = 1;
}

#
# Read from subprocess until expected string is received or we time out
# a return variable.
#	arg0 = prompt regexp
#
sub exp {
	undef $bufr;
	$outflag = $_[1];
	if (defined($opt_x)) {
		print STDERR "Expect: $_[0] timeout=$timeout\n";
	}
	($expected) = @_;
	while (1) {
		$rin = '';
		vec($rin,fileno(S),1) = 1;
		($nfound,$timeleft) = select($rout=$rin, undef, undef,
			 $timeout);
		if ($nfound == 0) {
			print STDERR "*** TIMEOUT ***\n";
			print STDERR "*** Expected=:$expected:\n";
			print STDERR "*** Got=:$bufr:\n";
			return("timeout");
		}
		$bytes = sysread(S,$ibuf, 80);
		# This ugliness strips parity
		#for ($i=0 ; $i < $bytes; $i++) {
		#	$v = ord(substr($ibuf,$i,1))&0x7f;
		#	substr($ibuf,$i,1) = sprintf("%c",$v);
		#}
		if ($bytes == 0 || !defined $bytes) {
			return $bufr;
		}
		if (defined($opt_x)) {
			print STDERR "Debug read: $ibuf\n";
		}
		$bufr .= $ibuf;
		if ($bufr =~ /$_[0]/)  { last; }
	}
	$bufr;
}

#
# Add string data to a section
#
sub addinfo {
	($section, $data) = @_;
	if (!-d "$infofile") {
		mkdir("$infofile", 0755) || die "Can't create $infofile: $!\n";
	}
	open(INF, ">>$infofile/$section") || die "Can't open $infofile/$section: $!\n";
	print INF $data;
	close(INF);
}

#
# Edit an info section
#
sub editinfo {
	($section) = @_;
	if ($editor ne "prompt") {
		system("$editor $infofile/$section");
		return;
	}
	print "\n";
	open(F, "$infofile/$section") || die "Can't open $infofile/$section: $!\n";
	print <F>;
	close(F);
	print "[Enter text, end with a . by itself on the last line or an EOF\n";
	print "To enter an editor type vi, emacs, or pico by itself on a line]\n";
	$bufr = "";
	while (1) {
		print "> ";
		$line = <STDIN>;
		if (!defined($line)) {
			last;
		}
		$orig = $line;
		chop $line;
		if ($line eq ".") {
			last;
		}
		if ($line eq "vi" || $line eq "emacs" || line eq "pico") {
			$editor = $line;
			&addinfo($section, $bufr);
			&editinfo($section);
			return;
		}
		$bufr .= $orig;
	}
	&addinfo($section, $bufr);
}

#
# Ask dump questions of user
#
sub askqs {
	if (defined($opt_n)) {
		return;
	}
	@dqs = (
	"q_custid",
"What is your customer ID (Nxxxx)? If you don't know it please send
us any information (name, address, email address) we might use to
find you in our database.",
	"q_hwchanges",
"Has any hardware or the operating environment been changed on this
system recently? This could include adding a disk or RAM, changing
BIOS settings, moving the machine, attaching to a UPS, or anything
else along these lines. If this is a new install (ie: it has never
been stable) please let us know that as well.",
	"q_swchanges",
"Have there been any changes to system software (new patches applied,
upgrades) or usage patterns (ie: heavier or lighter than normal usage,
setting up a news feed, installing new programs off the net)?",
	"q_hwnotfound",
"Please describe any hardware present on the system that is NOT
recognized by BSD/OS at boot time.",
	"q_cputype",
"What type of CPU and speed do you have (ex: Intel Pentium 120)?",
	"q_ramsize",
"What type of RAM is installed (number of SIMMS, size of each SIMM)?",
	"q_parity",
"Is parity checking enabled in the BIOS and do you have parity RAM?",
	"q_cache",
"How much secondary cache are you using and is it enabled?",
	"q_scsi",
"If using SCSI: do you have any SCSI devices in external cabinets? If yes,
please describe the devices and how they are cabled (what order on they are
on the cable and which ones are terminated). If not using SCSI enter NA.",
	"q_repeatable",
"Can this crash be repeated by setting up certain hardware or software
conditions? If so please describe how to repeat it on the failing system.",
	"q_localmods",
"Are there any local modifications to the kernel or system utilities? If so
please give a general description of their purpose and what modules they
affect."
);
	$skipem = 0;
	while ($cat = shift @dqs) {
		$q = shift @dqs;
		$q .= "\n----------------\n";
		if (! -f "$infofile/$cat") {
			open(F, ">$infofile/$cat") || die "Can't create $infofile/$cat: $!\n";
			print F "$q";
			close(F);
		}
		# Check if we've gotten an answer
		next if ($skipem != 0);
		open(F, "<$infofile/$cat") || die "Can't open $infofile/$cat: $!\n";
		$buf = join("",<F>);
		close(F);
		if ($buf eq $q) {
			&editinfo($cat);
			open(F, "<$infofile/$cat") || die "Can't open $infofile/$cat: $!\n";
			$buf = join("",<F>);
			close(F);
			if ($buf eq $q &&
			    &prompt("Skip the rest of the questions?", "n")
			    eq "y") {
				$skipem = 1;
			}
		}
	}
}

sub prompt {
	($p, $def) = @_;
	print "$p [$def] ";
	$ans = <STDIN>;
	chop $ans;
	if ($ans eq "") {
		$ans = $def;
	}
	return $ans;
}

sub runcmd {
        local($cmd) = @_;
        if (open(C, "-|") == 0) {
                open(STDERR, ">&STDOUT") || die;
                exec "$cmd";
                die "$cmd";
        }
        $out = join("", <C>);
	close(C);
	return $out;
}

sub addlog {
	($msg) = @_;
	&addinfo("log", $msg);
	print $msg;
}

sub addfile {
	($section, $file) = @_;
	open(F, "<$file") || do {
		&addlog("Can't read $file: $!\n");
		return;
	};
	$tmp = join("", <F>);
	close(F);
	unlink("$infofile/$section");
	&addinfo($section, $tmp);
}

sub addcmd {
	($section, $cmd) = @_;
	$out = &runcmd($cmd);
	unlink("$infofile/$section");
	&addinfo($section, $out);
}

sub getbsdgdb {
	@bsdplaces = (	"bsd.gdb",
			"/sys/compile/GENERIC/bsd.gdb",
			"/usr/src/sys/compile/GENERIC/bsd.gdb",
			"/sys/compile/LOCAL/bsd.gdb",
			"/usr/src/sys/compile/LOCAL/bsd.gdb",
			"/cdrom/sys/compile/GENERIC/bsd.gdb" );
	$bsdgdb = "";
	while (1) {
		if ($bsdgdb eq "") {
			$bsdgdb = shift @bsdplaces;
			if ($last == 0 && $bsdgdb eq "") {
				$out = &runcmd("bpatch -N $bsd -s version");
				@lines = split(/\n/, $out);
				$lines[1] =~ /:(\S+)$/;
				$bsdgdb = "$1/bsd.gdb";
				$last = 1;
			}
		}
		# Ask for one if we're out of ideas
		if ($bsdgdb eq "") {
			print "The filename of a bsd.gdb that matches $bsd is needed.\n";
			print "If no file is entered some dump analysis will not be performed\n";
			print "Please enter filename: ";
			$bsdgdb = <STDIN>;
			chop $bsdgdb;
			last if ($bsdgdb eq "");
		}
		if (! -r $bsdgdb) {
			&addlog("$bsdgdb not found or readable\n");
			$bsdgdb = "";
			next;
		}
		if (&compbsd($bsd, $bsdgdb) != 0) {
			&addlog("$bsdgdb does not match $bsd\n");
			$bsdgdb = "";
			next;
		}
		&addlog("Checking for debug symbols in $bsdgdb\n");
		$out = `objdump --syms $bsdgdb | head -100 | grep stabstr | wc`;
		if ($out =~ /0\s+0\s+0/) {
			&addlog("$bsdgdb does not contain debugging information\n");
			$bsdgdb = "";
			next;
		}
		last;
	}
}

#
# Compare two bsd's (usually a bsd.N and bsd.gdb) to see if they (probably)
# match.
#
sub compbsd {
	($bsd1, $bsd2) = @_;
	&addlog("Checking $bsd1 against $bsd2\n");
	if (!defined($opt_s)) {
		$out1 = `bpatch -N $bsd1 osrelease`;
		$out2 = `bpatch -N $bsd2 osrelease`;
	} else {
		$out1 = `nm -p -g $bsd1`;
		$out2 = `nm -p -g $bsd2`;
	}
	if ($out1 ne $out2) {
		return 1;
	}
	return 0;
}

sub gdbanal {
	unlink("$infofile/gdb");
	&addlog("Starting gdb...\n");
	&startsub("gdb -k $bsdgdb $core");
	$x = &exp($gdbprompt);
	&addinfo("gdb", $x);
	if ($x eq "timeout") {
		print WH "quit\n";
		close(C);
		return 1;
	}
	&addlog("Gathering basic gdb information\n");
	return 1 if (&gcmd("set height 0\n") != 0);
	return 1 if (&gcmd("source /sys/scripts/bsdi.ps\n") != 0);
	if ($eip ne "") {
		return 1 if (&gcmd("x/i $eip\n") != 0);
		return 1 if (&gcmd("list *$eip\n") != 0);
	}
	return 1 if (&gcmd("bt\n") != 0);
	return 1 if (&gcmd("frame 2\n") != 0);
	return 1 if (&gcmd("p/x frame\n") != 0);
	return 1 if (&gcmd("p cnt\n") != 0);
	return 1 if (&gcmd("p/x _klock\n") != 0);
	return 1 if (&gcmd("p/x slock\n") != 0);
	return 1 if (&gcmd("p/x ulock\n") != 0);
	return 1 if (&gcmd("p/x ipend_lock\n") != 0);
	return 1 if (&gcmd("p/x kdebug_lock\n") != 0);
	return 1 if (&gcmd("p num2cpu\n") != 0);
	for ($cpu = 0; $cpu < 15; $cpu++) {
		return 1 if (&gcmd("p/x *num2cpu[$cpu]\n") != 0);
	}
	return 1 if (&gcmd("ps\n") != 0);
	# Run a stack trace on each process...
	&addlog("Gathering stack traces for each process (this might take some time)\n");
	@list = split(/\n/, $x);
	foreach $_ (@list) {
		if (/^\s*([0-9]+) ([0-9,a-f]+) /) {
			$pid = $1;
			$paddr = $2;
		} else {
			&addinfo("gdb", "\n*** Bad ps line: $_");
			next;
		}
		return 1 if (&gcmds("printf \"%s\\n\", &((struct proc *)0x$paddr)->p_comm\n") != 0);
		$x =~ s/\n.*$//;
		&addinfo("gdb", "\n**** Proc $pid, $x, paddr=0x$paddr ****\n");
		return 1 if (&gcmd("paddr 0x$paddr\n") != 0);
		return 1 if (&gcmd("bt\n") != 0);
		# Extract the highest frame number
		@tmp = split(/\n/, $x);
		$hframe = -1;
		foreach $l (@tmp) {
			if ($l =~ /^\#([0-9]+)/) {
				$hframe = $1;
			}
		}
		next if ($hframe == -1);
		for ($fno = 0; $fno <= $hframe; $fno++) {
			return 1 if (&gcmd("frame $fno\n") != 0);
			return 1 if (&gcmd("info locals\n") != 0);
		}
	}
	print WH "quit\n";
	close(C);
	return 0;
}

sub gcmd {
	($cmd) = @_;
	&addinfo("gdb", "===> $cmd");
	print WH $cmd;
	$x = &exp($gdbprompt);
	&addinfo("gdb", $x);
	if ($x eq "timeout" || $x eq "") {
		print WH "quit\n";
		close(C);
		return 1;
	}
	return 0;
}
# silent
sub gcmds {
	($cmd) = @_;
	print WH $cmd;
	$x = &exp($gdbprompt);
	if ($x eq "timeout" || $x eq "") {
		print WH "quit\n";
		close(C);
		return 1;
	}
	return 0;
}

# Unpack binary commands
sub setup_bins {
	open(P, "| gunzip | pax -r -v");
	while (<DATA>) {
		print P unpack("u", $_);
	}
	close(P);
}
__END__
