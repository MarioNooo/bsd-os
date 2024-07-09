#!/usr/local/bin/perl -w
require 5.002;
use strict;
use Socket;
use FileHandle;

sub MultiLineCommand; # forward declaration

my ($remote,$port, $iaddr, $paddr, $proto, $line, $msgno, $result, $junk, $command, $param);

$msgno  = shift || 0;
$command = shift || "retr";
$param   = shift || "";
$remote = shift || 'localhost';
$port   = shift || 110; # pop3 
if ($port =~ /\D/) {$port = getservbyname($port, 'tcp') }
die "No port" unless $port;
$iaddr = inet_aton($remote);
$paddr = sockaddr_in($port, $iaddr);

$proto = getprotobyname('tcp');
socket(SOCK, PF_INET, SOCK_STREAM, $proto) or die "socket: $!";
connect(SOCK, $paddr)                      or die "connect: $!";
SOCK->autoflush();
$line=<SOCK>;
print SOCK "user qpop\r\n";
$line=<SOCK>;
print SOCK "pass 400miles\r\n";
$line=<SOCK>;

if ( $msgno == 0 ) {
  print SOCK "stat\r\n";
  $line=<SOCK>;
  ($result, $msgno, $junk) = split ' ', $line, 3;
  die "STAT to server failed." unless $result eq "+OK";
  my $i;
  for($i = 1; $i <= $msgno; $i++) {
    &MultiLineCommand("$command %d $param\r\n", $i);
  }
}
else {
  # Get the message 
  
  # Just RETR
  &MultiLineCommand("$command %d $param\r\n", $msgno);
  # RETR with MANGLE
  #&MultiLineCommand("retr %d mangle\r\n", $msgno);
  # RETR with MANGLE(TEXT=HTML)
  #&MultiLineCommand("retr %d mangle(text=html)\r\n", $msgno);
  # TOP 10 with MANGLE
  #&MultiLineCommand("top %d mangle\r\n", $msgno);
}
print SOCK "quit\r\n";
$line=<SOCK>;
close(SOCK);
exit;


sub MultiLineCommand {
  my @res;
  printf SOCK $_[0], $_[1];
  $line = <SOCK>;
  @res = split ' ', $line;
  die "Command failed." unless $res[0] eq "+OK";
  while($line = <SOCK>) {
    last if ($line eq ".\r\n");
    print $line;
  }
}
