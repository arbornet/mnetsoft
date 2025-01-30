#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Data.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Accessor;
use Newuser::Exit;

package Newuser::Data;

Newuser::Accessor::define(__PACKAGE__, 'printer', 'reader', 'user');

sub new
{
	my $klass = shift;
	my $newuser = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->printer($newuser->printer);
	$self->reader($newuser->reader);

	# Store a reference to ourselves in the user, for our key.
	my $user = $newuser->user;
	$self->user($user);
	$user->accessor($self->key, $self);
	return $self;
}

sub value
{
	my $self = shift;
	return Newuser::Accessor::accessor($self->key, $self, shift);
}

sub key
{
	die "P:Key called from Newuser::Data superclass";
}

sub get_or_interrupt
{
	my $self = shift;
	my $isvalid = 0;

	eval {
		$self->getvalue;
		$isvalid = $self->isvalid;
		$self->error unless $isvalid;
	};
	if ($@) {
		die $@ unless ($@ =~ /^S:INT:/);
		Newuser::Exit::confirm_and_exit;
	}
	return $isvalid;
}

sub get
{
	my $self = shift;
	$self->preface;
	while (!$self->get_or_interrupt) {}
}

sub isvalid
{
	return 1;
}

sub preface
{
	# ...
}

sub error
{
	# ...
}

1;
