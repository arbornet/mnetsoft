# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #


# $sysh= ft_connect($sysalias)
#   Search the system-list file or URL for a system with the given alias.
#   If found, fetch the system information page, construct a system information
#   record, and return it.  This is a hashref with the following fields:
#      name     - Printable name of site - "FooTalk".
#      alias    - Name we used to join it.
#      banner   - Welcome text, possibly multiple lines - "Welcome to FooTalk".
#      login    - 'cookie' or 'basic'.
#      pwedit   - Can server change passwords and fullnames and such?
#      direct   - True if we are executing Backtalk directly
#      secure   - True if we are executing Backtalk directly or over https
#      anon     - defined if we allow anonymous reading.
#      amanon   - defined if we are anonymous.
#      location - URL or path to anonymous executable.
#      pwurl    - URL or path to executable requiring login.
#      version  - Version number of server, like "1.3.14".
#      versnum  - Version number of server, like "1003014".
#      bbsrc    - commands from system rc file
#      cflist   - Array of conference names in user's .cflist
#      cfonce   - commands from user's .cfonce file
#      cfrc     - commands from user's .cfrc file
#      username - my login id
#      password - my password
#      session  - true if login==cookie and we have logged in.
#   on failure, set $ft_err if the system does not exist or we do not have
#   access.  If we have a previous connection, this reuses it.

