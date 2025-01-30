#!/usr/bin/perl -T
#
# Create a "users.xhtml" web page from the list of installed
# accounts, plus an "external" list and a list of accounts to
# specifically ignore.  If a user's ~/www directory has a
# ".dontlist" file in it, then we don't list it.
#

use warnings;
use strict;

our $MINUID = 131073;
our $NOBODY = 'nobody';
our %pages;

open SHELLS, "/etc/shells" || die "Can't open /etc/shells: $!";
our @shells = grep {chomp; s/\s+//; s/#.*//; !/^$/} <SHELLS>;
close SHELLS;

sub baddir
{
	my $dir = shift;
	return ($dir eq '/nonexistent' || $dir eq '/' || !(-d $dir && -x $dir));
}

sub badshell
{
	my $shell = shift;
	return !(grep {$shell eq $_} @shells);
}

setpwent;
while (my ($login, $uid, $homedir, $shell) = (getpwent)[0,2,7,8]) {
	my $webdir = $homedir . '/www';
	next if ($uid < $MINUID ||
	         $login eq $NOBODY ||
	         baddir($webdir) ||
	         badshell($shell));
	$pages{$login} = $webdir
}
endpwent;

if (open(IGNORED, "/cyberspace/etc/web.ignore")) {
	map { delete $pages{$_} } grep {chomp; s/\s+//; s/#.*//; !/^$/} <IGNORED>;
	close IGNORED;
}

foreach my $login (keys %pages) {
	my $link = '/var/www/users/' . $login;
	unlink($link);
	symlink($pages{$login}, $link) || die "symlink failed: $!";
}
