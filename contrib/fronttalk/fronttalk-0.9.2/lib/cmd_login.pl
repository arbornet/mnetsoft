# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_conf.pl";

sub cmd_login {
    if (!$sysh->{amanon})
    {
    	print "Already Logged in\n";
	return 1;
    }
    if (ft_login($sysh, \&login_callback))
    {
    	print "Logged in as $sysh->{username}\n";
	ft_userinfo($sysh);

	# reload rc files now that we are a new user
	sysrc();

	ft_updateconf($confh);
	if ($ft_err)
	{
	    print "$ft_errmsg\n"
	}
	else
	{
	    # Rerun conference rc files, now that we are logged in.
	    confrc();
	}
    }
    elsif ($ft_err)
    {
    	print "$ft_err.\n";
    }
    else
    {
    	print "Remaining anonymous.\n";
    }
    return 1;
}

1;
