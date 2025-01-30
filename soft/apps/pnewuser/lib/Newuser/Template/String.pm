#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: String.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Template;
use Text::Template::Preprocess;

package Newuser::Template::String;
our @ISA = qw( Newuser::Template );

sub prefix
{
	return undef;
}

sub maketemplate
{
	my $self = shift;
	my $source = shift;

	return new Text::Template::Preprocess(TYPE => 'STRING',
	    SOURCE => $source);
}

1;
