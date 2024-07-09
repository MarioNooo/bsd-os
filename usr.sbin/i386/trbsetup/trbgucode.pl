#!/usr/bin/perl
#
# trbgucode.pl,v 1.1 1998/09/01 17:39:56 geertj Exp
#
# This converts the microcode from tlnkp.mac 
# (from the root of Tokendisk 6.0, disk 1 of 2)
# and generates C code with the microcode embedded in it. 
# This is typically run as:
#	trbgucode tlnkp.mac trbucode.c
#
# (code based on tegucode.pl)
#

if ($#ARGV != 1) {
	die "Usage: $0 infile.mac outfile.c\n";
}
$fn = shift;
$out = shift;

open(F,$fn) || die;
$len = read(F, $buf, 0x10000);
close(F);
print "read $len bytes\n";
# 0x4000 is arbitrary to protect against empty files mainly
if (($len < 0x4000) || (($len % 16) != 0)) {
	die "Short microcode image\n";
}
open(C,">$out") || die;
print C "/*-
 * Copyright (c) 1998 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI \$Id\$
 */
\n";

print C "unsigned char trb_ucode[] = {\n";
printf(C "\t0x%2.2x, 0x%2.2x, /* length of ucode */\n\t", 
    $len % 0x100, $len / 0x100);
$cnt = 0;
for ($i=0; $i < $len; $i++) {
	$s = unpack("C",substr($buf,$i,1));
	printf(C "0x%2.2x, ",$s);
	if (++$cnt == 8) {
		print C "\n\t";
		$cnt = 0;
	}
}
print C "\n};\n";
exit(0);
