#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Creator.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use JSON;

package Newuser::Creator;

sub loguser
{
	my $self = shift;
	my $newuser = $self->newuser;
	my $account = $self->account;
	my $logger = $newuser->logger;
	$logger->log($account);
}

sub create
{
	my $self = shift;
	my $account = shift;

	$self->account($account);
	$self->adduser;
	$self->quotas;
	$self->loguser;
	1;
}

1;
