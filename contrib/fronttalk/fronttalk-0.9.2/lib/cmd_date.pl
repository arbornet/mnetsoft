# Copyright 2005, Jan D. Wolter, All Rights Reserved.

require "if_date.pl";


# Date and CDate Commands.  These just pass a date string to backtalk to
# convert into an Unix time integer, and returns the result.  It's really
# just for testing, and Picospan compatibility.

sub cmd_date {
    my ($arg)= @_;

    if ($sysh->{versnum} < 3007)
    {
	print "This server does not support the date command.\n";
	return 1;
    }

    require "date.pl";
    my ($type,$str)= parse_date(\$arg);
    my ($date);
    if (!defined $str or ($date= ft_date($str)) <= 0)
    {
    	print "Bad Date\n";
    }
    else
    {
	print scalar localtime($date), "\n";
    }
    return 1;
}

sub cmd_cdate {
    my ($arg)= @_;

    if ($sysh->{versnum} < 3007)
    {
	print "This server does not support the cdate command.\n";
	return 1;
    }

    require "date.pl";
    my ($type,$str)= parse_date(\$arg);
    my ($date);
    if (!defined $str or ($date= ft_date($str)) <= 0)
    {
    	print "Bad Date\n";
    }
    else
    {
	printf "%lX\n", $date;
    }
    return 1;
}

1;
