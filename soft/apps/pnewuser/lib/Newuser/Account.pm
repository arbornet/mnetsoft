#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Account.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use POSIX;

use Newuser::Accessor;
use Newuser::Passwd::Blowfish;
use Newuser::Isodate;
use Newuser::User;

package Newuser::Account;

Newuser::Accessor::define(__PACKAGE__,
	'addtime', 'homedir', 'password', 'quotas', 'starttime', 'tty', 'user');

sub TO_JSON
{
        my $self = shift;
        my $hash = {};
        $hash->{$_} = $self->{$_} for (keys %{ $self });
        return $hash;
}

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->starttime(Newuser::Isodate::now);
	$self->tty(POSIX::ttyname(0));
	$self->password(new Newuser::Passwd::Blowfish);
	$self->quotas(Newuser::Configuration::quotas);
	return $self;
}

1;
