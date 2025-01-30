#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Program.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use POSIX qw( setsid );

package Newuser::Program;

sub run
{
	my @cmd = @_;
	die "U:Empty pipe command" unless @cmd;
	my $pid = open(PIPE, '-|', @cmd);
	die "R:RUN:Pipe command failed: @cmd: $!" unless defined $pid and $pid > 0;
	my @output = <PIPE>;
	close PIPE;
	return join('', @output);
}

sub pipeto
{
	return pipetoproc(sub { exec { $_[0] } @_; exit 1; }, @_);
}

sub pipetoproc
{
	my $procref = shift;
	my @cmd = @_;
	my $pid = open(my $prog, '|-');
	die "R:PIPETOPROC:fork" unless defined $pid and $pid >= 0;
	if ($pid == 0) {
		POSIX::setsid;
		&$procref(@cmd);
		print STDERR "proc returned!\n";
		exit 1;
	}
	return($prog);
}

1;
