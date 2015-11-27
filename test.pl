#!/usr/bin/perl

use strict;

my $i = 0;

for("a".."zzzz") {
  ++$i;
  print "set $_ $i\n";
}

for my $cmd(qw/get del/) {
  print "$cmd $_\n" for("a".."zzzz");
}

print "aaaa\n";
print "quit";
