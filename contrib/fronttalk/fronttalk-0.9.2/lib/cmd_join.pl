# sysrc()
#   Run or rerun the start-up rc files:  bbs/rc, remote .cfonce, local .cfonce

sub sysrc {
    # Set everything to compiled in defaults
    set_sys_dflt();

    # bbs/rc file (unless it is remote and we have noremote set)
    $cmd_source= 0;
    source_cmds($sysh->{bbsrc}) if $sysh->{direct} or flagval('remote');

    # remote .cfonce (unless we are directly connected)
    $cmd_source= 1;
    source_cmds($sysh->{cfonce}) if !$sysh->{direct} and flagval('remote');

    # local .cfonce
    $cmd_source= 2;
    source_cmds(ft_getfile($cfonce)) if $cfonce;
}


# confrc()
#   Run the conference rc for the current conference:  conf/rc, remote .cfrc,
#   local .cfrc

sub confrc
{
    # if nosource is set, do nothing
    flagval('source') or return;

    # conference rc file (unless it is remote and we have noremote set)
    $cmd_source= 3;
    source_cmds($confh->{confrc}) if $sysh->{direct} or flagval('remote');

    # remote .cfrc (unless we are directly connected)
    $cmd_source= 4;
    source_cmds($sysh->{cfrc}) if !$sysh->{direct} and flagval('remote');

    # local .cfrc
    $cmd_source= 5;
    source_cmds(ft_getfile($cfrc));
}


# join_conf($sysh, $confname, $oldconfh, $newonly)
#   Join a conference, printing out appropriate banners, and return the
#   conference handle.  If $confname is undefined, we go to the "no conf"
#   conference.  If $oldconfh is defined, it is a previous conference
#   If newonly is defined, then we should only join if there
#   new responses or items in the conference.  On failure we return undef
#   and define globals $ft_err and $ft_errmsg.

sub join_conf {
    my ($sysh, $confname, $oldconfh, $newonly)= @_;
    my ($confh);
    $ft_err= undef;

    if (defined $confname)
    {
	# get new conference handle
	$confh= ft_openconf($sysh,$confname);
	return undef if ($ft_err);

	if ($newonly and $confh->{newr} == 0 and $confh->{newi} == 0)
	{
	    ($ft_err,$ft_errmsg)= ('JC1', "No new items in $confname");
	    return undef;
	}
    }

    # Say goodbye to previous conference, if any.
    print exec_sep($defhash{loutmsg},0,$oldconfh,1) if defined $oldconfh;

    set_conf_dflt();

    if ($confh)
    {
	$confh->{curr}= $confh->{first};

	if ($debug)
	{
	    my $x;
	    print "CONFERENCE INFORMATION:\n";
	    print "alias=$confh->{alias}\n";
	    print "name=$confh->{name}\n";
	    print "login='$confh->{login}'\n";
	    print "logout='$confh->{logout}'\n";
	    print "first=$confh->{first}\n";
	    print "last=$confh->{last}\n";
	    print "newr=$confh->{newr}\n";
	    print "newi=$confh->{newi}\n";
	    print "readonly=$confh->{readonly}\n";
	    print "noenter=$confh->{noenter}\n";
	    print "amfw=$confh->{amfw}\n";
	    print "myname=$confh->{myname}\n";
	    print "\n";
	}

	# run initialization files
	confrc();

	# set command source to command line
	$cmd_source= 6;
	$remote_source= 0;

	# print conference welcome screen and counts of new items/responses
	pager_on();
	print exec_sep($defhash{linmsg},0,$confh,3);
	pager_off();
    }

    return $confh;
}


sub cmd_join {
    my ($args)= @_;
    my ($h, $sy, $oldsy);

    my ($ty,$cf)= parse_cmd(\$args,1);

    # No argument - tell current conference
    if (!defined $ty)
    {
    	print "Current conference: $confh->{name}\n";
	return 1;
    }

    # Was a system named?
    if ($cf =~ /^(.*):(.*)$/)
    {
    	$sy= $1;
	$cf= $2;
	if ($sy ne '' and $sy ne $sysh->{alias})
	{
	    $oldsy= $sysh;
	    $sysh= ft_connect($sy);
	    if ($ft_err)
	    {
	    	print "Cannot connect to $sy.\n$ft_errmsg.\n";
		$sysh= $oldsy;
		return 1;
	    }
	}
    }

    # Attempt to join a conference
    if ($h= join_conf($sysh, $cf, $confh, 0))
    {
    	$confh= $h;
    }
    else
    {
    	print "Cannot join conference $cf.\n$ft_errmsg\n";
	$sysh= $oldsy if $oldsy;
    }
    return 1;
}


sub cmd_leave {

    $confh= join_conf($sysh, undef, $confh, 0);

    return 1;
}


sub cmd_resign {

    require "if_conf.pl";
    my $err= ft_resignconf($confh);
    print "$err\n" if defined $err;
    return 1;
}


sub cmd_server {
    my ($args)= @_;
    my ($h, $oldsy);

    my ($ty,$sy)= parse_cmd(\$args,1);

    # No argument - tell current server
    if (!defined $ty)
    {
    	print "Current server: $sysh->{name}\n";
	return 1;
    }

    if ($sy eq $sysh->{alias})
    {
    	print "Already connected to $sy\n";
	return 1;
    }

    $oldsy= $sysh;
    $sysh= ft_connect($sy);
    if ($ft_err)
    {
	print "Cannot connect to $sy.\n$ft_errmsg.\n";
	$sysh= $oldsy;
	return 1;
    }
    print "Connected to $sysh->{name} server";
    print " (version $sysh->{version} - ",$sysh->{direct}?'direct':'network',')'
        if $sysh->{version};
    print "\n\n",$sysh->{banner};

    # run RC files for new system
    sysrc();

    # Attempt to join a conference
    if ($h= join_conf($sysh, '?', $confh, 0))
    {
    	$confh= $h;
    }
    else
    {
    	print "Cannot join $sy default conference.\n$ft_errmsg\n";
	$sysh= $oldsy;
	print "Returning to $sysh->{name} server\n";
    }
    return 1;
}


sub cmd_next {
    my ($args)= @_;
    my ($h, $i, $c);

    if (@{$sysh->{cflist}} == 0)
    {
    	print "No conference list defined\n";
	return;
    }

    # Attempt to join conferences in .cflist until one works
    $i= $sysh->{cfindex};
    while ($c= $sysh->{cflist}[++$i] and !($h= join_conf($sysh, $c, $confh, 1)))
    {
	print $ft_errmsg,"\n";
    }

    if ($c)
    {
	$confh= $h;
	$sysh->{cfindex}= $i;
    }
    else
    {
    	print "No more conferences left\n";
	$sysh->{cfindex}= -1;
    }
    return 1;
}

sub cmd_rewind {
    $sysh->{cfindex}= -1;
    return 1;
}


1;
