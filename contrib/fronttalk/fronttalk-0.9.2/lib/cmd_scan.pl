# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_scan.pl";

# browse_cb($itemh, $rdflag)
#  Called by ft_browse for each item found.

sub browse_cb {
    my ($itemh, $rdflag)= @_;

    print exec_sep($defhash{$rdflag->{short}?'ishort':'isep'},
		   1, $itemh, undef, $rdflag);
}

sub cmd_browse {
    my ($args)= @_;
    my (@ih, $itemh, $short, $n);

    inconf() or return 1;

    my ($isel, $qry, $rdflag)= parse_range($args, $confh);
    $short= $BROWSE_SHORT;
    $short= 1 if $rdflag->{short};
    $short= 0 if $rdflag->{long};
    $rdflag->{short}= $short;

    # default to 'browse all'
    $isel= '1-$' if $isel =~ /^\s*$/;

    # If the isep asks for length or lines, add the 'measure' flag
    $qry.= '&measure=1'
    	if $defhash{$rdflag->{short}?'ishort':'isep'} =~ /sf_rl(ine|en)/;

    $intr= 0;
    pager_on();
    $n= ft_browse($confh, $isel, $qry, \&browse_cb, $rdflag);
    pager_off();

    if ($ft_err)
    {
    	print "Error: $ft_errmsg\n";
    }
    elsif ($n == 0)
    {
    	print "No items matched.\n";
    }

    return 1;
}


sub cmd_check {
    my ($args)= @_;
    my ($l,$t,$w,$cflist);

    while (1)
    {
    	($t,$w)= parse_cmd(\$args,1);
	last if !$t;
	$cflist.= ',' if $cflist;
	$cflist.= $w;
    }

    my @ch= ft_check($cflist);
    if ($ft_err)
    {
	print "Error: $ft_errmsg\n";
	return 1;
    }

    print "new resp items  conference\n\n";

    foreach $l (@ch)
    {
    	print (($l->{name} eq $confh->{alias}) ? '-->' : '   ');
	printf " %3d %3d $l->{name}\n", $l->{newr}, $l->{newi};
    }
    return 1;
}


1;
