# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #


# ft_updateconf($confh)
#   Update our information on an open conference.

sub ft_updateconf {
    my ($confh)= @_;
    my ($page, $in);

    $page= ft_runscript($confh->{sysh}, "/fronttalk/join",
	"conf=$confh->{alias}", 1)
    	or return;

    print "UPDATECONF:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
    	if ($tok->{TOKEN} eq 'text')
	{
	    $confh->{$in}= $tok->{TEXT} if $in;
	}
	elsif ($tok->{TOKEN} eq 'CONF')
	{
	    $confh->{name}= ft_unquote($tok->{NAME});
	    $confh->{readonly}= $tok->{READONLY};
	    $confh->{noenter}= $tok->{NOENTER};
	    $confh->{fishbowl}= $tok->{FISHBOWL};
	    $confh->{userlist}= $tok->{USERLIST};
	    $confh->{grouplist}= $tok->{GROUPLIST};
	    $confh->{password}= $tok->{PASSWORD};
	    $confh->{partfile}= ft_unquote($tok->{PARTFILE});
	    $confh->{fwlist}= ft_unquote($tok->{FW});
	    $confh->{logindate}= $tok->{LOGINDATE};
	    $confh->{logoutdate}= $tok->{LOGOUTDATE};
	    $confh->{indexdate}= $tok->{INDEXDATE};
	    $confh->{bulldate}= $tok->{BULLDATE};
	    $confh->{confrcdate}= $tok->{CONFRCDATE};
	    $confh->{ulistdate}= $tok->{ULISTDATE};
	    $confh->{welcomedate}= $tok->{WELCOMEDATE};
	}
	elsif ($tok->{TOKEN} eq 'ITEMS')
	{
	    $confh->{first}= $tok->{FIRST};
	    $confh->{last}= $tok->{LAST};
	    $confh->{items}= $tok->{N};
	}
	elsif ($tok->{TOKEN} eq 'USER')
	{
	    $confh->{myname}= ft_unquote($tok->{NAME});
	    $confh->{newr}= $tok->{NEWRESP};
	    $confh->{newi}= $tok->{NEWITEM};
	    $confh->{unseen}= $tok->{UNSEEN};
	    $confh->{amfw}= $tok->{FAIRWITNESS};
	    $confh->{lastin}= $tok->{LASTIN};
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
	    return;
	}
    }
}


# ft_getconf($confh,$what...)
#     Fill in some of the less commonly used fields in the conference handle.
#     List the extra field names you want:
#
#     index    - conference index file
#     bull     - conference bulletin file
#     welcome  - conference welcome file
#     ulist    - conference ulist file
#     glist    - conference glist file

sub ft_getconf {
    my $confh= shift;
    my ($name, $query, $page, $in);

    $ft_err= undef;

    foreach $name (@_)
    {
    	next if defined $confh->{$name};
	$query.= "&get$name=1";
    }
    return if !$query;

    $page= ft_runscript($confh->{sysh}, "/fronttalk/confextra",
    	"conf=$confh->{alias}$query", 0);
    return if $ft_err;

    print "CONFEXTRA:\n$page\n" if $debug;

    # Parse info out of conference extra info page
    while ($tok= ft_parse(\$page,$in))
    {
    	if ($tok->{TOKEN} eq 'text')
	{
	    $confh->{$in}.= ft_unquote($tok->{TEXT}) if $in;
	}
	elsif ($tok->{TOKEN} =~ /^(INDEX|BULLETIN|WELCOME|ULIST|GLIST|CONFRC)$/)
	{
	    $in= lc($1);
	}
	elsif ($tok->{TOKEN} =~ /^\/(INDEX|BULLETIN|WELCOME|ULIST|GLIST|CONFRC)$/)
	{
	    $in= undef;
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    ($ft_err,$ft_errmsg)= ('BTE1',"Request failed: ".
	    	ft_unquote($tok->{MSG}));
	}
    }
}


# ft_resignconf($confh)
#   Resign from an open conference.  Returns an error message on failure,
#   undef on success.

sub ft_resignconf {
    my ($confh)= @_;
    my ($page, $tok, $err);

    $page= ft_runscript($confh->{sysh}, "/fronttalk/resign",
	"conf=$confh->{alias}", 1)
    	or return $ft_errmsg;

    print "RESIGN:\n$page\n" if $debug;

    # Parse error messages out of response
    while ($tok= ft_parse(\$page,0))
    {
	if ($tok->{TOKEN} eq 'ERROR')
	{
	    $err.= ft_unquote($tok->{MSG})."\n";
	}
	elsif ($tok->{TOKEN} eq 'DONE')
	{
	}
    }
    return $err;
}


1;
