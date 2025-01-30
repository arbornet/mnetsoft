#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Passwd.pm 1006 2010-12-08 02:38:52Z cross $
#
# Get a password and crypt it.
#

use strict;
use warnings;

use Newuser::Secret;

package Newuser::Passwd;

sub TO_JSON
{
	my $self = shift;
	my $hash = {};
	$hash->{'hashed'} = $self->hashed;
	return $hash
}

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->{passwd} = Newuser::Secret::get unless $self->{passwd};
	$self->{salt} = $self->gensalt;
	return $self;
}

sub gensalt
{
	my $self = shift;
	return sprintf("%2x", int(rand(65536)));
}

sub hashed
{
	my $self = shift;
	my $passwd = $self->plain;
	return(crypt($passwd, $self->{salt}));
}

sub plain
{
	my $self = shift;
	return $self->{passwd};
}

1;
