#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Birthday.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Birthday;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Enter your birthday: ';

sub key
{
	return 'birthday';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->line($PROMPT));
}

1;
