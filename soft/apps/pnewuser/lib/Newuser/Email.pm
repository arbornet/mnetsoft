#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Email.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Email::Valid;

package Email;

sub new
{
	my $klass = shift;
	my $address = shift;
	my $self = {};
	$self->{valid} = new Email::Valid;
	bless($self, ref($klass) || $klass);
	$self->address $address;
	return $self;
}

sub address
{
	my $self = shift;
	my $addr = shift;
	$self->{address} = $addr if $addr;
	return $self->{address};
}

sub isvalid
{
	my $canonical;
	eval {
		my $valid = $self->{valid};
		$canonical = $valid->address(
		    -address => $self->{address},
		    -mxcheck => 1);
	};
	return $canonical;
}

1;
