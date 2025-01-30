#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Blowfish.pm 1006 2010-12-08 02:38:52Z cross $
#
# Modern BSD specific password generation.
#

use warnings;
use strict;

use Newuser::Passwd;
use Newuser::Program;

package Newuser::Passwd::Blowfish;
our @ISA = qw( Newuser::Passwd );

my $genbsalt = "$::PNUHOME/bin/genbsalt";
my $rounds = 6;

sub gensalt
{
	my $self = shift;
	my $salt = Newuser::Program::run($genbsalt, '-n', $rounds);
	chomp $salt;
	return $salt;
}

1;
