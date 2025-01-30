#!/usr/bin/perl

use warnings;
use strict;

use Newuser::Captcha;

my $cap = new Newuser::Captcha;
print "The banner is:\n", $cap->banner;
print "\nEnter the captcha: ";
my $challenge = <>;
chomp $challenge;
print "Validation = ", 0 + $cap->validate($challenge), "\n";
print "secret == ", $cap->{secret}, "\n";
