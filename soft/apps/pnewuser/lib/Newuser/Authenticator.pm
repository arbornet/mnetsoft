#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Authenticator.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Accessor;
use Newuser::Configuration;
use Newuser::Captcha;

package Newuser::Authenticator;

Newuser::Accessor::define(__PACKAGE__,
	'captchas', 'lasterror', 'printer', 'reader');

my $EX_AUTHENTICATION = "U:CAPTCHA authentication failed.";
my $DEFAULT_MAX_TRIES = 5;

my $HELP =
'Because of past abuses, we have had to institute rather draconian measures
to prevent damage to Grex as a service.  One of those is a simple test of
whether or not an account is being created by a human being.  Below this
text, a random string of letters and numbers will be printed out in a large,
block-style font.

To verify that you are a human and not a computer, please type that string
back in when prompted.
';

my $TEMP_FAILURE = '
Sorry, that was incorrect.  Try again.
';

my $FAILURE = '
Sorry, you have failed the test, and you are probably a computer program,
not a human being.  Please have a human come back and create an account
later.
';

sub new
{
	my $klass = shift;
	my $newuser = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->printer($newuser->printer);
	$self->reader($newuser->reader);
	$self->lasterror("");
	my @captchas;
	my $maxtries = Newuser::Configuration::max_captcha_tries || $DEFAULT_MAX_TRIES;
	for (my $i = 0; $i < $maxtries; $i++) {
		push @captchas, new Newuser::Captcha;
	}
	$self->captchas([@captchas]);
	return $self;
}

sub do_captcha_and_test
{
	my $self = shift;
	my $captcha = shift;
	my $reader = $self->reader;
	print "\n", $captcha->banner, "\n";
	my $token = $reader->line("Enter string: ");
	return $captcha->validate($token);
}

sub authenticate_or_die
{
	my $self = shift;
	$self->help;
	$self->try_max_times || $self->failure;
}

sub try_max_times
{
	my $self = shift;
	foreach my $captcha ( @{ $self->captchas } ) {
		$self->print_last_error;
		return 1 if $self->do_captcha_and_test($captcha);
		$self->lasterror($TEMP_FAILURE);
	}
	return 0;
}

sub print_last_error
{
	my $self = shift;
	print $self->lasterror;
}

sub failure
{
	print $FAILURE;
	die $EX_AUTHENTICATION;
}

sub help
{
	my $self = shift;
	my $printer = $self->printer;
	$printer->string($HELP);
}

1;
