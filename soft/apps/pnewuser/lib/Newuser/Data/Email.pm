#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Email.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Email::Valid;

use Newuser::Data;

package Newuser::Data::Email;
our @ISA = qw( Newuser::Data );

my $PREFACE = '
NOTE: It is critically important that you enter a valid email address
here.  This is the address to which we will mail your password once you
create your account; if you do not get that email, you will be unable
to login!

Further, if you should ever forget your password and request that the
Grex staff reset it, this is the address that the replacement will be
sent to.

';

my $PROMPT = 'Enter your email address: ';

my $ERROR = '
Sorry, that was not a valid email address.  Please try again!
';

sub key
{
	return 'email';
}

sub canonical
{
	my $self = shift;
	my $email = $self->value;
	my $c = '';
	eval {
		$c = Email::Valid->address(-address => $email, -mxcheck => 1);
	};
	return $c;
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->line($PROMPT));
}

sub setvalue
{
	my $self = shift;
	my $user = $self->user;
	$user->accessor($self->key, $self->canonical);
}

sub isvalid
{
	my $self = shift;
	return $self->canonical;
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
