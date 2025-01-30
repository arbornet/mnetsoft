#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Template.pm 1006 2010-12-08 02:38:52Z cross $
#
# Newuser template - wrapper around Text::Template to provide
# text generation.
#

use warnings;
use strict;

use Text::Template;
use Text::Template::Preprocess;

use Newuser::Accessor;

package Newuser::Template;

Newuser::Accessor::define(__PACKAGE__, 'prefix');

# This is the character that starts a comment.  Change to suite.
my $COMMENT_CHARACTER = '#';

#
# Note that the comment pattern appears in a loop.  This is to handle
# the case where you have a line that ends in a comment, followed
# immediately by a comment line.  The first expression will create
# a new string, but the g modifier won't attempt to modify it.
#
sub strip_comments
{
	while (/(\A|^|[^\\])$COMMENT_CHARACTER[^\n]*(\n|\Z)/) {
		s/(\A|^|[^\\])$COMMENT_CHARACTER[^\n]*(\n|\Z)/$1/g;
	}
	s/\\($COMMENT_CHARACTER)/$1/g;
}

sub new
{
	my $klass = shift;
	my $filename = shift;

	die "Attempt to create an empty template" unless ($filename);

	my $self = {};
	bless($self, ref($klass) || $klass);
	$self->{filename} = $filename;

	return $self;
}

sub pause
{
	my $self = shift;
	my $prompt = shift;

	$prompt = "Press <ENTER> to continue...." unless $prompt;
	print $prompt;
	$self->readenter;
}

sub readenter
{
	my $enter = <>;
	unless ($enter) {
		# EOF?
		print "End of file encountered!\n";
	}
}

sub maketemplate
{
	my $self = shift;
	my $source = shift;
	return new Text::Template::Preprocess(
	    SOURCE => $source,
	    UNTAINT => 1,
	    BROKEN => sub {
		my %args = @_;
		die $args{error};
	    }
	);
}

sub error
{
	return Text::Template::TTerror;
}

sub include
{
	my $self = shift;
	$self->includeto(@_, \*STDOUT);
}

sub includeto
{
	my $self = shift;
	my $varref = shift;
	my $file = shift;
	my $output = shift;

	my $prefix = $self->{prefix};
	$file = $prefix . '/' . $file if ($file !~ /^\// && $prefix);

	my $template = $self->maketemplate($file);
	die "Cannot create template from $file: " . $self->error unless ($template);
	$template->preprocessor(\&strip_comments);
	my $status = $template->fill_in(HASH => $varref, OUTPUT => $output,
		_PREPEND => 'use warnings; use string;');
	die "R:Error processing template: " . $self->error unless($status);
}

sub process
{
	my $self = shift;
	my $varref = (shift || {});
	$self->processto($varref, \*STDOUT);
}

sub processto
{
	my $self = shift;
	my $varref = shift;
	my $output = shift;

	unless ($self->{filename}) {
		die "R:Attempt to process an uninitialized template";
	}
	$varref->{include} = sub { $self->include($varref, @_); ''; };
	$varref->{pause}   = sub { $self->pause; ''; };
	$varref->{enter}   = sub { $self->readenter; ''; };
	$self->includeto($varref, $self->{filename}, $output);
}

1;
