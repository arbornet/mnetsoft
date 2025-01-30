#!/usr/bin/perl -T
#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: pnewuser.pl 1026 2011-01-08 12:28:58Z cross $
#
# Gather information and create new accounts automatically.
#

use warnings;
use strict;

our $PNUHOME;
BEGIN {
#
# Disable all signals while we start up.
#
$SIG{$_} = 'IGNORE' for keys(%SIG);

#
# Clear and set up the environment, saving only terminal and
# timezone, if available.
#
my $TERM = $ENV{'TERM'};
my $TZ   = $ENV{'TZ'};

%ENV = ();
$ENV{'PATH'} = '/usr/sbin:/sbin:/usr/bin:/bin';
$ENV{'TERM'} = $TERM if $TERM;
$ENV{'TZ'}   = $TZ   if $TZ;
$ENV{'PERL_DL_NONLAZY'} = '1';

# Set up the global PNUHOME.
$PNUHOME = '@@PNUHOME@@';
}

# Set up our include path.
use lib "$PNUHOME/lib";

# Now, the main program.
use Newuser;

chdir(Newuser::rundir) || die "Can't chdir: $!\n";

my $VERSION = '$Revision: 1026 $';
my $DATE    = '$Date: 2011-01-08 07:28:58 -0500 (Sat, 08 Jan 2011) $';

$VERSION =~ s/^.*:\s+(\d+).*$/$1/;
$DATE    =~ s/^.*\((.*)\).*$/$1/;

# Print the banner and go to it.
print "pnewuser program ($DATE revision $VERSION)\n\n";

my $newuser = new Newuser;
$newuser->main;
