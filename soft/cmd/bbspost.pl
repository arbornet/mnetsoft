#!/usr/bin/perl

$VERSION = "0.9.2";
#$debug = 1;

use lib "/usr/local/lib/fronttalk-0.9.2";

require "config.pl";
require "if_core.pl";
require "net_core.pl";
require "if_post.pl";

if (@ARGV != 2) {
	die "Usage: $0 conference itemno.\n";
}
$conf = shift;
$item = shift;

# Initialize the fronttalk interface.
ft_init($SYSLIST, undef);
if ($ft_err) {
	die "$0: Cannot initialize interface: $ft_errmsg.\n";
}

# Connect to backtalk.
$sysalias = undef;
$sysh = ft_connect($sysalias);
if ($ft_err || !defined($sysh)) {
	die "$0: Cannot connect to system \"$sysalias\": $ft_errmsg.\n";
}

# Login to the conference in question.
$confh = ft_openconf($sysh, $conf);
if ($ft_err || !defined($confh)) {
	die "$0: Cannot open conference \"$conf\": $ft_errmsg.\n";
}

# Collect the text of the response.
while (<>) {
	$text .= $_;
}

# Post it.
$respno = ft_respond($confh, $item, $text, "");
if ($ft_err || !defined($respno)) {
	die "$0: Cannot enter response: $ft_errmsg.\n";
}

print "Entered response #$respno in item $item in $conf.\n" if $debug;
