#!/usr/bin/perl

# vixie 22nov92 [original, written while at decwrl, freely redistributable]
# vixie 28jan94 [updated for bsd/386 1.1]
#
# uusnap.pl,v 2.2 1996/06/02 08:27:07 vixie Exp

require 'sysexits.ph';

$Now = time;

#
# command line arguments
#
$AllSys = 0;
$params = '';
foreach (@ARGV) {
	if (/^-v/) { $verbose++; }
	elsif (/^-a/) { $AllSys++; }
	elsif (/^-P/) { $params = $_; }
	elsif (/^-/)  { die "usage:  $0 [-a] [-v] [-Pparams] [match ...]\n"; }
	else { $siteswanted{$_} = ''; $siteswanted++; }
}

#
# parameters
#
open(uuparams, "/usr/sbin/uuparams $params -w DIR|") || die "uuparams: $!";
while (<uuparams>) {
	chop;  next unless /\=/;
	$UUparams{$`} = $';
}
close(uuparams);
($StatDir = $UUparams{'STATUSDIR'}) || die "no STATUSDIR defined\n";
($SpoolDir = $UUparams{'SPOOLDIR'}) || die "no SPOOLDIR defined\n";

#
# find connection status
#
#**      Files each have 2 lines:
#**            node "in"  line oktime nowtime retrytime state count text...
#**            node "out" line oktime nowtime retrytime state count text...
#
#		bob out notty 759818163 759824266 600 1 0 NO DEVICE
#
chdir($StatDir) || die "chdir $StatDir: $!";
opendir(d, '.') || die "opendir $StatDir: $!";
while ($f = readdir(d)) {
	next if ($f =~ /^\./) || ($f =~ /\.\d+$/);
	next if ($siteswanted && !defined($siteswanted{$f}));
	open(f) || next;
	$t = 0;
	foreach (<f>) {
		chop; next if /^$/;
		($node, $inout, $tty,
		 $oktime, $nowtime, $retrytime, $state, $count,
		 @rest) = split;
		next if ($nowtime < $t);
		$t = $nowtime;
		if (/^$|FAILED|COMPLETE|NO DEVICE/) {
			$inout = '';
			$tty = '';
		}
		$nodes{$node} = '';
		$interesting{$node} = '' if ($inout || $tty);
		$status{$node} = join(' ', @rest);
		if ($retrytime && $count) {
			$status{$node} .= " (";
			$_ = ($nowtime + $retrytime - $Now) / 60;
			if ($_ < 0) {
				$status{$node} .= "retry time reached";
			} else {
				$status{$node} .= sprintf("retry: %d min", $_);
			}
			$status{$node} .= sprintf(", count: %d)", $count);
		}
	}
	close(f);
}
closedir(d);

#
# find queue size
#
chdir($SpoolDir) || die "chdir $SpoolDir: $!";
opendir(d, '.') || die "opendir $SpoolDir: $!";
while ($f = readdir(d)) {
	if ($f ne '.Xqtdir') {
		next if ($f =~ /^\./);
		next if ($siteswanted && !defined($siteswanted{$f}));
	}
	next if (! -d $f);	# pedantic? if opendir is in libc, it's needed.
	opendir(dd, $f) || next;
	$size = 0;
	%count = ();
	while ($ff = readdir(dd)) {
#HINT		if ($ff eq '.active') {	# probably should use this... }
		next unless ($ff =~ /^([CD])\./);
		$count{$1}++;
		$size += ((-s "$f/$ff") / 1024.0);
	}
	closedir(dd);
	if ($size > 999.0) {
		$units = 'm';
		$size /= 1024.0;
	} else {
		$units = 'k';
	}
	$queue{$f} = sprintf("%4dc %4dd %3d%s",
		$count{'C'}, $count{'D'}, 0.5+$size, $units);
	$nodes{$f} = '';
	$interesting{$f} = '' if ($count{'C'} || $count{'D'} || $size);
	%count = ();	# conserve dynamic memory
}
closedir(d);

#
# format and display
#
$printed = 0;
foreach $node (sort(keys(%nodes))) {
	next unless (defined($interesting{$node}) || $AllSys);
	printf "%-10s %s %s\n", $node, $queue{$node}, $status{$node};
	$printed++;
}

exit &EX_NOHOST unless $printed;
exit 0;
