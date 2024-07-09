#!/usr/bin/perl

#
# Copyright (c) 1994 Berkeley Software Design, Inc.
# All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
#	BSDI	adm.pl,v 1.6 1998/11/13 16:54:56 polk Exp

# Where we live...
$ADMBIN="/usr/sbin";

# Strip the path for our name
$0 =~ s#^.*/##;

$ETC="/etc";
$SKEL="/usr/share/skel/skel.en_US";
$PWD_MKDB="/usr/sbin/pwd_mkdb";
$DEF_DIRMODE=0755;

$GROUP="$ETC/group";
$PASSWD="$ETC/master.passwd";
$ALIASES="$ETC/aliases";
$RESOLV="$ETC/resolv.conf";

$SENDMAILCW="sendmail.cw";
$SENDMAIL_PID="/var/run/sendmail.pid";

$PASSWD_DEF="/var/db/passwd.cf";	# Defaults file for adduser

$TZDIR="/usr/share/zoneinfo";		# Timezone data

$SHLIBC="/shlib/libc_s.3.0.0";		# Shared version of libc

$COLS=80;		# Assume 80 column screen
$GUTTER=2;		# Minimum 2 spaces between items in columnar lists

push(@INC, "/usr/libdata/adm");

# Load handy routines
require 'util.pl';

1;
