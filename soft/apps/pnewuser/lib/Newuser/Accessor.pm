#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Accessor.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

package Newuser::Accessor;

sub define
{
	my $package = shift;
	foreach my $name (@_) {
		my $method = $package . '::' . $name;
		no strict "refs";
		*$method = sub ($;$) {
			my $self = shift;
			my $value = shift;
			$self->{$name} = $value if defined($value);
			return $self->{$name};
		}
	}
}

sub accessor ($$;$)
{
	my $field = shift;
	my $self = shift;
	my $value = shift;
	$self->{$field} = $value if defined($value);
	return $self->{$field};
}

1;
