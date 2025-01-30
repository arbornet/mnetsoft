#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Secret.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Program;

package Newuser::Secret;

my $apg = '/usr/local/bin/apg';
my $algorithm = 0;
my $length = 8;

sub get
{
	my $secret = Newuser::Program::run($apg,
	    '-a', $algorithm,
	    '-m', $length,
	    '-x', $length,
	    '-n', 1);
	chomp($secret);
	return($secret);
}

1;
