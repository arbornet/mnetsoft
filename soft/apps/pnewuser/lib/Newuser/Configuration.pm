#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Configuration.pm 1006 2010-12-08 02:38:52Z cross $
#
# Tcl interface for configuration.
#

use warnings;
use strict;

use Tcl;

package Newuser::Configuration;

my $interp = new Tcl;
my %config;
my %quotadevices;
$interp->EvalFile($::PNUHOME . "/lib/nuconfig.tcl");

tie %config, 'Tcl::Var', $interp, "config";
tie %quotadevices, 'Tcl::Var', $interp, "quotadevs";

sub group
{
	return $config{'group'};
}

sub loginclass
{
	return $config{'loginclass'};
}

sub uidrange
{
	return $config{'uidrange'};
}

sub userdirs
{
	return @{ $config{'userdev'} };
}

sub aliasfiles
{
	return @{ $config{'aliases'} };
}

sub badnamesfiles
{
	return @{ $config{'badnames'} };
}

sub badpatternsfiles
{
	return @{ $config{'badpatterns'} };
}

sub syslog
{
	return $config{'syslog'};
}

sub logfile
{
	return $config{'logfile'};
}

sub logprog
{
	return $config{'logprog'};
}

sub mailerrsfile
{
	return $config{'mailerrsfile'};
}

sub max_captcha_tries
{
	return $config{'maxauthfailures'};
}

sub quotas
{
	return [ map { [ $_, @{ $quotadevices{$_} } ] } keys %quotadevices ];
}

sub infodir
{
	return $::PNUHOME . '/lib/info';
}

sub lockdir
{
	return $config{'lockdir'};
}

sub emptydir
{
	return $config{'emptydir'};
}

sub skeldir
{
	return $config{'skeldir'};
}

sub messagedir
{
	return $::PNUHOME . '/lib/messages';
}

sub userpath
{
	my $user = shift;
	my $proc = $config{'userpath'};
	return $interp->call($proc, $user);
}

1;
