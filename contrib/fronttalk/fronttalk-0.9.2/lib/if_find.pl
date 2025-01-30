# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #


#  ft_find($confh, $isel, $qry, $resp_cb, $title_cb, $cb_extra...)
#  Do a find.  The item selector is some kind of range like '3-$'.  The query
#  string is an HTTP format string setting variables including 'rsel', 'by',
#  'pattern, 'sincedate', 'showforgotten' and 'reverse'.  The query string
#  starts with an & if it is not empty.  For each match to a response found,
#  the response callback function will be called once.  It is passed an item
#  and response handle, both somewhat impoverished.  If the match was in the
#  title, we call the title callback, with just the item handle.  On error
#  $ft_err will be set when the main program returns.  The program returns
#  the total number of matches found.
#
#  Fields of item handle passed to title and response callback:
#
#      confh       - conference handle
#      number	   - Item number
#      title	   - Item title
#      authorid    - Login of author of resp 0 (title callback only)
# 
#  Fields response handles passed to response callback:
#
#      itemh       - item handle
#      number      - Response number
#      authorid    - Login of author
#      lnum        - Line number
#      ltext       - Line text

sub ft_find {
    my ($confh, $isel, $qry, $r_cb, $t_cb, @extra)= @_;
    my $itemh;

    # initialize and response handles
    $itemh= {confh => $confh, findcnt => 0};

    ft_runscript($sysh, "fronttalk/find",
    	"conf=$confh->{alias}&isel=".ft_httpquote($isel).$qry, 1,
	\&ft_find_cb, $itemh, $r_cb, $t_cb, @extra);

    return $itemh->{findcnt};
}


sub ft_find_cb {
    my ($tok, $itemh, $r_cb, $t_cb, @extra)= @_;

    if ($tok->{TOKEN} eq 'TEXT' or $tok->{TOKEN} eq 'TITLE')
    {
	$itemh->{number}= $tok->{ITEM};
	$itemh->{title}= ft_unquote($tok->{TITLE});
	if ($tok->{TOKEN} eq 'TEXT')
	{
	    my $resph= {
	        itemh => $itemh,
		number => $tok->{RESP},
		authorid => $tok->{AUTHOR},
		lnum => $tok->{LINE},
		ltext => ft_unquote($tok->{TEXT}),
	        };
	    &{$r_cb}($itemh, $resph, @extra);
	}
	else
	{
	    $itemh->{authorid}= $tok->{AUTHOR};
	    &{$t_cb}($itemh, @extra);
	}
    }
    elsif ($tok->{TOKEN} eq 'COUNT')
    {
	$itemh->{findcnt}= $tok->{N};
    }
    elsif ($tok->{TOKEN} eq 'ERROR')
    {
	($ft_err,$ft_errmsg)= ('BTE1',"Request failed: ".
	    ft_unquote($tok->{MSG}));
    }
    return 0;
}


1;
