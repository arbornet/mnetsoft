# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# Range Keywords:  If any of the following keywords appear in a range, then
# we append the query string to the current query, and set the given flag
# to the given value.  The query string will be handled by Backtalk, and the
# flags by Fronttalk.  Mostly.

@rngtab= (
#    word             min max   query string		flag	value
    ['new',		1, 3,	'&rsel=new',		'new',	 1 ],
    ['brandnew',	2, 8,	'&rsel=brandnew'],
    ['unseen',		2, 6,	'&rsel=unseen'],
    ['newresponse',	4, 11,	'&rsel=newresp'],
    ['favorites',	2, 9,	'&favonly=1'],
    ['reverse',		1, 7,	'&reverse=1'],
    ['favfirst',	4, 8,	'&nofavfirst=0'],
    ['nofavfirst',	6, 10,	'&nofavfirst=1'],
    ['bnewfirst',	5, 9,	'&bnewfirst=1'],
    ['bnewlast',	5, 8,	'&bnewlast=1'],
    ['noforget',	3, 8,	'&showforgotten=1',	'sf',	   1 ],
    ['noforgotten',	3, 11,	'&showforgotten=1',	'sf',	   1 ],
    ['forget',		3, 6,	'&showforgotten=2',	'sf',	   2 ],
    ['forgotten',	3, 9,	'&showforgotten=2',	'sf',	   2 ],
    ['noresponse',	3, 10,	'&noresp=1',		'noresp',  1 ],
    ['forceresponse',	3, 13,	undef,			'force',   1 ],
    ['confirm',		1, 7,	undef,			'confirm', 1 ],
    ['noconfirm',	3, 9,	undef,			'confirm', 0 ],
    ['pass',		1, 4,	undef,			'pass',	   1 ],
    ['nopass',		3, 6,	undef,			'pass',	   0 ],
    ['date',		1, 4,	undef,			'date',	   1 ],
    ['nodate',		3, 6,	undef,			'date',	   0 ],
    ['uid',		1, 3,	undef,			'uid',	   1 ],
    ['nouid',		3, 5,	undef,			'uid',	   0 ],
    ['numbered',	3, 8,	undef,			'num',	   1 ],
    ['nonumbered',	6, 10,	undef,			'num',	   0 ],
    ['unnumbered',	6, 10,	undef,			'num',	   0 ],
    ['ff',		2, 2,	undef,			'ff',	   1 ],
    ['formfeed',	4, 8,	undef,			'ff',	   1 ],
    ['short',		2, 5,	undef,			'short',   1 ],
    ['long',		2, 4,	undef,			'long',    1 ],
    ['skip',		2, 4,	undef,			'noskip', -1 ],
    ['noskip',		4, 6,	undef,			'noskip',  1 ],
    );


# ($isel, $qrystring, $hashref)= parse_range($argstring, $confh, $find, %defts)
#   Given an argument string passed to READ or BROWSE or whatever, parse it
#   and return an item selectr, a string of other HTTP query arguments that
#   effect the fronttalk server's behavior and a reference to a hash of flags
#   that will effect the fronttalk client's behavior.  The latter includes
#   the following fields:
#      $h->{resp}:    0=normal, 1=none, 2=force
#      $h->{pass}:    0 or 1
#      $h->{date}:    0 or 1
#      $h->{uid}:     0 or 1
#      $h->{num}:     0 or 1
#      $h->{ff}:      0 or 1
#      $h->{short}:   0 or 1
#   Calling procedure should do defaults.

sub parse_range {
    my ($args,$confh, $find, %flag)= @_;
    my ($sel, $qry);
    my ($word, $type, $len, $row);

    # defaults established by 'set' command
    defined $flag{uid} or $flag{uid}= flagval('uid');
    defined $flag{date} or $flag{date}= flagval('date');
    defined $flag{num} or $flag{num}= flagval('numbered');
    my $nofavfirst= flagval('nofavfirst');

    KEYWORD: while (1)
    {
	($type, $word)= parse_cmd(\$args, 1);
	last if !defined($type);

	# expand any macro words
	expand_cmd_macro(\$type, \$word, \$args, 4) if $type ne 'S';

	# handle selectors
	if ($word =~ /^[\d,^*.<>\$\-]+$/)
	{
	    $sel eq '' or $sel =~ /-$/ or
	    	$sel.= ',';
	    $sel.= $word;
	    next KEYWORD;
	}

	# quoted strings
	if ($type eq 'Q')
	{
	    $qry.= '&pattern='.ft_httpquote($word);
	    next KEYWORD;
	}

	# process flags
	$len= length($word);
	foreach $row (@rngtab)
	{
	    if ($len >= $row->[1] and $len <= $row->[2] and
		substr($row->[0],0,$len) eq $word)
	    {
                if ($row->[0] eq "favfirst" || $row->[0] eq "nofavfirst") {
                    $nofavfirst = 0;
		}
		# Item selector flags to be passed to backtalk
		$qry.= $row->[3] if $row->[3];
		# Action selector flags to be processed by Fronttalk
		$flag{$row->[4]}= $row->[5] if $row->[5];
		next KEYWORD;
	    }
	}


	# 'since'
	if ($word eq 'sin' or $word eq 'sinc' or $word eq 'since')
	{
	    my $date;
	    require "date.pl";
	    ($type, $date)= parse_date(\$args);
	    next if !defined $date;
	    expand_cmd_macro(\$type, \$date, \$args, 4) if $type eq 'W';
	    $qry.= '&sincedate='.ft_httpquote($date);
	    next KEYWORD;
	}

	# login names
	if ($word =~ /^\(([a-z0-9,]*)\)$/)
	{
	    $qry.= '&by='.$1;
	    next KEYWORD;
	}

	# Error
	print "Don't understand \"$word\".\n";
    }
    if ($nofavfirst) {
        $qry .= '&nofavfirst=1';
    }

    # fixups on selector
    $sel= '1-$' if $sel eq '' and ((!$find and $qry =~ /(rsel|pattern|by)=/) or
				   ($find and $qry =~ /(rsel)=/));
    $sel=~ s/\*/^-\$/g;
    $sel=~ s/\^/$confh->{first}/g;
    $sel=~ s/\./$confh->{curr}/g;

    return ($sel, $qry, \%flag);
}

sub dump_rdflag {
    my ($rdflag)= @_;
    my ($k);
    print "RDFLAG $rdflag\n";
    foreach $k (sort keys %$rdflag)
    {
    	print " $k=$rdflag->{$k}\n";
    }
}

1;
