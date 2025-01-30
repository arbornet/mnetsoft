#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Login.pm 1006 2010-12-08 02:38:52Z cross $
#

use warnings;
use strict;

use Mail::Alias;

use Newuser::Configuration;
use Newuser::Data;

package Newuser::Data::Login;
our @ISA = qw( Newuser::Data );

Newuser::Accessor::define(__PACKAGE__, 'last_error');

my $PREFACE = '
Enter your desired login name.  It should be at least two characters
long and consist only of lower case alphanumeric characters and the
underbar (_) character.  It must begin with a letter.

';

my $PROMPT = 'Enter your desired login name: ';

my $ERROR = 'That is an invalid login name; please try again.';
my $ERROR_BAD = 'That login name is not permitted; please pick another.';
my $ERROR_USED = 'That login name is already in use; please pick another.';

sub key
{
	return 'login';
}

sub getvalue
{
	my $self = shift;
	my $reader = $self->reader;
	$self->value($reader->line($PROMPT));
}

sub isactivealias
{
	my $self = shift;
	my $login = $self->value;
	my $aliases = Mail::Alias->new;
	return 1 if $aliases->exists($login);
	foreach my $alias_file (Newuser::Configuration::aliasfiles) {
		$aliases = Mail::Alias->new($alias_file);
		return 1 if $aliases->exists($login);
	}
	return 0;
}

sub isbadname
{
	my $self = shift;
	my $login = $self->value;
	my $isbad = 0;
	foreach my $badnames_file (Newuser::Configuration::badnamesfiles) {
		open(BADNAMES, $badnames_file) || next;
		while (my $badname = <BADNAMES>) {
			chomp $badname;
			$badname =~ s/\#.*//;
			$badname =~ s/^\s+//;
			$badname =~ s/\s+$//;
			$isbad = 1 if $badname eq $login;
		}
		close(BADNAMES);
	}
	return $isbad;

}

sub name_matches_bad_pattern
{
	my $self = shift;
	my $login = $self->value;
	my $isbad = 0;
	for my $patterns_file (Newuser::Configuration::badpatternsfiles) {
		open(BADPATTERNS, $patterns_file) || next;
		while (my $badpattern = <BADPATTERNS>) {
			chomp $badpattern;
			$badpattern =~ s/\#.*//;
			$badpattern =~ s/^\s+//;
			$badpattern =~ s/\s+$//;
			$isbad = 1 if $login =~ $badpattern;
		}
		close(BADPATTERNS);
	}
	return $isbad;
}

sub isvalid
{
	my $self = shift;
	my $login = $self->value;
	if (getpwnam($login) || $self->isactivealias) {
		$self->last_error($ERROR_USED);
		return 0;
	}
	if ($self->isbadname || $self->name_matches_bad_pattern) {
		$self->last_error($ERROR_BAD);
		return 0;
	}
	if ($login =~ /[^a-z0-9_]/ || $login =~ /^[^a-z]/) {
		$self->last_error($ERROR);
		return 0;
	}
	return 1;
}

sub error
{
	my $self = shift;
	my $error = ($self->last_error || $ERROR);
	print STDERR $error, "\n";
}

sub preface
{
	my $self = shift;
	my $printer = $self->printer;
	$printer->string($PREFACE);
}

1;
