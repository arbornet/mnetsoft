# Copyright 2002, Jan D. Wolter, All Rights Reserved.  #


# ft_changefile($file, $text, $confh)
#  Overwrite a Backtalk text file with a new text.  Files may be any of
#      "cflist"      - user's conference hot list
#      "cfonce"      - user's rc file
#      "cfrc"        - user's other rc file
#      ".plan"       - user's .plan file
#      "bull"        - conference bulletin file
#      "index"       - conference index file
#      "login"       - conference login screen
#      "logout"      - conference logout screen
#      "ulist"       - conference user list file
#      "glist"       - conference group list file
#      "secret"      - conference password
#      "welcome"     - conference welcome file
#      "confrc"      - conference rc file
#      "bbsrc"       - system rc file
#      "dflt.cflist" - system default conference hot list
#      "motd.html"   - system motd file
#  The confh argument need be given only for conference files.
#  Returns an error message on failure, undef on success.

sub ft_changefile {
    my ($file, $text, $confh)= @_;
    my ($page,$err,$qarg);

    $qarg= 'file='.ft_httpquote($file).'&text='.ft_httpquote($text);
    $qarg.= '&conf='.$confh->{alias} if $confh;

    $page= ft_runscript($sysh, "fronttalk/changefile", $qarg, 1);
    return $ft_errmsg if ($ft_err);

    print "CHANGEFILE:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'ERROR')
	{
	    $err.= ft_unquote($tok->{MSG})."\n";
	}
    }
    return $err;
}


# ft_changename($newname, $confh)
#  Change a user's conference name.  Returns an error message on failure,
#  undef on success.

sub ft_changename {
    my ($newname, $confh)= @_;
    my ($page,$err);

    $qarg= 'newname='.ft_httpquote($newname).'&conf='.$confh->{alias};

    $page= ft_runscript($sysh, 'fronttalk/changename',
	'newname='.ft_httpquote($newname).'&conf='.$confh->{alias}, 1);
    return $ft_errmsg if ($ft_err);

    print "CHANGENAME:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'ERROR')
	{
	    $err.= ft_unquote($tok->{MSG})."\n";
	}
    }
    return $err;
}


1;
