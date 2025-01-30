#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Captcha.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Program;
use Newuser::Secret;

package Newuser::Captcha;

my $figlet = '/usr/local/bin/figlet';
my $font = 'clb8x10';

sub getbanner
{
	return Newuser::Program::run($figlet,
	    '-f', $font,
	    '--',
	    shift);
}

sub new
{
	my $klass = shift;
	my $self = {};
	my $secret = Newuser::Secret::get;
	chomp $secret;
	$self->{secret} = $secret;
	$self->{banner} = getbanner $secret;
	bless($self, ref($klass) || $klass);
	return $self;
}

sub banner
{
	return shift->{banner};
}

sub validate
{
	my $self = shift;
	my $token = shift;
	return $token eq $self->{secret};
}

1;
