#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Computers.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Computers;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Enter the kind of computer(s) you use:';

sub key
{
	return 'computers';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->paragraph($PROMPT));
}

1;
