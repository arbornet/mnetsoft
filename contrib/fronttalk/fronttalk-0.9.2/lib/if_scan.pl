# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #


# ft_browse($confh, \$isel, $qry, $cb, $cb_extra...)
#  Get handles for many items in the given conference.  The query string is
#  an HTTP format string setting the variables 'rsel', 'showforgotten' and
#  'reverse' and 'sincedate'.  The query string starts with an & if it is not
#  empty.    For each item called, we call the callback function with the
#  item handle.  The item handle has fields including:
#
#      confh       - conference handle
#      number	   - Item number
#      title	   - Item title
#      mytitle	   - Private title, if any
#      linkdate	   - Date entered or linked
#      lastdate    - Date of most recent response
#      maxresp     - Maximum response number
#      firstresp   - Number of first returned response
#      favorite    - True if a favorite
#      frozen      - True if frozen
#      retired     - True if retired
#      forgotten   - True if forgotten
#      authorid    - Login of author of resp 0
#      authoruid   - Uid number of author of resp 0
#      authorname  - Fullname of author of resp 0
#      size        - bytes in resp 0 (if measure requested in query)
#      lines       - number of lines in resp 0 (if measure requested in query)
#
#  Returns the count of items found.
 
sub ft_browse {
    my ($confh, $isel, $qry, $cb, @extra)= @_;
    my ($itemh, $i, $page, @ih);

    $itemh= {confh=> $confh, brcnt=> 0};

    ft_runscript($sysh, "fronttalk/browse",
    	"conf=$confh->{alias}&isel=".ft_httpquote($isel).$qry, 1,
	\&ft_browse_cb, $itemh, \@ih, $cb, @extra);

    return $cb ? $itemh->{brcnt} : @ih;
}

sub ft_browse_cb {
    my ($tok, $itemh, $ihr, $cb, @extra)= @_;

    if ($tok->{TOKEN} eq 'ITEM')
    {
	if ($cb)
	{
	    $itemh->{brcnt}++;
	}
	else
	{
	    push @{$ihr}, {};
	    $itemh= $ihr->[$#$ihr];
	}
	$itemh->{number}= $tok->{N};
	$itemh->{title}= ft_unquote($tok->{TITLE});
	$itemh->{mytitle}= ft_unquote($tok->{MYTITLE});
	$itemh->{linkdate}= $tok->{LINKDATE};
	$itemh->{lastdate}= $tok->{LASTDATE};
	$itemh->{readdate}= $tok->{READDATE};
	$itemh->{maxresp}= $tok->{MAXRESP};
	$itemh->{firstresp}= $tok->{MAXRESP} - $tok->{NEWRESP};
	$itemh->{favorite}= $tok->{FAVORITE};
	$itemh->{frozen}= $tok->{FROZEN};
	$itemh->{forgotten}= $tok->{FORGOTTEN};
	$itemh->{retired}= $tok->{RETIRED};
	$itemh->{linked}= $tok->{LINKED};
	($itemh->{authorid}, $itemh->{authoruid}, $itemh->{authorname})=
	    split /,/, ft_unquote($tok->{AUTHOR}), 3;
	$itemh->{size}= $tok->{SIZE};
	$itemh->{lines}= $tok->{LINES};

        &{$cb}($itemh, @extra) if $cb;
    }
    elsif ($tok->{TOKEN} eq 'ERROR')
    {
	($ft_err,$ft_errmsg)= ('BTE1',"Request failed: $tok->{MSG}");
    }
    return 0;
}


# ft_conflist()
#  Get an array of handles for the lines of the conference menu.
#  Each array element has the following hash fields:
#
#      type	   - 'C' = conference, 'H' = subheading
#      name	   - Name to join conference by, text of heading
#      title	   - Longer name of conference
#      des	   - Description of conference or category
 
sub ft_conflist {
    my (@cla, $i, $page, $in, $type, $name, $title, $des);

    $page= ft_runscript($sysh, "/fronttalk/conflist", '', 1)
    	or return undef;

    print "CONFLIST:\n$page\n" if $debug;

    # Parse info
    $i= 0;
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'text' and $in)
	{
	    $h->{des}= $tok->{TEXT};
	}
	elsif ($tok->{TOKEN} eq 'CF' or $tok->{TOKEN} eq 'CAT')
	{
	    $in= $cla[$i]->{type}= (($tok->{TOKEN} eq 'CF') ? 'C' : 'H');
	    $h= $cla[$i];
	    $i++;

	    $h->{name}= ft_unquote($tok->{NAME});
	    $h->{title}= ft_unquote($tok->{TITLE});
	}
	elsif ($tok->{TOKEN} eq '/CF' or $tok->{TOKEN} eq '/CAT')
	{
	    $in= undef;
	}
    }

    return @cla;
}


# ft_check($conflist)
#  Get an array of handles for the hot list conferences, or the given
#  conferences if $conflist is not empty.  Each array element has the
#  following hash fields:
#
#      name	   - Name of conference
#      err	   - 'Not accessible', etc
#      total	   - number of items
#      newr 	   - number of new response items
#      newi 	   - number of brand new items
#      unseen 	   - number of unseen items
 
sub ft_check {
    my ($conflist)= @_;
    my (@cha, $i, $page, $h);

    $conflist= "csel=".ft_httpquote($conflist) if $conflist;

    $page= ft_runscript($sysh, "/fronttalk/check", $conflist, 1)
    	or return undef;

    print "CHECK:\n$page\n" if $debug;

    # Parse info
    $i= 0;
    while ($tok= ft_parse(\$page))
    {
	if ($tok->{TOKEN} eq 'CHECK')
	{
	    $cha[$i]->{name}= ft_unquote($tok->{NAME});
	    $h= $cha[$i];
	    $i++;

	    $h->{err}= $tok->{ERR};
	    $h->{total}= $tok->{TOTAL};
	    $h->{newr}= $tok->{NEWR};
	    $h->{newi}= $tok->{NEWI};
	    $h->{unseen}= $tok->{UNSEEN};
	}
    }

    return @cha;
}


1;
