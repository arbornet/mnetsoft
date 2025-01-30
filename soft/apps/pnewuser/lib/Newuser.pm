#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Newuser.pm 1006 2010-12-08 02:38:52Z cross $
#
# This is the main `driver' of the program.
#

use warnings;
use strict;

use Newuser::Account;
use Newuser::Accessor;
use Newuser::Authenticator;
use Newuser::Confirmation;
use Newuser::Creator::FreeBSD;
use Newuser::Data_Factory;
use Newuser::Homedirectory;
use Newuser::Lockdirectory;
use Newuser::Log;
use Newuser::Mailer;
use Newuser::Printer;
use Newuser::Reader;
use Newuser::Signals;
use Newuser::User;

package Newuser;

Newuser::Accessor::define(__PACKAGE__,
	'account', 'authenticator', 'confirmation', 'creator', 'homedir',
	'lockdir', 'log', 'mailer', 'plan', 'printer', 'reader');

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	return $self;
}

sub rundir
{
	Newuser::Configuration::emptydir;
}

sub initialize
{
	my $self = shift;

	Newuser::Signals::initialize;
	chdir($self->rundir) || die "Can't chdir: $!\n";

	$self->printer(new Newuser::Printer);
	$self->reader(new Newuser::Reader 'pnewuser');
	$self->log(new Newuser::Log);
	my $account = new Newuser::Account;
	$account->user(new Newuser::User);
	$self->account($account);
	$self->mailer(new Newuser::Mailer);
	$self->authenticator(new Newuser::Authenticator($self));
	$self->confirmation(new Newuser::Confirmation($self));
	$self->creator(new Newuser::Creator::FreeBSD($self));
	my $data_factory = new Newuser::Data_Factory($self);
	$self->{data_factory} = $data_factory;
	@{ $self->{mandatory_data} } = ($data_factory->mandatory);
	@{ $self->{optional_data} } = ($data_factory->optional);
	@{ $self->{hidden_data} } = ($data_factory->hidden);
	@{ $self->{login_data} } = ($data_factory->login);
	@{ $self->{unchangeable_data} } = ($data_factory->unchangeable);

	Newuser::Signals::enable_interrupts;
}

sub main
{
	eval {
		my $self = shift;
		$self->initialize;
		$self->welcome;
		$self->authenticate;
		$self->mandatory;
		$self->optional;
		$self->hidden;
		$self->login;
		$self->unchangeable;
		$self->confirm_and_create;
		$self->cleanup;
		$self->done;
	};
	if ($@) {
		if ($@ =~ /^S:INT:/ ||
		    $@ =~ /^U:CAPTCHA/)
		{
			exit 1;
		}
		print STDERR "An error occurred:\n";
		print STDERR $@;
	}
}

sub confirm_and_create
{
	my $self = shift;
	my $loop;
	do {
		$loop = 0;
		eval {
			$self->confirm;
			$self->lock;
			$self->create_plan;
			$self->create_and_send_mail;
		};
		if ($@) {
			die unless ($@ =~ /^R:RECOVERABLE/ || $@ =~ /^U:/);
			$self->unlock;
			if ($@ =~ /^R:RECOVERABLE:user exists/) {
				$self->printfile("collision");
			}
			$loop = 1;
		}
	} while($loop);
}

sub lock
{
	my $self = shift;
	my $user = $self->user;
	my $login = $user->value('login');
	my $lockdir = new Newuser::Lockdirectory($login);
	$self->lockdir($lockdir);
	Newuser::Signals::disable_interrupts;
	$lockdir->lock;
}

sub create_plan
{
	my $self = shift;
	my $lockdir = $self->lockdir;
	my $plan = $self->plan;
	$plan->create($lockdir->lockdir);
}

sub unlock
{
	my $self = shift;
	my $lockdir = $self->lockdir;
	$lockdir->unlock;
	Newuser::Signals::enable_interrupts;
}

sub welcome
{
	shift->printfile('welcome');
}

sub authenticate
{
	my $self = shift;
	my $auth = $self->authenticator;
	$auth->authenticate_or_die;
}

sub mandatory
{
	my $self = shift;
	$self->section("mandatory", $self->{mandatory_data});
}

sub optional
{
	my $self = shift;
	$self->section("optional", $self->{optional_data});
}

sub other
{
	my $self = shift;
	$self->section("other", $self->{other_data});
}

sub unchangeable
{
	my $self = shift;
	mapget($self->{unchangeable_data});
	$self->generated;
}

sub generated
{
	my $self = shift;
	my $account = $self->account;
	my $user = $self->user;
	my $login = $user->value('login');
	my $homedir = new Newuser::Homedirectory $login;
	$self->homedir($homedir);
	$account->homedir($homedir->homedir);
}

sub section
{
	my $self = shift;
	my $printfile = shift;
	$self->printfile($printfile);
	mapget(shift);
}

sub mapget
{
	my $reference = shift;
	map { $_->get } @{ $reference };
}

sub printfile
{
	my $self = shift;
	my $file = shift;

	my $printer = $self->printer;
	$printer->file($file);
}

sub hidden
{
	mapget(shift->{hidden_data});
}

sub login
{
	mapget(shift->{login_data});
}

sub confirm
{
	my $self = shift;
	my $account = $self->account;
	my $plan = new Newuser::Plan($account);
	$self->plan($plan);
	my $confirmation = $self->confirmation;
	$confirmation->plan($plan);
	$confirmation->confirm_or_exit;
}

sub send_mail
{
	my $self = shift;
	my $account = $self->account;
	my $mailer = $self->mailer;
	my $log = $self->log;
	$self->printfile('emailing');
	if (not $mailer->send_mail($account)) {
		$log->send_mail_failure($account);
		return;
	}
	$log->send_mail_success($account);
}

sub create_account
{
	my $self = shift;
	my $account = $self->account;
	my $log = $self->log;
	my $creator = $self->creator;
	$self->printfile('creating');
	if (not $creator->create($account)) {
		$log->creation_failure($account);
		return 0;
	}
	$log->creation_success;
	return 1;
}

sub create_and_send_mail
{
	my $self = shift;
	if ($self->create_account) {
		$self->send_mail;
	}
}

sub cleanup
{
	my $self = shift;
	$self->printfile('cleaning');
	my $lockdir = $self->lockdir;
	$lockdir->unlock;
}

sub done
{
	my $self = shift;
	$self->printfile('done');
}

sub user
{
	return shift->account->user;
}

sub logger
{
	return shift->log->logger;
}

1;
