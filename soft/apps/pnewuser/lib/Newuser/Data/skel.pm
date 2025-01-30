#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: skel.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Skel;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Enter something: ';

sub key
{
	return 'skel';
}

sub getvalue;
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->line($PROMPT));
}

1;
