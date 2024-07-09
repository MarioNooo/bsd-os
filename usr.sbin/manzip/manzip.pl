#!/usr/bin/perl

$0 =~ s#.*/##;

# parse arguments
require 'getopts.pl';
&Getopts("dM:m:s:u") || 
	die "Usage: $0 [-u] [-M path] [-m path] [-s subdirs]\n";

# compression type and suffix
$compress="/usr/contrib/bin/gzip -9";
$uncompress="/usr/contrib/bin/gunzip";
$ext=".gz";

# Get the man directory list from man.conf
$_ = `/usr/bin/grep ^_default /etc/man.conf`;	# get the man.conf _default
chop;						# drop the newline
split;						# split it into words
$_ = `/bin/csh -c \'echo $_[1]\'`;		# glob it
chop;						# lose the newline
@mandirs=split;					# generate the array

# Which subdirs?
$_ = `/usr/bin/grep ^_subdir /etc/man.conf`;	# get the man.conf _subdir
chop;						# drop the newline
@subdirs=split;					# split it into words
shift @subdirs;
@subdirs=split(/:\s/,$opt_s) if defined($opt_s); # override if explicit

# What architecture?
$arch = `/usr/bin/arch`;
chop($arch);

# Next see if they have a MANPATH variable set to override
@mandirs = split(/:/, $ENV{"MANPATH"}) if defined($ENV{"MANPATH"});

# -m augments the existing list
if (defined($opt_m)) {
	split(/:/, $opt_m);
	for $i (@_) {
		push(@mandirs, $i);
	}
}

# -M replaces the existing list
@mandirs = split(/:/, $opt_M) if (defined($opt_M));

for $i (@mandirs) {
	next if ! -d $i;
	print "Processing pages in $i\n";
	&zipdir($i);
}
exit 0;

######################################################################

sub zipdir {
	local($dir, $oldwd, $i, $ino, %num, %names, $links) = @_;
	
	$oldwd = `pwd`;
	chop($oldwd);

	if (!chdir($dir)) {
		print STDERR "$0: chdir $dir failed: $!\n"; 
		return; 
	}

	# Generate the list of files
	$links=0;
	for $i (@subdirs) {
		opendir(DIR, $i) || next;
		while ($_ = readdir(DIR)) {
			if ($opt_u) {
				next if ! /\.[0-8]$ext$/;
			} 
			else {
				next if ! /\.[0-8]$/;
			}
			$ino = (lstat("$i/$_"))[1];
			if ( -l _ ) {
				$ino = "S$links";
				$links++;
			}
			$num{$ino}++;
			$names{$ino,$num{$ino}} = "$i/$_";
		}
		closedir(DIR);
		opendir(DIR, "$i/$arch") || next;
		while ($_ = readdir(DIR)) {
			if ($opt_u) {
				next if ! /\.[0-8]$ext$/;
			} 
			else {
				next if ! /\.[0-8]$/;
			}
			$ino = (lstat("$i/$arch/$_"))[1];
			if ( -l _ ) {
				$ino = "S$links";
				$links++;
			}
			$num{$ino}++;
			$names{$ino,$num{$ino}} = "$i/$arch/$_";
		}
		closedir(DIR);
	}

	# Process the list of files
	for $_ (keys(%num)) {

		# No extra hard links to file -> just do it...
		if ($num{$_} == 1) {
			if (-l $names{$_,1}) {
				&do_symlink($names{$_,1});
				next;
			}
			&do_compress($names{$_,1});
			next; 
		} 

		# Remove extra links
		for ($i=2; $i <= $num{$_}; $i++) {
			&unlink_file($names{$_,$i});
		}

		# Compress (or uncompress) the file
		if (! &do_compress($names{$_,1})) {
			# Restore old links if we failed
			for ($i=2; $i <= $num{$_}; $i++) {
				&link_file($names{$_,1}, $names{$_,$i});
			}
			next;
		}

		# Make new links
		if ($opt_u) { 
			$names{$_,1} =~ s/$ext$//; 
		}
		else { 
			$names{$_,1} .= $ext;
		}
		for ($i=2; $i <= $num{$_}; $i++) {
			if ($opt_u) {
				$names{$_,$i} =~ s/$ext$//;
			}
			else {
				$names{$_,$i} .= $ext;
			}
			&link_file("$names{$_,1}", "$names{$_,$i}");
		}
	}
	chdir($oldwd) || print STDERR "$0: chdir $oldwd failed: $!\n";
}

sub do_symlink {
	local($name, $ptr) = @_;

	print "doing symlink $name\n" if $opt_d;
	$ptr = readlink $name;
	print "unlinking $name\n" if $opt_d;
	unlink($name) || 
		print STDERR "$0: WARNING: unlink $name failed ($!)\n"; 
	if ($opt_u) {
		$ptr =~ s/$ext$//;
		$name =~ s/$ext$//;
	} 
	else {
		$ptr .= $ext;
		$name .= $ext;
	}
	print "symlinking $name\n" if $opt_d;
	symlink("$ptr", "$name") || 
		print STDERR "$0: WARNING: link $name failed ($!)\n"; 
	1;
}

sub do_compress {
	local($name, $op, $tmp, @cmd, $pid) = @_;

	if ($opt_u) {
		$op = $uncompress;
		$tmp = $name;
		$tmp =~ s/$ext$//;
		if ( -f $tmp ) {
			# DON'T overwrite existing uncompressed file
			unlink($name);
			return;
		}
	}
	else {
		$op = $compress;
		# DO overwrite existing compressed file
		unlink("$name$ext") if -f "$name$ext";
	}
	print "$op $name\n" if $opt_d; 

	# build a command array to avoid running a shell each time
	@cmd = split(/\s/, $op);
	push(@cmd, $name);
	$pid = fork;
	if ($pid == 0) {
		$SIG{'INT'} = 'IGNORE';
		exec @cmd;
	}
	waitpid($pid, 0);
	if ($? != 0) {	
		print STDERR "$0: WARNING: $op $name failed\n"; 
		return 0;
	}
	1;
}

sub unlink_file {
	local($name) = @_;

	print "unlink($name)\n" if $opt_d;
	unlink($name) || 
		print STDERR "$0: WARNING: unlink $name failed ($!)\n";
	1;
}

sub link_file {
	local($to, $from) = @_;
	
	print "link($to, $from)\n" if $opt_d;
	link($to, $from) || 
		print STDERR "$0: WARNING: ln $to $from failed ($!)\n";
	1;
}
