# Copyright 2001, Jan D. Wolter, All Rights Reserved.

sub pager_init {
    # initialize some stuff for paging
    open SAVEOUT, ">&STDOUT";
    $in_pager= 0;
}

sub pager_on {

    return if $in_pager;

    return if !$defhash{pager};
    $sigpipe_cnt= 0;

    # attach pager to standard out
    if (!open STDOUT, "|$defhash{pager}")
    {
    	print "Cannot run pager $defhash{pager}\n";
	return;
    }
    # turn off buffering
    $|= 1;
    $in_pager= 1;
}

# restore standard out to normal

sub pager_off {
    return if !$in_pager;

    close STDOUT;
    open STDOUT, ">&SAVEOUT";

    $in_pager= 0;
}


1;
