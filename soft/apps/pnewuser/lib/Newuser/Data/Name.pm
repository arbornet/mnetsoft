#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Name.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Name;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Enter your name: ';

sub key
{
	return 'name';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	my $name = $reader->line($PROMPT);
	$self->value($name or 'anonymous');
}

1;
