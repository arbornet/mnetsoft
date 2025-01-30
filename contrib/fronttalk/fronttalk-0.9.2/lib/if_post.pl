# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #


# ft_itemop($op, $confh, $isel, $qry)
#  Operate on the given item in the given conference.  If op is:
#    F - forget it.
#    M - remember it.
#    Z - freeze it.
#    T - thaw it.
#    K - kill it.
#    R - retire it.
#    U - unretire it.
#    S - mark it seen.
#  Returns a message - either an error message or confirmation messages.

%opname= ( F => 'forgotten',    M => 'remembered',    Z => 'frozen',
           T => 'thawed',       K => 'killed',        R => 'retired',
           U => 'unretired',    S => 'marked seen',   V => 'favored',
	   D => 'disfavored'
         );

sub ft_itemop {
    my ($op, $confh, $isel, $qry)= @_;
    my ($page,$err);

    $page= ft_runscript($sysh, "/fronttalk/itemop",
    	"conf=$confh->{alias}&op=$op&isel=".ft_httpquote($isel)."$qry", 1);
    return $ft_errmsg if $ft_err;

    print "ITEMOP:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'ERROR')
	{
	    $err.= ft_unquote($tok->{MSG})."\n";
	}
	elsif ($tok->{TOKEN} eq 'DONE')
	{
	    $err.= "Item ".$tok->{ITEM}." $opname{$op}.\n";
	}
    }
    return $err;
}


# ft_respop($op, $confh, $item, $resp)
#  Operate on the given response of the given item in the given conference.
#  If op is
#    U - make it the first unseen response.
#    H - hide it.
#    S - show it.
#    E - erase it.

sub ft_respop {
    my ($op, $confh, $item, $resp)= @_;
    my ($page);

    $page= ft_runscript($sysh, "/fronttalk/respop",
    	"conf=$confh->{alias}&op=$op&item=$item&resp=$resp", 1)
    	or return undef;

    print "RESPOP:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'OK')
	{
	    return undef;
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    return ft_unquote($tok->{MSG})."\n";
	}
    }
    return "Permission Denied\n";
}


# ft_enter($confh, $title, $text)
#  Enter a new item into the given conference.
#  Return the item number on success.

sub ft_enter {
    my ($confh, $title, $text)= @_;

    $page= ft_runscript($sysh, "/fronttalk/enter",
    	"conf=$confh->{alias}&text=".ft_httpquote($text).
	    '&title='.ft_httpquote($title), 1)
    	or return undef;

    print "ENTER:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'OK')
	{
	    return $tok->{N};
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    ($ft_err,$ft_errmsg)= ('BTE1',"Request failed: ".
		ft_unquote($tok->{MSG}));
	    return undef;
	}
    }
    return undef;
}


# ft_respond($confh, $item, $text, $alias)
#  Respod to a item into the given conference.
#  Return the response number on success.

sub ft_respond {
    my ($confh, $item, $text, $alias)= @_;

    $page= ft_runscript($sysh, "/fronttalk/respond",
    	"conf=$confh->{alias}&text=".ft_httpquote($text).
	"&item=$item&pseudo=".ft_httpquote($alias), 1)
    	or return undef;

    print "RESPOND:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'OK')
	{
	    return $tok->{N};
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    ($ft_err,$ft_errmsg)= ('BTR1',"Request failed: ".
		ft_unquote($tok->{MSG}));
	    return undef;
	}
    }
    return undef;
}


# ft_retitle($confh, $item, $private, $newtitle)
#  Retitle the given item in the given conference.  Returns an error message
#  on failure, undef otherwise.

sub ft_retitle {
    my ($confh, $item, $private, $newtitle)= @_;
    my ($page,$err);
    print "ft_retitle($confh->{alias}, $item, $private, $newtitle)\n" if $debug;

    $page= ft_runscript($sysh, "/fronttalk/retitle",
    	"conf=$confh->{alias}&item=$item&private=$private&new=".
	ft_httpquote($newtitle), 1)
    	or return "Retitle request failed";

    print "RETITLE:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	$err.= ft_unquote($tok->{MSG})."\n"
	    if ($tok->{TOKEN} eq 'ERROR');
    }
    return $err;
}


# ft_link($srcconf, $confh, $isel, $qry)
#   Link items from the source conference whose name (not handle) is given
#   by $srcconf.  The items to link are selected by the $isel and $qry.  We
#   link to the conference whose handle is given as $confh.
#   Returns a message - either an error message or confirmation messages.

sub ft_link {
    my ($src, $confh, $isel, $qry)= @_;
    my ($page,$msg);

    $page= ft_runscript($sysh, "/fronttalk/link",
    	"dst=$confh->{alias}&src=$src&isel=".ft_httpquote($isel)."$qry", 1);
    return $ft_errmsg if $ft_err;

    print "LINK:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'ERROR')
	{
	    $msg.= ft_unquote($tok->{MSG})."\n";
	}
	elsif ($tok->{TOKEN} eq 'DONE')
	{
	    $msg.= "$srcconf item $tok->{SRCITEM} linked to $confh->{alias} item $tok->{DSTITEM}.\n";
	}
    }
    return $msg;
}


# ft_respop($op, $confh, $item, $resp)
#  Operate on the given response of the given item in the given conference.
#  If op is
#    U - make it the first unseen response.
#    H - hide it.
#    S - show it.
#    E - erase it.

sub ft_respop {
    my ($op, $confh, $item, $resp)= @_;
    my ($page);

    $page= ft_runscript($sysh, "/fronttalk/respop",
    	"conf=$confh->{alias}&op=$op&item=$item&resp=$resp", 1)
    	or return undef;

    print "RESPOP:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'OK')
	{
	    return undef;
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    return ft_unquote($tok->{MSG})."\n";
	}
    }
    return "Permission Denied\n";
}


# ft_enter($confh, $title, $text)
#  Enter a new item into the given conference.
#  Return the item number on success.

sub ft_enter {
    my ($confh, $title, $text)= @_;

    $page= ft_runscript($sysh, "/fronttalk/enter",
    	"conf=$confh->{alias}&text=".ft_httpquote($text).
	    '&title='.ft_httpquote($title), 1)
    	or return undef;

    print "ENTER:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'OK')
	{
	    return $tok->{N};
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    ($ft_err,$ft_errmsg)= ('BTE1',"Request failed: ".
		ft_unquote($tok->{MSG}));
	    return undef;
	}
    }
    return undef;
}


# ft_respond($confh, $item, $text, $alias)
#  Respod to a item into the given conference.
#  Return the response number on success.

sub ft_respond {
    my ($confh, $item, $text, $alias)= @_;

    $page= ft_runscript($sysh, "/fronttalk/respond",
    	"conf=$confh->{alias}&text=".ft_httpquote($text).
	"&item=$item&pseudo=".ft_httpquote($alias), 1)
    	or return undef;

    print "RESPOND:\n$page\n" if $debug;

    # Parse info out of conference info page
    while ($tok= ft_parse(\$page,$in))
    {
	if ($tok->{TOKEN} eq 'OK')
	{
	    return $tok->{N};
	}
	elsif ($tok->{TOKEN} eq 'ERROR')
	{
	    ($ft_err,$ft_errmsg)= ('BTR1',"Request failed: ".
		ft_unquote($tok->{MSG}));
	    return undef;
	}
    }
    return undef;
}



1;
