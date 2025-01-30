# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_post.pl";

sub cmd_link {
    my ($args, $itemh)= @_;
    my $msg;

    inconf() or return 1;

    # Get source conference name
    my ($type, $src)= parse_cmd(\$args);
    if (!$type or $type ne 'K')
    {
        print "No conference specified!\n";
	return 1;
    }

    # Get source item selector
    my ($isel, $qry, $rdflag)= parse_range($args, $confh);
    if ($isel =~ /^\s*$/)
    {
	print "No item range given!\n";
	return 1;
    }

    if ($rdflag->{confirm} or (!defined($rdflag->{confirm}) and $dflt_confirm))
    {
        # If 'confirm' option was set, do a browse, asking about each item
	my $srch= ft_openconf($sysh, $src);
	if ($ft_err)
	{
	    print "Cannot access conference $src: $ft_errmsg\n";
	    return 1;
	}

    	require "if_scan.pl";
	my ($it,$ans);
	$qry.= '&measure=1' if $defhash{isep} =~ /sf_rl(ine|en)/;
	my @ih= ft_browse($srch, $isel, $qry);
	print "No Items Matched.\n" if @ih == 0;

	foreach $it (@ih)
	{
	    # Display header and ask if it should go
	    print exec_sep($defhash{isep}, 1, $it, undef, $rdflag);
	    print "\nLink item $it->{number} [YNQ]? ";
	    $ans= lc(<STDIN>);
	    chop $ans;

	    if ($ans eq 'y')
	    {
		$msg= ft_link($src, $confh, $it->{number});
		if ($ft_err)
		{
		    print "$ft_errmsg\n";
		}
		elsif ($msg)
		{
		    print $msg;
		}
		else
		{
		    print "Item $it->{number} vanished unexpectedly.\n";
		}
	    }
	    elsif ($ans eq 'q')
	    {
	    	last;
	    }
	    else
	    {
	    	print "Skipping item $it->{number}.\n";
	    }
	}
    }
    else
    {
	# if 'noconfirm' option was set, just do it in one blast

	$msg= ft_link($src, $confh, $isel, $qry);
	if ($ft_err)
	{
	    print "$ft_errmsg\n";
	}
	elsif ($msg)
	{
	    print $msg;
	}
	else
	{
	    print "No Items Matched.\n";
	}
    }
    return 1;
}


1;
