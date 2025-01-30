# Copyright 2005, Jan D. Wolter, All Rights Reserved.  #


#  ft_date($date)
#  Do a date command.  Just pass in a date string and return the unix time
#  integer that backtalk interprets it as.

sub ft_date {
    my ($str)= @_;
    my ($page, $tok);

    $page= ft_runscript($sysh, "fronttalk/date", "dstr=".ft_httpquote($str), 0);
    return undef if $ft_err;

    print "DATE:\n$page\n" if $debug;

    while ($tok= ft_parse(\$page))
    {
	return $tok->{TEXT}+0  if $tok->{TOKEN} eq 'text';
    }
    return -1;
}

1;
