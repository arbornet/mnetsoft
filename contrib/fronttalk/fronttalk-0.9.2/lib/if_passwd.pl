# Copyright 2005, Jan D. Wolter, All Rights Reserved.


#  ft_passwd($newpasswd)
#  Change the user's password.  Returns error message on failure, undef on
#  success.

sub ft_passwd {
    my ($pass)= @_;
    my ($page, $tok, $err);

    $page= ft_runscript($sysh, "fronttalk/passwd",
	"new=".ft_httpquote($pass), 1);

    print "PASSWD:\n$page\n" if $debug;

    return $ft_errmsg if $ft_err;

    while ($tok= ft_parse(\$page))
    {
	if ($tok->{TOKEN} eq 'ERROR')
	{
	    $err.= ft_unquote($tok->{MSG})."\n";
	}
	elsif ($tok->{TOKEN} eq 'DONE')
	{
	}
    }
    return $err;
}


1;
