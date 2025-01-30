#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Signals.pm 1026 2011-01-08 12:28:58Z cross $
#
# Deal with signals.
#

use warnings;
use strict;

package Newuser::Signals;

sub initialize
{
	$SIG{$_} = 'IGNORE' for keys(%SIG);
	$SIG{CHLD} = 'DEFAULT';
}

sub interrupt
{
	print STDERR "*Interrupt*\n";
	die "S:INT:Interrupt";
}

sub hangup
{
	die "S:INT:Hangup";
}

sub enable_interrupts
{
	$SIG{'HUP'} = \&hangup;
	$SIG{'INT'} = \&interrupt;
}

sub disable_interrupts
{
	$SIG{'INT'} = 'IGNORE';
	$SIG{'HUP'} = 'IGNORE';
}

1;
