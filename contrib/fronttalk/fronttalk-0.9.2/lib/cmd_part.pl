# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_part.pl";


# part_cb($itemh, $rdflag)
#  This is called by ft_part() for each participant found.

sub part_cb {
    my ($parth)= @_;

    if ($parth->{error})
    {
    	print "User $parth->{myid} $parth->{error}.\n";
    }
    else
    {
	print exec_sep($defhash{partmsg}, 0, $parth, 2);
    }
}


sub cmd_part {
    my ($args)= @_;
    my ($n, $t, $w, $users);

    while (1)
    {
	($t, $w)= parse_cmd(\$args);
	last if !$t;
	$users.= ',' if $users;
	$users.= $w;
    }

    # Do it
    $intr= 0;
    pager_on();
    print exec_sep($defhash{partmsg}, 0, $confh, 1);
    $n= ft_part($confh, $users, \&part_cb);
    print exec_sep($defhash{partmsg}, 0, $confh, 4, $n) if !$ft_err;
    pager_off();
    print "Error retrieving participants: $ft_errmsg\n" if $ft_err;
    return 1;
}


1;
