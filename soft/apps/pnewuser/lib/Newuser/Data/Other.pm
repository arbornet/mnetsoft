#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Other.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Other;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Enter your other interests:';

sub key
{
	return 'interests';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->paragraph($PROMPT));
}

1;
