#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Confirmation.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Newuser::Accessor;
use Newuser::Data_Factory;
use Newuser::Exit;
use Newuser::Plan;

package Newuser::Confirmation;

Newuser::Accessor::define(__PACKAGE__, 'account', 'newuser', 'plan', 'printer');

sub new
{
	my $klass = shift;
	my $newuser = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->newuser($newuser);
	$self->printer($newuser->printer);
	$self->account($newuser->account);
	$self->plan($newuser->plan);
	return $self;
}

sub print
{
	my $self = shift;
	my $printer = $self->printer;
	$printer->string(shift);
}

sub printf
{
	shift->print(sprintf(shift, @_));
}

my $ACTIONS = "
Type 'help' to see a list of things you can change, or
type 'done' to create your account if you are satisfied.
";

sub show
{
	my $self = shift;
	my $account = $self->account;
	my $user = $account->user;
	my $login = $user->value('login');
	my $email = $user->value('email');
	my $plan = $self->plan;

	$self->print("\nYour account so far:\n\n");
	$self->print("Login: $login\n");
	$self->print("Email address: $email\n");
	$plan->print;
	$self->print("$ACTIONS\n");
}

sub help
{
	my $self = shift;
	my $account = $self->account;
	my $user = $account->user;
	my $unsetable = {};

	my $i = 0;
	$self->print("\nSelect the option to change from the list below:\n");
	foreach my $key (sort Newuser::Data_Factory->setable_keys) {
		$self->print("\n") if (($i++ % 4) == 0);
		$self->printf("%-15s", $key);
	}
	$self->print("\n\n");
	$self->print("Enter 'help' to display this message again.\n");
	$self->print("Enter 'show' to display your account.\n");
	$self->print("Enter 'quit' to cancel creating your account.\n");
	$self->print("Enter 'done' to continue and create your account.\n");
	$self->print("\n");
}

sub get_choice_or_interrupt
{
	my $self = shift;
	my $newuser = $self->newuser;
	my $reader = $newuser->reader;
	my $choice = '';
	eval {
		$choice = $reader->line("Your choice, or 'done'? ");
		if (defined($choice)) {
			chomp($choice);
			$choice = lc $choice;
		}
	};
	if ($@) {
		die $@ unless ($@ =~ /^S:INT:/);
		Newuser::Exit::confirm_and_exit;
	}

	return $choice;
}

sub confirm_or_exit
{
	my $self = shift;
	my $newuser = $self->newuser;
	my $user = $newuser->user;

	$self->show;
	my $choice = '';
	do {
		last unless (defined($choice = $self->get_choice_or_interrupt));
		if  (defined(my $datum = $user->accessor($choice))) {
			$datum->get;
			$self->help;
		} elsif ($choice eq 'help') {
			$self->help;
		} elsif ($choice eq 'show') {
			$self->show;
		} elsif ($choice eq 'quit') {
			Newuser::Exit::confirm_and_exit;
		} elsif ($choice eq 'done' || $choice eq "'done'") {
			# ...
		} else {
			print STDERR "Invalid choice; try again.\n";
		}
	} while ($choice ne 'done');
}

1;
