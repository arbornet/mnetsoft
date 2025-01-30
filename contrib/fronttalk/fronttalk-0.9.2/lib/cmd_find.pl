# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_find.pl";


# find_resp_cb($itemh, $rdflag)
#  This is called by ft_find() for each match to a response found.

sub find_resp_cb {
    my ($itemh, $resph, $rdflag)= @_;

    print exec_sep($defhash{fsep}, 1, $itemh, $resph, $rdflag,
    		   $resph->{lnum}, $resph->{ltext});
}

# find_title_cb($itemh, $rdflag)
#  This is called by ft_find() for each match to a title found.

sub find_title_cb {
    my ($itemh, $rdflag)= @_;

    print exec_sep($defhash{ftsep}, 1, $itemh, undef, $rdflag);
}


sub cmd_find {
    my ($args, $ih)= @_;
    my ($n, $rdflag, $itemh);

    inconf() or return 1;

    # parse FIND arguments
    my ($isel, $qry, $rdflag)= parse_range($args, $confh, 1);

    # default to 'current' at RFP prompt or 'all' at OK prompt
    $isel= $ih ? $ih->{number} : '1-$' if $isel =~ /^\s*$/;
    
    # Do it
    $intr= 0;
    pager_on();
    $n= ft_find($confh, $isel, $qry, \&find_resp_cb, \&find_title_cb, $rdflag);
    print "$n matches.\n" if !$ft_err;
    pager_off();
    print "Error searching conference: $ft_errmsg\n" if $ft_err;
    return 1;
}


1;
