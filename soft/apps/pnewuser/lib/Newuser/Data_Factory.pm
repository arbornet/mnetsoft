#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Data_Factory.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

package Newuser::Data_Factory;

my @MANDATORY = qw( Name Email );
my @OPTIONAL = qw( Address Telephone Occupation Computers Birthday Gender Other );
my @HIDDEN = qw( Hidden );
my @LOGIN = qw( Login );
my @UNCHANGEABLE = qw( Shell );
my @SETABLE = ( @MANDATORY, @OPTIONAL, @HIDDEN, @LOGIN );
my @MODULES = ( @SETABLE, @UNCHANGEABLE );

sub load
{
	my $module = shift;
	my $lib = $module . ".pm";
	$lib =~ s,::,/,g;
	require $lib;
	import $module;
}

sub import_all
{
	load "Newuser::Data::$_" for @MODULES;
}

sub keys
{
	return map { eval "key Newuser::Data::$_" } @MODULES;
}

sub setable_keys
{
	return map { eval "key Newuser::Data::$_" } @SETABLE;
}

sub new
{
	my $klass = shift;
	my $newuser = shift;
	my $self = {};
	$self->{newuser} = $newuser;
	bless($self, ref($klass) || $klass);
	return $self;
}

sub create_list_of_data_objects
{
	my $self = shift;
	my @types = @_;
	my $newuser = $self->{newuser};
	return map { "Newuser::Data::$_"->new($newuser) } @types;
}

sub mandatory
{
	return shift->create_list_of_data_objects(@MANDATORY);
}

sub optional
{
	return shift->create_list_of_data_objects(@OPTIONAL);
}

sub hidden
{
	return shift->create_list_of_data_objects(@HIDDEN);
}

sub login
{
	return shift->create_list_of_data_objects(@LOGIN);
}

sub unchangeable
{
	return shift->create_list_of_data_objects(@UNCHANGEABLE);
}

import_all;

1;