sub ft_connect {
    my ($sysalias)= @_;
    my ($i,$s,$page,$code);
    my ($in);

    $ft_err= undef;

    # If we have it in cache, reuse it
    return $syscache{$sysalias} if ($syscache{$sysalias});

    if ($sysalias)
    {
	# Scan sequentially through system list looking for alias
	$s->{alias}= $sysalias;
	for ($i= 0; $i < @syslist; $i++)
	{
	    if ($syslist[$i][0] eq $sysalias)
	    {
	    	$s->{location}= $syslist[$i][1];
		last;
	    }
	}
	if (!$s->{location})
	{
	    ($ft_err, $ft_errmsg)= ('IFC1', "Unknown system '$sysalias'");
	    return $s;
	}
    }
    else
    {
	# Use default entry from system list
	($s->{alias}, $s->{location})= @{$syslist[$default_sys]};
    }
    $s->{direct}= ($s->{location} !~ /^https?:\/\//);
    $s->{secure}= ($s->{direct} or ($s->{location} =~ /^https:\/\//));

    print "System Alias=$s->{alias} Location=$s->{location}\n" if $debug;

    # Fetch the system's fronttalk information page
    if ($s->{direct})
    {
	$s->{location} =~ s/^file://;
	require "net_exec.pl";
    	$page= ft_getexec($s->{location}, "/public/fronttalk");
    }
    else
    {
	require "net_http.pl";
	$s->{location}.= '/' if $s->{location} !~ /\/$/;
    	($page,$code)= ft_gethttp("$s->{location}/public/fronttalk");
    }
    return $s if $ft_err;
    if (!$page or $page =~ /<TITLE>Backtalk Script Not Found<\/TITLE>/)
    {
	($ft_err, $ft_errmsg)=
	    ('IFC2', "Could not retrieve system info from '$s->{alias}'");
	return $s;
    }
    print "SYSINFO:\n$page\n" if $debug;

    # Parse info out of fronttalk info page
    while ($tok= ft_parse(\$page,defined $in))
    {
    	if ($tok->{TOKEN} eq 'text')
	{
	    $s->{$in}.= $tok->{TEXT} if defined $in;
	}
	elsif ($tok->{TOKEN} eq 'INFO')
	{
	    $s->{name}= ft_unquote($tok->{NAME});
	    $s->{login}= $tok->{AUTH};
	    $s->{anon}= $tok->{ANON};
	    $s->{pwedit}= $tok->{PWEDIT};
	    $s->{pwurl}= ft_unquote($tok->{PWURL});
	    $s->{version}= ft_unquote($tok->{VERSION});
	    my ($a,$b,$c)= split /\./, $s->{version};
	    $s->{versnum}= ($a*1000+$b)*1000+$c;
	}
	elsif ($tok->{TOKEN} =~ /^(BANNER|BBSRC)$/)
	{
	    $in= lc($1);
	}
	elsif (lc($tok->{TOKEN}) eq "/$in")
	{
	    $in= undef;
	}
    }

    # Log in user and get user information
    ft_userinfo($s);

    # save sysinfo for reuse
    $syscache{$s->{alias}}= $s;

    return $s;
}


# ft_userinfo($sysh) - given a system handle, log in the user and fill in the
# user information.

sub ft_userinfo
{
    my ($s)= @_;
    my ($page, $in, $tok, $msg);

    # erase any values left over from a previous user
    $s->{session}= $s->{cflist}= $s->{cfonce}= $s->{cfrc}= $s->{amanon}= undef;

    while (1)
    {
	# Get user info
	$page= ft_runscript($s, "/fronttalk/begin", '', 1);
	return $s if $ft_err;

	print "BEGIN:\n$page\n" if $debug;

	# Parse info out of user info page
	$in= undef;
	$msg= undef;
	while ($tok= ft_parse(\$page))
	{
	    if ($tok->{TOKEN} eq 'text')
	    {
		if ($in eq 'cflist')
		{
		    $s->{cflist}= [split /[\s,]+/, $tok->{TEXT}];
		}
		elsif (defined $in)
		{
		    $s->{$in}= $tok->{TEXT};
		}
	    }
	    elsif ($tok->{TOKEN} =~ /^(CFLIST|CFONCE|CFRC)$/)
	    {
		$in= lc($1);
	    }
	    elsif (lc($tok->{TOKEN}) eq "/$in")
	    {
		$in= undef;
	    }
	    elsif ($tok->{TOKEN} eq 'ERROR')
	    {
	        # login failed, save error message, erase username, try again
		$msg= ft_unquote($tok->{MSG});
		$s->{username}= undef;
	    }
	}
	last if !$msg;
        print "$msg\n";
    }
    $s->{session}= 1;
}


# $confh= ft_openconf($sysh,$confname)
#   Open the named conference on the system with the given handle.  If
#   successful, returns a hashref with the following fields:
#
#    sysh       - reference to system handle
#    alias      - name it was joined under
#    name       - longer mixed-case name of conference
#    login      - login text
#    logout     - logout text
#    first      - number of first item
#    last       - number of last item
#    items      - number of items
#    newr       - number of newresponse items
#    newi       - number of brandnew items
#    unseen     - number of unseen items
#    readonly   - defined 1 if you may not respond to items in this conference.
#    noenter    - defined 1 if you may not create items in this conference.
#    fishbowl   - defined 1 if the conference is a fishbowl.
#    userlist   - defined 1 if the conference has ulist
#    grouplist  - defined 1 if the conference has glist.
#    password   - defined 1 if the conference has a password.
#    amfw       - defined 1 if the user is a fairwitness of this conference
#    myname     - user's name in this conference
#    fwlist     - comma-separated fairwitness list
#    confrc     - commands from conference rc file
#    partfile   - name of participation file
#    lastin     - Unix integer date of last time I was in conf (0=never)
#    logindate  - Unix integer modification date of login file (0=none)
#    logoutdate - Unix integer modification date of logout file (0=none)
#    indexdate  - Unix integer modification date of index file (0=none)
#    welcomedate- Unix integer modification date of welcome file (0=none)
#    bulldate   - Unix integer modification date of bull file (0=none)
#    ulistdate  - Unix integer modification date of ulist file (0=none)
#    glistdate  - Unix integer modification date of glist file (0=none)
#    confrcdate - Unix integer modification date of rc file (0=none)
#    anonpost   - defined if we allow anonymous posting

sub ft_openconf {
    my ($sysh, $confname)= @_;
    my ($c,$page, $in);

    $page= ft_runscript($sysh, "/fronttalk/join", "conf=$confname", 1)
    	or return undef;

    print "JOIN:\n$page\n" if $debug;

    $c->{sysh}= $sysh;
    $c->{alias}= $confname;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
    	if ($tok->{TOKEN} eq 'text')
	{
	    $c->{$in}.= $tok->{TEXT} if $in;
	}
	elsif ($tok->{TOKEN} eq 'CONF')
	{
	    $c->{name}= ft_unquote($tok->{NAME});
	    $c->{readonly}= $tok->{READONLY};
	    $c->{noenter}= $tok->{NOENTER};
	    $c->{fishbowl}= $tok->{FISHBOWL};
	    $c->{userlist}= $tok->{USERLIST};
	    $c->{grouplist}= $tok->{GROUPLIST};
	    $c->{password}= $tok->{PASSWORD};
	    $c->{fwlist}= ft_unquote($tok->{FW});
	    $c->{partfile}= ft_unquote($tok->{PARTFILE});
	    $c->{logindate}= $tok->{LOGINDATE};
	    $c->{logoutdate}= $tok->{LOGOUTDATE};
	    $c->{indexdate}= $tok->{INDEXDATE};
	    $c->{bulldate}= $tok->{BULLDATE};
	    $c->{confrcdate}= $tok->{CONFRCDATE};
	    $c->{ulistdate}= $tok->{ULISTDATE};
	    $c->{glistdate}= $tok->{GLISTDATE};
	    $c->{welcomedate}= $tok->{WELCOMEDATE};
	    $c->{anonpost}= $tok->{ANONPOST};
	}
	elsif ($tok->{TOKEN} eq 'ITEMS')
	{
	    $c->{first}= $tok->{FIRST};
	    $c->{last}= $tok->{LAST};
	    $c->{items}= $tok->{N};
	}
	elsif ($tok->{TOKEN} eq 'USER')
	{
	    $c->{myname}= ft_unquote($tok->{NAME});
	    $c->{newr}= $tok->{NEWRESP};
	    $c->{newi}= $tok->{NEWITEM};
	    $c->{unseen}= $tok->{UNSEEN};
	    $c->{amfw}= $tok->{FAIRWITNESS};
	    $c->{lastin}= $tok->{LASTIN};
	}
	elsif ($tok->{TOKEN} =~ /^(LOGIN|LOGOUT|CONFRC)$/)
	{
	    $in= lc($1);
	}
	elsif (lc($tok->{TOKEN}) eq "/$in")
	{
	    $in= undef;
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    ($ft_err,$ft_errmsg)= ('BTE1',ft_unquote($tok->{MSG}));
	    return undef;
	}
    }

    return $c;
}


#  ft_nextitem($confh, $isel, $qry, $first, $item_callback, $resp_callback,
#              $cb_extra...)
#  Get the next item.  The item selector is some kind of range like '3-$'.
#  The query string is an HTTP format string setting the variables 'rsel',
#  'showforgotten' and 'reverse'.  The query string starts with an & if it
#  is not empty.  First is true if this is the first call in a "read" - so
#  item selector may need reshuffling.  If a next item is found, then the
#  item callback function will be called once after the item header has
#  been read.  It is passed an item handle.  Then the response callback is
#  called for each response, giving the response handle as the first
#  arguments.  any extra arguments given are passed through to the callback
#  function after the handle.  On error $ft_err will be set when the main
#  program returns.  On success we return the item handle.
#
#  The item handle has fields including:
#
#      confh       - conference handle
#      isel        - new isel after reading this item.
#      number	   - Item number
#      title	   - Item title
#      mytitle	   - Private item title, if any
#      linkdate	   - Date entered or linked
#      lastdate    - Date of most recent response
#      maxresp     - Maximum response number
#      firstresp   - Number of first returned response
#      frozen      - True if frozen
#      retired     - True if retired
#      linked      - True if linked
#      forgotten   - True if forgotten
#      favorite    - True if favorite
#      authorid    - Login of author of resp 0
#      authoruid   - Uid number of author of resp 0
#      authorname  - Fullname of author of resp 0
#      resph       - current response handle
#      size        - number of characters in resp 0 (if measure=1 in query)
#      lines       - number of lines in resp 0 (if measure=1 in query)
# 
#  Each response handle has the following fields:
#
#      itemh       - item handle
#      number      - Response number
#      authorid    - Login of author
#      authoruid   - Uid number of author
#      authorname  - Fullname of author
#      date	   - Date entered
#      text        - Text of response
#      hidden      - True if hidden
#      erased      - True if erased

sub ft_nextitem {
    my ($confh, $isel, $qry, $shuffle, $item_cb, $resp_cb, @extra)= @_;
    my $itemh;

    # initialize item handle
    $itemh= {confh => $confh};

    # add shuffle flag
    $qry.= '&shuffle=1' if $shuffle;

    ft_runscript($sysh, "fronttalk/read",
    	"conf=$confh->{alias}&isel=".ft_httpquote($isel).$qry, 1,
	\&ft_nextitem_cb, $itemh, $item_cb, $resp_cb, @extra);

    return defined($itemh->{number}) ? $itemh : undef;
}


sub ft_nextitem_cb {
    my ($tok, $itemh, $item_cb, $resp_cb, @extra)= @_;

    if ($tok->{TOKEN} eq 'text')
    {
	print "FT_NEXTITEM_CB: TEXT\n$tok->(TEXT}\n" if $debug;
	$itemh->{resph}->{text}.= $tok->{TEXT};
	return 1;	# keepwhite
    }
    elsif ($tok->{TOKEN} eq 'QRY')
    {
	$itemh->{isel}= $tok->{ISEL};
	return 0;
    }
    elsif ($tok->{TOKEN} eq 'ITEM')
    {
	$itemh->{number}= $tok->{N};
	$itemh->{title}= ft_unquote($tok->{TITLE});
	$itemh->{mytitle}= ft_unquote($tok->{MYTITLE}) if $tok->{MYTITLE};
	$itemh->{linkdate}= $tok->{LINKDATE};
	$itemh->{lastdate}= $tok->{LASTDATE};
	$itemh->{readdate}= $tok->{READDATE};
	$itemh->{firstresp}= $tok->{FIRSTRESP}+0;
	$itemh->{maxresp}= $tok->{MAXRESP}+0;
	$itemh->{frozen}= $tok->{FROZEN};
	$itemh->{forgotten}= $tok->{FORGOTTEN};
	$itemh->{favorite}= $tok->{FAVORITE};
	$itemh->{retired}= $tok->{RETIRED};
	$itemh->{linked}= $tok->{LINKED};
	($itemh->{authorid}, $itemh->{authoruid}, $itemh->{authorname})=
	    split /,/, ft_unquote($tok->{AUTHOR}), 3;
	$itemh->{size}= $tok->{SIZE};
	$itemh->{lines}= $tok->{LINES};

	# Call the item callback function
    	&{$item_cb}($itemh, @extra);

	return 0;
    }
    elsif ($tok->{TOKEN} eq 'RESP')
    {
	my $resph= {
	    itemh	=> $itemh,
	    number	=> ($tok->{N} + 0),
	    date	=> $tok->{DATE},
	    parent	=> $tok->{PARENT},
	    hidden	=> $tok->{HIDDEN},
	    erased	=> $tok->{ERASED},
	    };
	($resph->{authorid},$resph->{authoruid},$resph->{authorname})=
	    split /,/, ft_unquote($tok->{AUTHOR}), 3;

	$itemh->{resph}= $resph;
	return 1;	# keepwhite
    }
    elsif ($tok->{TOKEN} eq '/RESP')
    {
    	&{$resp_cb}($itemh->{resph}, @extra);
	return 0;
    }
    elsif ($tok->{TOKEN} eq 'ERROR')
    {
	($ft_err,$ft_errmsg)= ('BTE1',"Request failed: ".
	    ft_unquote($tok->{MSG}));
	return 0;
    }
}


# dump_itemh($itemh)
#   Dump and item handle.  (For debugging)
sub dump_itemh {
    my ($itemh)= @_;
    print "ITEMH $itemh\n";
    print "confh=$itemh->{confh}\n";
    print "number=$itemh->{number}\n";
    print "isel=$itemh->{isel}\n";
    print "title=$itemh->{title}\n";
    print "linkdate=$itemh->{linkdate}\n";
    print "lastdate=$itemh->{lastdate}\n";
    print "maxresp=$itemh->{maxresp}\n";
    print "firstresp=$itemh->{firstresp}\n";
    print "frozen=$itemh->{frozen}\n";
    print "retired=$itemh->{retired}\n";
    print "forgotten=$itemh->{forgotten}\n";
    print "authoruid=$itemh->{authoruid}\n";
    print "authorname=$itemh->{authorname}\n";
}


1;
