#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Shell.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Shell;
our @ISA = qw( Newuser::Data );

my $PROMPT = '';
my $RESH = '/usr/local/bin/resh';

sub key
{
	return 'shell';
}

sub getvalue
{
	my $self = shift;
	$self->value($RESH);
}

1;
