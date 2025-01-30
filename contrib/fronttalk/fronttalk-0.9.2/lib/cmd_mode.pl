# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_post.pl";


# Item Mode Commands - forget, remember, freeze, thaw, retire, unretire, kill,
#   mark seen, favor, disfavor, tempfavor.

sub itemop {
    my ($op, $verb, $dflt_confirm, $args, $itemh)= @_;
    my $rc= ($itemh and !flagval('modestay')) ? 2 : 1;
    my $msg;

    inconf() or return 1;

    my ($isel, $qry, $rdflag)= parse_range($args, $confh);

    # default to current item
    if ($isel =~ /^\s*$/)
    {
	if ($itemh)
	{
	    # at RFP prompt
	    $isel= $itemh->{number};
	    $rc= 2 if $op eq 'K';
	}
	else
	{
	    # at Ok prompt
	    if ($op ne 'S')
	    {
		print "No item range given.\n";
		return 1;
	    }
	    $isel= '1-$';
	}
    }

    if ($rdflag->{confirm} or (!defined($rdflag->{confirm}) and $dflt_confirm))
    {
        # If 'confirm' option was set, do a browse, asking about each item

    	require "if_scan.pl";
	my ($it,$ans);
	$qry.= '&showforgotten=1' if $qry !~ /&showforgotten=/;
	$qry.= '&measure=1' if $defhash{isep} =~ /sf_rl(ine|en)/;
	my @ih= ft_browse($confh, $isel, $qry);
	print "No Items Matched.\n" if @ih == 0;

	foreach $it (@ih)
	{
	    # Display header and ask if it should go
	    print exec_sep($defhash{isep}, 1, $it, undef, $rdflag);
	    print "\n$verb item $it->{number} [YNQ]? ";
	    $ans= lc(<STDIN>);
	    chop $ans;

	    if ($ans eq 'y')
	    {
		$msg= ft_itemop($op, $confh, $it->{number});
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
		    print "Item vanished unexpectedly.\n";
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

	$msg= ft_itemop($op, $confh, $isel, $qry);
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
    return $rc;
}

sub cmd_forget {
    return itemop('F', 'Forget', 0, @_);
}

sub cmd_remember {
    return itemop('M', 'Remember', 0, @_);
}

sub cmd_freeze {
    return itemop('Z', 'Freeze', 0, @_);
}

sub cmd_thaw {
    return itemop('T', 'Thaw', 0, @_);
}

sub cmd_retire {
    return itemop('R', 'Retire', 0, @_);
}

sub cmd_unretire {
    return itemop('U', 'Unretire', 0, @_);
}

sub cmd_kill {
    return itemop('K', 'Kill', 1, @_);
}

sub cmd_fixseen {
    return itemop('S', 'Mark as seen', 0, @_);
}

sub cmd_favor {
    if ($sysh->{versnum} < 3001)
    {
    	print "This server does not support favorites.\n";
	return 1;
    }
    return itemop('V', 'Favor', 0, @_);
}

sub cmd_disfavor {
    if ($sysh->{versnum} < 3001)
    {
    	print "This server does not support favorites.\n";
	return 1;
    }
    return itemop('D', 'Disfavor', 0, @_);
}

# Response Mode Commands - show, hide, erase

sub respop {
    my ($op, $args, $itemh)= @_;
    my ($type, $resp)= parse_cmd(\$args, 0);
    if ($resp !~ /^\d+$/)
    {
    	print "You must specify a response number\n";
	return 1;
    }
    $msg= ft_respop($op, $itemh->{confh}, $itemh->{number}, $resp);
    if ($ft_err)
    {
	print "$ft_errmsg\n";
    }
    elsif ($msg)
    {
	print $msg;
    }
}

sub rfp_hide {
    respop('H', @_);
    return 1;
}

sub rfp_show {
    respop('S', @_);
    return 1;
}

sub rfp_erase {
    respop('E', @_);
    return 1;
}


sub do_retitle {
    my ($private, $args, $itemh)= @_;
    my ($type, $val, $new, $item, $err);
    my $re= $private ? 'my' : 're';

    inconf() or return 1;

    while (1)
    {
	($type, $val)= parse_cmd(\$args, 0);
	$type or last;

	if ($type eq 'Q')
	{
	    $new.= $val;
	}
	elsif ($type eq 'S')
	{
	    if (defined $item or $val =~ /-,/)
	    {
	    	print "Cannot retitle multiple items\n";
		return 1;
	    }
	    $item= $val;
	}
	else
	{
	    print "Weird parameter $val on ${re}title command\n";
	    return 1;
	}
    }

    # default to current item
    if (!defined $item)
    {
	if (!$itemh)
	{
	    print "No item range given.\n";
	    return 1;
	}
	$item= $itemh->{number};
    }

    if (!$new)
    {
    	print "New ",$private?'Private ':'',"Title for Item $item: ";
	$new= <STDIN>;
	chomp $new;
	if (!$private and $new =~ /^\s*$/)
	{
	    print "Canceling retitle.\n";
	    return 1;
	}
    }

    $err= ft_retitle($confh, $item, $private, $new);
    if ($ft_err)
    {
    	print "$ft_errmsg\n";
    }
    if ($err)
    {
    	print "$err\n";
    }
    else
    {
	print $private ? "Private title set.\n" : "Retitle succeeded.\n";
	$itemh->{title}= $new;
    }
    return 1;
}

sub cmd_retitle { return do_retitle(0, @_); }
sub cmd_mytitle { return do_retitle(1, @_); }

1;
