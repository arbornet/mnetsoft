#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Homedirectory.pm 1613 2017-06-01 10:25:47Z cross $
#

use warnings;
use strict;

use Filesys::Df;

use Newuser::Accessor;
use Newuser::Configuration;

package Newuser::Homedirectory;

Newuser::Accessor::define(__PACKAGE__, 'homedir');

sub TO_JSON
{
	my $self = shift;
	return $self->homedir;
}

sub new
{
	my $klass = shift;
	my $login = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->homedir($self->pick($login));
	return $self;
}

sub pick
{
	my $self = shift;
	my $login = shift;

	my @filesystems = Newuser::Configuration::userdirs;
	my $filesystem = '/home';
	my $mostfree = 0;
	foreach my $fs (@filesystems) {
		my $df = Filesys::Df::df($fs);
		next unless $df;
		if ($df->{bavail} > $mostfree) {
			$filesystem = $fs;
			$mostfree = $df->{bavail};
		}
	}
	my $userpath = Newuser::Configuration::userpath($login);
	#return $filesystem . '/' . $userpath;
	return '/' . $userpath;
}

sub create_template
{
}

1;
