#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Lockdirectory.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Fcntl;
use File::Find;
use IO::File;

use Newuser::Accessor;
use Newuser::Configuration;

package Newuser::Lockdirectory;

Newuser::Accessor::define(__PACKAGE__, 'basename');

sub TO_JSON
{
	my $self = shift;
	return $self->homedir;
}

sub new
{
	my $klass = shift;
	my $login = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->basename($login);
	return $self;
}

sub lockdir
{
	Newuser::Configuration::lockdir . '/' . shift->basename;
}

sub copyfile
{
	my $from = shift;
	my $to = shift;
	my $mode = shift;
	my $f = new IO::File($from, Fcntl::O_RDONLY)
		or die "R:UNRECOVERABLE:sysopen($from): $!";
	my $t = new IO::File($to, Fcntl::O_CREAT | Fcntl::O_WRONLY, $mode)
		or die "R:UNRECOVERABLE:sysopen($to): $!";
	$f->binmode;
	$t->binmode;
	while ((my $nread = sysread($f, my $buffer, 8192)) > 0) {
		syswrite($t, $buffer, $nread);
	}
}

sub lock
{
	my $self = shift;
	my $lockdir = $1 if $self->lockdir =~ /(.*)/;
	my $skeldir = $1 if Newuser::Configuration::skeldir =~ /(.*)/;
	my %options = (
		no_chdir => 1,
		untaint => 1,
		wanted => sub {
			my $skelname = $1 if /(.*)/;
			my $name = $1 if $skelname =~ /$skeldir\/+(.*)/;
			return unless $name;
			my $mode = (lstat $skelname)[2] & 07777;
			my $lockname = $lockdir . '/' . $name;
			if (-d $skelname) {
				mkdir($lockname, $mode) || die "S:mkdir($lockname): $!";
			} elsif (-f $skelname) {
				copyfile($skelname, $lockname, $mode);
			}
		}
	);
	mkdir($lockdir, 0755) || die "R:RECOVERABLE:Can't create lock directory: $!\n";
	eval {
		File::Find::find(\%options, $skeldir);
	};
	# Be extra careful about special cases.
	chmod(0700, $lockdir . '/.ssh');
	chmod(0700, $lockdir . '/.pgp');
	chmod(0700, $lockdir . '/.gpg');
	chmod(0711, $lockdir . '/.cfdir');
}

sub unlock
{
	my $self = shift;
	my $lockdir = $1 if $self->lockdir =~ /(.*)/;
	my %options = (
		no_chdir => 1,
		untaint => 1,
		wanted => sub {
			my $lockname = $1 if /(.*)/;
			if (-d $lockname) {
				rmdir($lockname);
			} else {
				unlink($lockname);
			}
		}
	);
	File::Find::finddepth(\%options, $lockdir);
	rmdir($lockdir);
}

1;
