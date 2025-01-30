#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Reader.pm 1006 2010-12-08 02:38:52Z cross $
#
# Perl/ReadLine based text reader.  Inspired by `gate'.
#

use warnings;
use strict;

package Newuser::Reader;

use Term::ReadLine;

our $MAXTEXT = 64*1024*1024;	# 64KB maximum buffer size.

sub new
{
	my $klass = shift;
	my $name = shift;

	die "P:Attempt to create a reader without a name" unless $name;

	my $self = {};
	bless($self, ref($klass) || $klass);
	#
	# Use Term::ReadLine::Stub because Term::ReadLine::Gnu
	# interacts poorly with Perl's safe signals and interrupts.
	#
	my $term = new Term::ReadLine::Stub 'Reader';
	$self->{term} = $term;
	$term->ornaments(0);
	return $self;
}

sub sanitize
{
	return shift;
}

sub trim
{
	my $line = shift || '';
	$line =~ s/^\s+//;
	$line =~ s/\s+$//;
	return $line;
}

sub line
{
	my $self = shift;
	my $prompt = shift;
	my $term   = $self->{term};
	$prompt = '>' unless $prompt;
	my $line = $term->readline($prompt);
	if (!defined($line)) {
		print STDERR "\n";
	}
	return trim(sanitize($line));
}

sub paragraph
{
	my $self = shift;
	my $heading = shift;
	my $prompt = shift;
	my $term = $self->{term};
	$prompt = '>' unless $prompt;

	print $heading, "\n";
	print "Type '^D' or '.' on a line by itself to end your input.\n";
	my $paragraph = '';
	my $line;
	while (defined($line = $term->readline($prompt))) {
		last if $line eq '.';
		$line =~ s/\s+$//;
		$paragraph .= sanitize($line) . "\n";
	}
	if (!defined($line)) {
		print STDERR "\n";
	}
	$paragraph =~ s/\A\s+//;
	$paragraph =~ s/\s+\z/\n/;
	return $paragraph;
}

1;
