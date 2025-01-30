#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Telephone.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Telephone;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Enter your telephone number (123-456-7890): ';

sub key
{
	return 'telephone';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->line($PROMPT));
}

1;
