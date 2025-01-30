#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Printer.pm 1006 2010-12-08 02:38:52Z cross $
#
# Print information.
#

use warnings;
use strict;

package Newuser::Printer;

use Newuser::Configuration;
use Newuser::Template;
use Newuser::Template::String;

our $EX_INSECURE_FILE = "U:Attempt to print an insecure file";

sub new
{
	my $klass = shift;
	my $self = {};
	bless($self, ref($klass) || $klass);
	return $self;
}

sub printobject
{
	my $template = shift;
	my $context = shift;
	eval {
		$template->prefix(Newuser::Configuration::infodir);
		$template->process($context);
	};
	if ($@) {
		die $@ unless ($@ =~ /^S:INT:/);
	}
}

sub file
{
	my $self = shift;
	my $file = shift;
	my $context = shift;

	die $EX_INSECURE_FILE . ': ' . $file if ($file =~ /\.\.\//);
	my $template = new Newuser::Template($file);
	printobject($template, $context);
}

sub string
{
	my $self = shift;
	my $message = shift;
	my $context = shift;

	my $template = new Newuser::Template::String($message);
	$template->prefix(Newuser::Configuration::infodir);
	$template->process($context);
}

1;
