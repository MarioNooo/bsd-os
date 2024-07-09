#!/usr/bin/perl
#
# This extracts the microcode from an smc825uc.bin file (from the SMC driver
# floppy Novell 2.2 drivers) and generates C code with the microcode embedded
# in it. This is typically run as:
#	tegucode smc825uc.bin teucode.c
#

if ($#ARGV != 1) {
	die "Usage: $0 infile.bin outfile.c\n";
}
$fn = shift;
$out = shift;

open(F,$fn) || die;
$len = read(F, $buf, 0x10000);
close(F);
print "read $len bytes\n";
$eye = index($buf, "microcode here>");
print "found eyecatcher at $eye\n";
$ucode = $eye+16;
if ($ucode+8192 > $len) {
	die "Short microcode image\n";
}
$sum = 0;
$cnt = 0;
open(C,">$out") || die;
print C "/*-
 * Copyright (c) 1995 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI \$Id\$
 */
\n";

print C "unsigned short smc_ucode[] = {\n";
print C "0xdead, 0xbeef, 0x0000,\n";
for ($i=$ucode; $i < $ucode+8192; $i+=2) {
	$s = unpack("S",substr($buf,$i,2));
	$sum += $s;
	printf(C "0x%4.4x, ",$s);
	if (++$cnt == 10) {
		print C "\n";
		$cnt = 0;
	}
}
print C "\n};\n";
if ($sum & 0xffff) {
	die "Checksum bad: $sum\n";
}
exit(0);
