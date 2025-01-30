#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Isodate.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use DateTime;

package Newuser::Isodate;

my $TIMEZONE = 'UTC';

sub now
{
	my $dt = DateTime->now( time_zone => $TIMEZONE );
	return $dt->iso8601 . 'Z';
}

1;
