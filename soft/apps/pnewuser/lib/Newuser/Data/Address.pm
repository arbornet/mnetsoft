#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Address.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Address;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Please enter your address:';

sub key
{
	return 'address';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->paragraph($PROMPT));
}

1;
