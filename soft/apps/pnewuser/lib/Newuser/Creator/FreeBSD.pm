#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: FreeBSD.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use JSON;
use POSIX;

use Newuser::Accessor;
use Newuser::Creator;
use Newuser::Isodate;

package Newuser::Creator::FreeBSD;
our @ISA = qw( Newuser::Creator );

Newuser::Accessor::define(__PACKAGE__,
	'account', 'newuser', 'setquota', 'useradd');

my $NUSERADD = $::PNUHOME . '/bin/nuseradd';
my $SETQUOTA = $::PNUHOME . '/bin/setquota';

#
# Note that the following routine is used in a child process to
# read lines from the parent a byte at a time; we do this to avoid
# interactions between the pipe and perl's IO buffering; in
# particular, the child will exec another program, and we want the
# remaining data on the pipe to be accessable to that program
# instead of read into the perl interpreter's input buffer.
#
sub getline
{
	my $line = "";
	my $byte;
	my $nread;
	while (($nread = sysread(STDIN, $byte, 1)) == 1 && $byte ne "\n") {
		$line .= $byte;
	}
	if ($nread <= 0) {
		exit 1;
	}
	return $line;
}

# Untaint it.  We could just use double-quote expansion, but this is more fun.
sub untaint
{
	return $1 if shift =~ /(.*)/s;
}

sub runuseradd
{
	chomp(my $login = getline,
	      my $gecos = getline,
              my $dir = getline,
	      my $skeldir = getline,
	      my $shell = getline);
	#
	# Please note that we have not yet read the hashed password; that's
	# read by nuseradd from it's stdin.
	#
	$gecos = 'anonymous' if $gecos eq '(no value given)';
	exec($NUSERADD,
		'-l', untaint($login),
		'-c', untaint($gecos),
		'-d', untaint($dir),
		'-k', untaint($skeldir),
		'-s', untaint($shell),
		'-H', 0) || print STDERR "exec($NUSERADD) failed: $!\n";
	exit 1;
}

sub new
{
	my $klass = shift;
	my $newuser = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->newuser($newuser);
	$self->useradd(Newuser::Program::pipetoproc(\&runuseradd));
	$self->setquota(Newuser::Program::pipeto($SETQUOTA));
	return $self;
}

sub quotas
{
	my $self = shift;
	my $account = $self->account;
	my $user = $account->user;
	my $login = $user->value('login');
	my $setquota = $self->setquota;
	$self->setquota(0);
	unless (defined($setquota)) {
		$setquota = Newuser::Program::pipeto($SETQUOTA);
	}
	my $qrefs = $account->quotas;
	foreach my $qref (@{ $qrefs }) {
		print $setquota join(' ', $login, @{ $qref }), "\n";
	}
	close($setquota);
	my $status = POSIX::WEXITSTATUS($?);
	return ($status == 0);
}

sub adduser
{
	my $self = shift;
	my $account = $self->account;
	my $user = $account->user;
	my $newuser = $self->newuser;
	my $lockdir = $newuser->lockdir;

	$account->addtime(Newuser::Isodate::now);

	my $useradd = $self->useradd;
	$self->useradd(0);
	unless ($useradd) {
		$useradd = Newuser::Program::pipetoproc(\&runuseradd);
	}
	print $useradd $user->value('login'), "\n";
	print $useradd $user->value('name'), "\n";
	print $useradd $account->homedir, "\n";
	print $useradd $lockdir->lockdir, "\n";
	print $useradd $user->value('shell'), "\n";
	print $useradd $account->password->hashed, "\n";
	close $useradd;
	my $status = POSIX::WEXITSTATUS($?);
	if ($status == 65) {   # 65 == EX_DATAERR => user exists.
		die 'R:RECOVERABLE:user exists';
	}
	return ($? == 0);
}

1;
