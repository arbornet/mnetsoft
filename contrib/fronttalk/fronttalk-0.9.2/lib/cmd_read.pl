# Copyright 2001, Jan D. Wolter, All Rights Reserved.


# do_rfp_prompt(itemh,rdflag)
#   Prompt for, read, and execute an RFP command.  Return codes tell what to
#   do next:
#     0=  drop back to OK prompt
#     1=  Do another rfp prompt on same item
#     2=  goto next item if there is one, OK prompt otherwise
#   We die() on interupt, so this should be called within an eval() to trap
#   that exception.

my $rfpline= undef;

sub do_rfp_prompt {
    my ($itemh, $rdflag)= @_;
    my ($cmd, $type, $word, $rc);

    if (!defined $rfpline)
    {
	my $prompt= $defhash{$itemh->{frozen} ? 'obvprompt' : 'rfpprompt'};

	return 0 if !defined($rfpline= iget($prompt));	# drop to OK on EOF
    }
    ($cmd,$rfpline)= split_cmds($rfpline);

    if ($cmd =~ /^!(.*)$/)
    {
	return cmd_unix($1);
    }
    else
    {
	($type, $word)= parse_cmd(\$cmd, 1);
	return 2 if !defined $type;	# goto next item on newline

	return do_cmd($word, $cmd, 0, $itemh, $rdflag);
    }
}


# read_item_cb($itemh, $rdflag)
#  This is called by ft_nextitem() for each item as soon as the item header is
#  read.  It used to print the item header, but doesn't anymore as we want to
#  be able to pass in a handle for response 0 if we happen to be displaying
#  response zero.

sub read_item_cb {
    my ($itemh, $rdflag)= @_;

    $curr_itemh= $itemh;
}


# read_resp_cb($resph, $rdflag)
#  This is called by ft_nextitem() for each response as it is read.  It prints
#  the response.

sub read_resp_cb {
    my ($resph, $rdflag)= @_;
    my $itemh= $resph->{itemh};
    my $i= $resph->{number};
    my ($n, $line);

    # Print item header
    if ($rdflag->{showhead})
    {
	print exec_sep($defhash{isep}, 1, $itemh, ($i==0) ? $resph : undef,
	    $rdflag),"\n\n";
	$rdflag->{showhead}= 0;
    }

    # Print number of responses
    if ($rdflag->{showcount} and $i > 0)
    {
	print exec_sep($defhash{nsep},1,$itemh,undef, $rdflag);
	$rdflag->{showcount}= 0;
    }

    # Print response header
    print exec_sep($defhash{rsep},1,$itemh,$resph, $rdflag)
	if $i > 0;

    if (!$resph->{hidden} or !flagval('hide'))
    {
	# Print response text
	$n= 1;
	foreach $line (split /\n/, $resph->{text})
	{
	    print exec_sep($defhash{txtsep},1,$itemh,$resph, $rdflag,
		$n++, $line);
	}
    }
    $itemh->{curr}= $i;
    $itemh->{max}= $i if $i > $itemh->{max};
}


# show_item($iselref,$qry,$rdflag,$header,$first)
#  Given an item selector, a query string and a read-flag set, fetch and
#  display the first selected item.  If $header is true, title line is
#  included.  If $first is true, this is the first item we are reading in this
#  "read" command.  Pager is set up, interupts and errors are handled.  Points
#  the global variable $curr_itemh at the item if it was found, or to undef
#  if not.  The item selector is updated to delete the shown item.

sub show_item {
    my ($iselref, $qry, $rdflag, $header, $first)= @_;

    # show the item header and the count of new responses if header flag true
    $rdflag->{showhead}= $rdflag->{showcount}= $header;

    # If the isep asks for length or lines, add the 'measure' flag
    $qry.= '&measure=1'
       if $header and $defhash{isep} =~ /sf_rl(ine|en)/;

    pager_on();
    $intr= 0;

    $curr_itemh= undef;
    eval { ft_nextitem($confh, $$iselref, $qry, $first,
    	\&read_item_cb, \&read_resp_cb, $rdflag) };
    $$iselref= $curr_itemh ? $curr_itemh->{isel} : undef;

    print exec_sep($defhash{zsep},1,$curr_itemh,undef, $rdflag)
	if !$@ and $curr_itemh;

    pager_off() if !$rdflag->{pass};

    # caught interupt or broken pipe
    print "\n$@" if ($@);

    print "Error retrieving item: $ft_errmsg\n" if ($ft_err);
}

sub cmd_print { cmd_read(@_, 1); }

