#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Hidden.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Hidden;
our @ISA = qw( Newuser::Data );

my $PROMPT = 'Would you like to hide this information from others [Y/n]? ';

my $ERROR = 'Please enter "yes" or "no".';

sub key
{
	return 'hidden';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->SUPER::value($reader->line($PROMPT));
}

sub isvalid
{
	my $self = shift;
	my $flag = lc($self->SUPER::value || '');
	return 1 if ($flag eq 'yes' || $flag eq 'ye' || $flag eq 'y' ||
		$flag eq 'no' || $flag eq 'n' || $flag eq '');
}

sub value
{
	my $self = shift;
	my $flag = lc($self->SUPER::value || '');
	return ($flag eq 'yes' || $flag eq 'ye' ||
		$flag eq 'y' || $flag eq '') ? 'yes' : 'no';
}

sub error
{
	print $ERROR, "\n";
}

1;
