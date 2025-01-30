#!/usr/bin/perl -T
#
# Create a "users.xhtml" web page from the list of installed
# accounts, plus an "external" list and a list of accounts to
# specifically ignore.  If a user's ~/www directory has a
# ".dontlist" file in it, then we don't list it.
#

use warnings;
use strict;

use CGI;

our $MINUID = 131073;
our $NOBODY = 'nobody';

open SHELLS, "/etc/shells" || die "Can't open /etc/shells: $!";
our @shells = grep {chomp; s/\s+//; s/#.*//; !/^$/} <SHELLS>;
close SHELLS;

sub badhome
{
	my $dir = shift;
	return ($dir eq '/nonexistent' || $dir eq '/' || !(-d $dir && -x $dir));
}

sub badshell
{
	my $shell = shift;

	return !(grep {$shell eq $_} @shells);
}

our @INDEX_FORMATS = ('xhtml', 'html', 'htm', 'php5', 'php', 'pl', 'shtml', 'cgi');
sub haswebpage
{
	my $webdir = shift;
	return undef unless (-d $webdir && -x $webdir) || (-e $webdir . '/.dontlist');
	foreach my $format (@INDEX_FORMATS) {
		my $index_file = $webdir . '/index.' . $format;
		return $index_file if (-f $index_file && -r $index_file);
	}
	return undef;
}

my %pages;
my %realnames;
setpwent;
while (my ($login, $uid, $gecos, $homedir, $shell) = (getpwent)[0,2,6,7,8]) {
	if ($gecos =~ /([^,]*),?.*/) {
		$realnames{$login} = $1;
	}
	next if ($uid < $MINUID ||
	         $login eq $NOBODY ||
	         badhome($homedir) ||
	         badshell($shell));
	my $webdir = $homedir . '/www';
	next unless haswebpage $webdir;
	$pages{$login} = '/~' . $login;
}
endpwent;

if (open(USERURLS, "/cyberspace/etc/user.urls")) {
	map {
		my ($login, $url, $gecos) = split(/:/);
		my ($homedir, $shell) = (getpwnam($login))[7,8];
		if (!badshell($homedir) && !badshell($shell)) {
			$pages{$login} = $url if defined($url);
			$realnames{$login} = $gecos if defined($gecos);
		}
	} grep {chomp; s/\s+//; s/#.*//; !/^$/} <USERURLS>;
}

if (open(IGNORED, "/cyberspace/etc/web.ignore")) {
	map { delete $pages{$_} } grep {chomp; s/\s+//; s/#.*//; !/^$/} <IGNORED>;
	close IGNORED;
}

my %alphausers;
foreach my $key (sort keys %pages) {
	my $first = substr($key, 0, 1);
	push @{ $alphausers{$first} }, $key;
}

#
# Now, we have all the data we need.  Generate output.
#

print <<'EOF';
<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
  "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en">
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <meta http-equiv="Content-Style-Type" content="text/css" />
  <meta name="author" content="grex.org" />

  <link rel="icon" href="/favicon.ico" type="image/x-icon" />

  <script type="text/javascript"></script>

  <style type="text/css" media="screen">
    @import "style/default.css";
  </style>

  <title>Grex user web page list</title>
</head>
<body>
<div id="wrap">
  <div id="header">
    <h1><a><span id="grex"><em>Grex</em></span> user web page list</a></h1>
  </div>
  <div id="menu">                                                                                                       
    <ul>
      <li class="first"><a href="http://grex.org/" accesskey="1">Home</a></li>
      <li><a href="grex.xhtml" accesskey="2">About Us</a></li>
      <li><a href="newuser/index.xhtml" accesskey="3">Get a Free Account</a></li>
      <li><a href="faq.xhtml" accesskey="4">FAQ</a></li>
      <li><a href="member.xhtml" accesskey="5">Become a Member</a></li>
      <li><a href="contact.xhtml" accesskey="6">Contact Us</a></li>
    </ul>
  </div>

  <div id="content">
  <p>The following is a list of Grexers who have home pages
  either on Grex or elsewhere on the Web.  If you want to put
  your own page on Grex, read the <a href="wwwfaq.xhtml">Grex
  WWW FAQ</a>.</p>

EOF

our @ALPHABET = split(//, 'abcdefghijklmnopqrstuvwxyz');
print '  <p>', "\n";
foreach my $letter (@ALPHABET) {
	print '    ';
	print('<a href="#', $letter, '">') if defined($alphausers{$letter});
	print uc($letter);
	print '</a>' if defined($alphausers{$letter});
	print "\n";
}
print '  </p>', "\n";

print <<'EOF';

  <p>Please mail
  <a href="mailto:webmaster@grex.org">webmaster@grex.org</a>
  if you'd like your page to be added to this list.</p>

  <hr />

EOF

foreach my $letter (@ALPHABET) {
	print '  <h1 id="', $letter, '">', uc($letter), '</h1>', "\n";
	next unless defined $alphausers{$letter};
	print '  <ul>', "\n";
	foreach my $login (@{ $alphausers{$letter} }) {
		print '    <li><a href="', $pages{$login}, '">', $login, '</a> - ',
		    CGI::escapeHTML($realnames{$login}), '</li>', "\n";
	}
	print '  </ul>', "\n";
	print "\n";
}

print <<'EOF';
  </div>

</div>
<div>
  <span id="feedback">We welcome your
    <a href="mailto:webmaster@grex.org">feedback</a>.</span>
</div>
</body>
</html>
EOF
