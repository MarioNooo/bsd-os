#	BSDI	query.pl,v 1.8 2003/02/28 22:36:33 chrisk Exp
#
# Query the system for configuration parameters
#
$DMESGFILE="/dev/klog";
$DMESGSAVE="/tmp/dmesg.boot";

require "/usr/libdata/adm/Descriptions.pl";

sub querysystem {
	$idata{"MEMSIZE"} = 0;

	if (open(IN, $DMESGSAVE) == 0 && open(IN, "/var/db/dmesg.boot") == 0) {
		if (open(IN, $DMESGFILE) == 0) {
			die "can't open $DMESGFILE -- installation aborted.\n";
		}
		fcntl(IN, 4, 4) || die "can't set $DMESGFILE to non-blocking.\n";
	} else {
		$DMESGFILE=$DMESGSAVE;
	}
	$len = sysread(IN, $dmesg, 8192);
	close(IN);
	if ($DMESGSAVE ne $DMESGFILE) {
		if (open(OUT, ">$DMESGSAVE")) {
			print OUT $dmesg;
			close(OUT);
		} else {
			die "can't open $DMESGSAVE\n";
		}
	}

	@lines = split(/\n/, $dmesg);

	$APM = 0;
	foreach $i (@lines) {
        	if ($i =~ /^buffer cache/) {
                	$col = 1;
                	next;
        	}
		if ($i =~ /^apm: conn. vers.*OK$/) {
			$APM = 1;
			next;
		}
        	if ($i =~ /^real mem/) {
			$i =~ /\s([0-9]*)$/;
			$idata{"MEMSIZE"} = $1;
                	next;
        	}
        	next if (!$col);
        	if ($i =~ /^([a-z]+[0-9])\s/) {
                	$devs{$1} = $i;
			$lastdev = $1;
			$auxinfo{$lastdev} = "";
        	} elsif ($i =~ /^$lastdev:/) {
			if ($auxinfo{$lastdev} ne "") {
				$auxinfo{$lastdev} .= "\n";
			}
			$auxinfo{$lastdev} .= $i;
		}
	}
	foreach $d (keys(%devs)) {
		$devs{$d} =~ /([^\s]*)\s([^\s]*)\s([^\s]*)[0-9][:\s]/;
		if ($2 eq "at") {
			if ($3 =~ /isa[0-9]\/lp$/) {
				$bus{$d} = "lp";
			} else {
				$bus{$d} = $3;
			}
		}
		$d =~ /(.*)[0-9]/;
		$base = $1;
		foreach $c (keys(%cl)) {
		    foreach $dev (split(/\s/, $cl{$c})) {
			if ($base eq $dev) {
			    if ($devs{$d} =~ /\sCD-ROM:/ ||
				$devs{$d} =~ /\sworm:/) {
				    $found{"cdrom"} .= "$d ";
			    } else {
				    $found{$c} .= "$d ";
			    }
			    if (($base eq "sd" || $base eq "wd"
			      || $base eq "sr" || $base eq "cr"
			      || $base eq "amir" || $base eq "aacr" ) &&
			       $devs{$d} =~
				/([0-9][0-9]*)\s*\*\s*([0-9][0-9]*)/){
				    $capacity{$d} = int($1*$2/(1024*1024));
			    } elsif ($base eq "sd" || $base eq "wd"
				  || $base eq "sr" || $base eq "cr"
				  || $base eq "amir" || $base eq "aacr" ) {
				    $capacity{$d} = 0;
			    }
			    if ($base eq "pccons" && $devs{$d} =~ /mono/ &&
				$ENV{'TERM'} =~ /bsdos-pc/) {
				    $ENV{'TERM'}="bsdos-pc-mono";
			    }
			    if ($base eq "com" && $devs{$d} =~ /console/ &&
				$ENV{'TERM'} =~ /bsdos-pc/) {
				    $ENV{'TERM'}="vt100";
				    $CONSOLE=$d;
			    }
			}
		    }
		}
	}

	@cdroms = sort split(/\s/,$found{"cdrom"});
	@disks = sort split(/\s/,$found{"disk"});
	@network = sort split(/\s/,$found{"network"});
	@pocket = sort split(/\s/,$found{"pocket"});
}
1;
