#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Exit.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

package Newuser::Exit;

sub confirm_and_exit
{
	print STDERR "Really exit [y/N]? ";
	my $choice = <>;
	$choice = 'yes' unless defined($choice);
	chomp($choice);
	$choice = lc $choice;
	if ($choice eq 'y' || $choice eq 'ye' || $choice eq 'yes') {
		print STDERR "Okay, Goodbye!\n";
		exit 0;
	}
}

1;
