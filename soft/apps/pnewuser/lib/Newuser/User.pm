#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: User.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Accessor;
use Newuser::Data_Factory;

package Newuser::User;

my $NOVALUE = '(no value given)';

sub TO_JSON
{
	my $self = shift;
	my $hash = {};
	$hash->{$_} = $self->{$_}->value for (keys %{ $self });
	return $hash;
}

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	return $self;
}

sub keys
{
	my $self = shift;
	return keys %{ $self };
}

sub accessor
{
	my $self = shift;
	my $key = shift;
	my $value = shift;
	return Newuser::Accessor::accessor($key, $self, $value);
}

sub value
{
	my $self = shift;
	my $datum = $self->accessor(shift);
	return $NOVALUE unless defined($datum);
	return $datum->value || $NOVALUE;
}

1;
