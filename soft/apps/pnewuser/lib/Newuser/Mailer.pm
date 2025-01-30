#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Mailer.pm 1006 2010-12-08 02:38:52Z cross $
#
# Sendmail mail for pnewuser.
#

use warnings;
use strict;

use Newuser::Accessor;
use Newuser::Configuration;
use Newuser::Program;
use Newuser::Template;

package Newuser::Mailer;

my @SENDMAIL = qw( /usr/sbin/sendmail -t -O ErrorMode=q );
my $TEMPLATE = 'welcome';

Newuser::Accessor::define(__PACKAGE__, 'sendmail');

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->sendmail(Newuser::Program::pipetoproc \&runsendmail);
	return $self;
}

sub runsendmail
{
	my $errsfile = Newuser::Configuration::mailerrsfile;
	open(my $errors, '>>', $errsfile) ||
		die "R:RECOVERABLE:runsendmail: open(my \$errors, \"$errsfile\"): $!";
	open(STDOUT, '>&', $errors);
	open(STDERR, '>&', $errors);
	exec(@SENDMAIL) || die "R:UNRECOVERABLE:runsendmail: exec(\"@SENDMAIL\"): $!";
	exit 1;
}

sub send_mail
{
	my $self = shift;
	my $account = shift;
	my $user = $account->user;
	my $name = $user->value('name');
	my $login = $user->value('login');
	my $passwd = $account->password->plain;
	my $email = $user->value('email');
	my %variables = (
	    'name' => $name,
	    'login' => $login,
	    'password' => $passwd,
	    'email' => $email
	);
	$self->send_template(\%variables);
	1;
}

sub send_template
{
	my $self = shift;
	my $varref = shift;
	my $sendmail = $self->sendmail;
	my $template = new Newuser::Template($TEMPLATE, $varref);
	$template->prefix(Newuser::Configuration::messagedir);
	$template->processto($varref, $sendmail);
	close $sendmail;
}

1;