sub cmd_read {
    my ($args, $print)= @_;
    my ($line, $n, $rn, $in, $rdflag, $itemh, %def);

    inconf() or return 1;

    # default to 'read new'
    $args= 'new' if $args =~ /^\s*$/;

    if ($print)
    {
	$def{ff}= 1;
	$def{pass}= 1;
    }

    # parse READ arguments
    my ($isel, $qry, $rdflag)= parse_range($args, $confh, 0, %def);

    # default to "read new"
    if (!$isel and $qry !~ /rsel=/)
    {
    	$rdflag->{new}= 1;
	$qry.= '&rsel=new';
	$isel= '1-$';
    }

    # Set 'noskip' if there is only one item, or a comma-separated list of
    # items in the range.  If there are any dashes, then we skip.  This
    # differs from Picospan, which will skip only within dashed ranges.
    $rdflag->{noskip}= 1
	if !defined $rdflag->{noskip} and $rdflag->{new} and $isel !~ /-/;

    # We set 'noskip' parameter here instead of in range.pl because we only
    # want it to work for the read command.
    $qry.= '&noskip=1' if $rdflag->{noskip} > 0;
    
    !$print or
	print exec_sep($defhash{printmsg}, 0, $confh);

    if ($debug)
    {
	print "isel: '$isel' query: '$query'\n";
	dump_rdflag($rdflag);
    }

    # Fetch an item
    $in= 0;
    while ($isel)
    {
	# fetch and show the response
	show_item(\$isel, $qry, $rdflag, 1, $in == 0);

	# drop out if there was no next item
	last if (!$curr_itemh and !$@);
	$confh->{curr}= $curr_itemh->{number};
	$in++;

	# now do rfp prompt
	next if $rdflag->{pass};

	$itemh= $curr_itemh;

	for ($rn= 0; 1; $rn++)
	{
	    if ($rdflag->{force} and $rn == 0)
	    {
	    	$rc= eval { rfp_resp('', $itemh) };
	    }
	    else
	    {
		$rc= eval { do_rfp_prompt($itemh, $rdflag) };
	    }
	    if ($@)
	    {
	    	print "\n$@";
		pager_off();	# if it got left on
	    }
	    else
	    {
		last if $rc == 2;	# goto next item
		return 1 if $rc == 0;	# goto OK prompt
	    }
	}
    }
    print "No items found in range\n" if !$in;
    pager_off() if $rdflag->{pass};
    print "Error retrieving item: $ft_errmsg\n" if ($ft_err);
    return 1;
}


# review($item, $first, $last, $rdflag)
#   redisplay some arbitrary range of responses from some item.  The last
#   can be given as '$' to indicate the end of the item.

sub review {
    my ($item, $first, $last, $rdflag)= @_;
    show_item(\$item, "&rsel=$first-$last", $rdflag, 0, 0);
    return $curr_itemh->{curr};
}

# Commands like -3 and +4 and .
sub rfp_relative {
    my ($offset, $itemh, $rdflag)= @_;
    $itemh->{curr}= review($itemh->{number},
	$itemh->{curr}+$offset, '$', $rdflag);
    $itemh->{maxresp}= $curr_itemh->{max}
        if $curr_itemh->{max} > $itemh->{maxresp};
    return 1;
}

# Commands like 17 and 12-14
sub rfp_range {
    my ($first, $last, $itemh, $rdflag)= @_;
    $itemh->{curr}= review($itemh->{number}, $first, $last, $rdflag);
    $itemh->{maxresp}= $curr_itemh->{max}
        if $curr_itemh->{max} > $itemh->{maxresp};
    return 1;
}

sub rfp_last {
    my ($args, $itemh, $rdflag)= @_;
    $itemh->{curr}= review($itemh->{number}, '$', '$', $rdflag);
    $itemh->{maxresp}= $curr_itemh->{max}
        if $curr_itemh->{max} > $itemh->{maxresp};
    return 1;
}

sub rfp_pass {
    return 2;
}


sub rfp_only {
    my ($args, $itemh, $rdflag)= @_;
    my ($type, $resp)= parse_cmd(\$args, 0);
    if ($resp !~ /^\d+$/)
    {
    	print "You must specify a response number\n";
	return 1;
    }
    # temporarily turn hide flag off for this
    my $old= $settings[1]->{hide};
    $settings[1]->{hide}= 0;
    $itemh->{curr}= review($itemh->{number}, $resp, $resp, $rdflag);
    $itemh->{maxresp}= $curr_itemh->{max}
        if $curr_itemh->{max} > $itemh->{maxresp};
    $settings[1]->{hide}= $old;
    return 1;
}

sub rfp_again {
    my ($args, $itemh, $rdflag)= @_;
    my $isel= $itemh->{number};
    show_item(\$isel, '&rsel='.$itemh->{firstresp}.'-$', $rdflag, 1, 0);
    $itemh->{maxresp}= $curr_itemh->{max}
        if $curr_itemh->{max} > $itemh->{maxresp};
    return 1;
}

sub rfp_head {
    my ($args, $itemh, $rdflag)= @_;
    print exec_sep($defhash{isep},1,$itemh,undef, $rdflag);
    return 1;
}

sub renew {
    my ($args, $itemh, $rdflag)= @_;
    my ($type, $resp)= parse_cmd(\$args, 0);
    if ($resp !~ /^\d+$/)
    {
	# probably wrong
	$resp= $itemh->{firstresp};
    }
    require "if_post.pl";
    print ft_respop('U', $itemh->{confh}, $itemh->{number}, $resp);
}

sub rfp_new {
    renew(@_);
    return 0;
}

sub rfp_hold {
    renew(@_);
    return 2;
}


1;
