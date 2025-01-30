#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Logger.pm 1006 2010-12-08 02:38:52Z cross $
#
# Logging interface for pnewuser.
#

use warnings;
use strict;

use Fcntl qw( :flock );
use JSON;
use Sys::Syslog;

use Newuser::Accessor;
use Newuser::Configuration;
use Newuser::Program;

# We start with the support classes.
package Newuser::Logger::Syslog;

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	my $facility = Newuser::Configuration::syslog;
	Sys::Syslog::openlog('pnewuser', 'pid', $facility || 'local0');
	return $self;
}

sub log
{
	my $self = shift;
	Sys::Syslog::syslog('info', '%s', shift);
}

1;

package Newuser::Logger::File;

Newuser::Accessor::define(__PACKAGE__, 'logfile');

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	my $logfilename = Newuser::Configuration::logfile;
	open(LOG, ">>$logfilename") || die "R:LOGFILE:$logfilename";
	$self->logfile(*LOG);
	return $self;
}

sub log
{
	my $self = shift;
	my $message = shift;

	my $logfile = $self->logfile;
	flock($logfile, Fcntl::LOCK_EX);
	print $logfile $message;
	flock($logfile, Fcntl::LOCK_UN);
}

1;

package Newuser::Logger::Prog;

Newuser::Accessor::define(__PACKAGE__, 'logprog');

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	my $program = Newuser::Configuration::logprog;
	my @prog = split(/\s+/, $program);
	$self->logprog(Newuser::Program::pipeto(@prog));
	return $self;
}

sub log
{
	my $self = shift;
	my $message = shift;
	my $f = $self->logprog;
	print $f $message;
}

1;

package Newuser::Logger;

Newuser::Accessor::define(__PACKAGE__, 'filelog', 'proglog', 'syslog');

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	if (Newuser::Configuration::syslog) {
		$self->syslog(new Newuser::Logger::Syslog);
	}
	if (Newuser::Configuration::logfile) {
		$self->filelog(new Newuser::Logger::File);
	}
	if (Newuser::Configuration::logprog) {
		$self->proglog(new Newuser::Logger::Prog);
	}

	return $self;
}

sub log
{
	my $self = shift;
	my $datum = shift;
	my $json = JSON->new;
	$json->utf8->pretty->indent->allow_blessed->convert_blessed;
	my $message = $json->encode($datum);

	$self->syslog->log($message)  if defined($self->syslog);
	$self->filelog->log($message) if defined($self->filelog);
	$self->proglog->log($message) if defined($self->proglog);
}

1;
