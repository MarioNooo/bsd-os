#! /usr/bin/perl
#
#
# Copyright [C] The Regents of the University of Michigan and Merit Network,
# Inc. 1993, 1994, 1995, 1996, 1997, 1998 All Rights Reserved.
#
# RCSID:	$Id: conversion.pl,v 1.5 1998/01/30 17:03:16 web Exp $
#
# Filename: conversion.pl
# Author:   Barry James

# Define some constants

$infile = "users.livingston";
$outfile = "users.merit";

#
# Read in Livingston style users file and change selected strings to
# match the DRAFT RADIUS RFC as implemented in Merit RADIUS.
#

open ( INFILE, $infile ) || die "Can't open $infile: $!\n";
open ( OFILE, ">$outfile" ) || die "Can't open $outfile: $!\n";

while ( <INFILE> )
{
	s/Challenge-State/State/;		# attribute 24 type string
	s/Client-Id/NAS-IP-Address/;		# attribute 4 type ipaddr
	s/Client-Port-Id/NAS-Port/;		# attribute 5 type int
	s/Dialback-Name/Framed-Callback-Id/;	# attribute 20 type string
	s/Dialback-No/Login-Callback-Number/;	# attribute 19 type string
	s/Framed-Address/Framed-IP-Address/;	# attribute 8 type ipaddr
	s/Jacobsen/Jacobson/;			# attribute 13 type int
	s/Framed-Netmask/Framed-IP-Netmask/;	# attribute 9 type ipaddr
	s/Framed-Filter-Id/Filter-Id/;		# attribute 11 type string
	s/Login-Host/Login-IP-Host/;		# attribute 14 type ipaddr
	s/Port-Message/Reply-Message/;		# attribute 18 type string
	s/User-Service-Type/Service-Type/;	# attribute 6 type int
	s/Login-User/Login/;
	s/Framed-User/Framed/;
	s/Dialback-Login-User/Callback-Login/;
	s/Dialback-Framed-User/Callback-Framed/;

	print OFILE "$_";

} # end while ( <INFILE> )
