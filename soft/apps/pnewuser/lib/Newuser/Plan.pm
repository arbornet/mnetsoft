#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Plan.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use IO::File;

use Newuser::Accessor;
use Newuser::Isodate;

package Newuser::Plan;

Newuser::Accessor::define(__PACKAGE__, 'account');

sub new
{
	my $klass = shift;
	my $account = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->account($account);
	return($self);
}

sub printparagraph
{
	my $f = shift;
	my $header = shift;
	my $paragraph = shift;
	print $f $header, ":\n\t", join("\n\t", split("\n", $paragraph)), "\n";
}

sub printstring
{
	my $f = shift;
	my $header = shift;
	my $string = shift;
	print $f $header, ': ', $string, "\n";
}

sub print
{
	my $self = shift;
	my $filehandle = shift;
	my $account = $self->account;
	my $user = $account->user;

	my $f = defined($filehandle) ? $filehandle : *STDOUT;

	printstring $f, 'Full name',  $user->value('name');
	print $f 'Registered at ', $account->starttime, ' on ', $account->tty, "\n";
	printparagraph $f, 'Address', $user->value('address');
	printstring $f, 'Telephone', $user->value('telephone');
	printparagraph $f, 'Occupation', $user->value('occupation');
	printparagraph $f, 'Computers', $user->value('computers');
	printstring $f, 'Gender', $user->value('gender');
	printstring $f, 'Birthday', $user->value('birthday');
	printparagraph $f, 'Interests', $user->value('interests');
}

sub create
{
	my $self = shift;
	my $dir = $1 if shift =~ /(.*)/;
	my $account = $self->account;
	my $user = $account->user;
	my $hidden = $user->value('hidden');
	my $mode = ($hidden eq 'yes') ? 0600 : 0644;
	my $file = $dir . '/.plan';
	unlink($file);
	my $fh = new IO::File($file, Fcntl::O_WRONLY | Fcntl::O_CREAT, $mode);
	die "R:IGNORE:Can't open $file: $!" unless defined($fh);
	$self->print($fh);
}

1;
