#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Log.pm 1006 2010-12-08 02:38:52Z cross $
#
# Logging routes for pnewuser.
#

use warnings;
use strict;

use Newuser::Accessor;
use Newuser::Logger;

package Newuser::Log;

Newuser::Accessor::define(__PACKAGE__, 'logger');

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->logger(new Newuser::Logger);
	return $self;
}

sub creation_failure
{
	my $self = shift;
	print STDERR  "Creation failure\n";
}

sub creation_success
{
	my $self = shift;
	print STDERR  "Creation successful\n";
}

sub send_mail_failure
{
	my $self = shift;
	print STDERR  "Mail send failure\n";
}

sub send_mail_success
{
	my $self = shift;
	print STDERR  "Mail successfully sent\n";
}

1;
