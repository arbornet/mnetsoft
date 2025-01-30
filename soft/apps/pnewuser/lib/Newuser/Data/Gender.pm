#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Gender.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Data;

package Newuser::Data::Gender;
our @ISA = qw( Newuser::Data );

my $PREFACE = '
Enter your gender if you would like, but do not feel obligated.

';

my $PROMPT = 'What is your gender? ';

my $ERROR = 'Most people are either male or female.';

sub key
{
	return 'gender';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->line($PROMPT));
}

sub isvalid
{
	my $self = shift;
	my $gender = lc($self->value || '');
	my @ok = qw(male female homme femme man woman womyn m f);
	return 1 if $gender eq '';
	foreach my $str (@ok) {
		return 1 if ($str eq $gender);
	}
	return 0;
}

sub error
{
	print $ERROR, "\n";
}

sub preface
{
	my $self = shift;
	my $printer = $self->printer;
	$printer->string($PREFACE);
}

1;
