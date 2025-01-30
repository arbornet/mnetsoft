# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #

# FrontTalk Interface

my $token_func;
my @token_arg;

# ft_getfile($fn)
#   Return the contents of a file or undef if it does not exist or cannot be
#   read.

sub ft_getfile {
    my ($file)= @_;
    my ($text);

    open FILE, $file
       or return undef;

    while (<FILE>)
    {
    	$text.= $_;
    }

    close FILE;

    return $text;
}

# ft_killchild()
#   Kill off any child process running under ft_getexec().

sub ft_killchild {
    return if !$child_pid;
    kill 9, $child_pid;
    waitpid $child_pid, 0;
}


# ft_unquote($html)
#   Given a text string with HTML character quoting, unquote it.  That is,
#   we make the following mappings:
#      &lt;   ->  <
#      &gt;   ->  >
#      &quot; ->  "
#      &apos; ->  '
#      &#39;  ->  '
#      &amp;  ->  &

sub ft_unquote {
    my ($str)= @_;
    $str=~ s/\&lt;/\</g;
    $str=~ s/\&gt;/\>/g;
    $str=~ s/\&quot;/"/g;
    $str=~ s/\&apos;/'/g;
    $str=~ s/\&#39;/'/g;
    $str=~ s/\&amp;/\&/g;
    return $str;
}


# ft_httpquote($html)
#   Given a text string do HTTP quoting on it.

sub ft_httpquote {
    my ($str)= @_;
    $str=~ s/\%/%25/g;
    $str=~ s/\0/%00/g;
    $str=~ s/"/%22/g;
    $str=~ s/'/%27/g;
    $str=~ s/\+/%2B/g;
    $str=~ s/ /\+/g;
    $str=~ s/\&/%26/g;
    $str=~ s/\=/%3D/g;
    $str=~ s/\>/%3E/g;
    $str=~ s/\</%3C/g;
    return ($str);
}


# ft_parse($textref,$keepwhite)
#   Return the next token out of the text.  Returns a hashref, with the
#   following fields:
#     Text strings:
#         {TOKEN=>text, VALUE=>string
#     HTML-like Tags:  (eg <FOO BAR=BAZ asdf=hjkl> )
#         {TOKEN=>FOO, BAR=>BAZ, ASDF=>hjkl}
#   If the text is empty, or contains no complete token, returns undef.

sub ft_parse {
    my ($text,$keepwhite)= @_;
    my ($t,$a,$k,$v);

    if ($keepwhite)
    {
    	$$text=~ s/^\n//;
    }
    else
    {
	$$text=~ s/^\s*//g;
    }

    return undef if $$text =~ /^\s*$/;
    if ($$text =~ s/^<([^\s>]+)\s*(([^"'>]|"[^"]*"|'[^']*')*)>//)
    {
    	$t->{TOKEN}= $1;
	$a= $2;
	while ($a =~ /(\w+)(=(?:([^"'\s>]*)|"([^"]*)"|'([^']*)'))?(?:\s|$)/g)
	{
	    $k= uc($1);
	    $v= $2 ? ($3 or $4 or $5) : 1;
	    $t->{$k}= $v;
	}
	return $t;
    }
    elsif ($$text =~ s/([^<]+)(?=<)//)
    {
	$t->{TOKEN}= 'text';
	$t->{TEXT}= ft_unquote($1);
	return $t;
    }
    else
    {
    	return undef;
    }
}


# ft_getsyslist($syslist)
#   Load a system list file into memory.  This can be called multiple times
#   to load multiple system lists.

sub ft_getsyslist {
    my ($path)= @_;
    my ($content, $tok, $name, $namelist, $href, $des, $code, $in, $def);

    # Grab the content, either locally or off the web.
    if ($path =~ /^https?:\/\//)
    {
	require "net_http.pl";
    	($content, $code)= ft_gethttp($path);
    }
    else
    {
	$path=~ s/^file://;
    	$content= ft_getfile($path);
    }
    print "SYSLIST:\n$content\n" if $debug;

    while ($tok= ft_parse(\$content))
    {
        if ($tok->{TOKEN} eq 'text' and $in eq 'system')
	{
	    $des= $tok->{TEXT};
	}
	elsif ($tok->{TOKEN} eq 'SYSTEM')
	{
	    print "SYSTEM: $tok->{NAME} $tok->{HREF}\n" if $debug;
	    $namelist= $tok->{NAME};
	    $href= $tok->{HREF};
	    $des= undef;
	    $def= $tok->{DEFAULT};
	    $in= 'system';
	}
	elsif ($tok->{TOKEN} eq '/SYSTEM')
	{
	    # this should go into a hash instead of an array
	    foreach $name (split /,/, $namelist)
	    {
		push @syslist, [$name, $href, $des];
	    }
	    $default_sys= $#syslist
		if !defined $default_sys and $def;
	    $in= undef;
	}
	elsif ($tok->{TOKEN} eq 'INCLUDE' and $tok->{HREF} and !$in)
	{
	    ft_getsyslist($tok->{HREF});
	}
    }

    $default_sys= 0 if !defined $default_sys;
}


# ft_runscript_cb($text)
#   This is used if ft_runscript() is passed a callback function. It is called
#   by ft_gethttp() or ft_getexec(), and in turn calls whatever callback
#   function was passed to ft_runscript.  The function and extra args are
#   in globals because we can't pass data through the lower levels.
#   So This is called consecutively with various hunks from the text of a
#   response.  It in turn calls the function defined by the global variable
#   $token_func with each parsed token.  $token_func should return 1 if
#   keepwhite should be turned on while parsing the next token.

sub ft_runscript_cb {
   my ($txt)= @_;
   my ($tok, $keepwhite);

   print "FT_RUNSCRIPT_CB TXT=$txt\nEOT\n" if $debug;

   $saved_text.= $txt;

   $keepwhite= 0;
   while ($tok= ft_parse(\$saved_text, $keepwhite))
   {
	print "GOT TOKEN $tok->{TOKEN}\n" if $debug;
   	$keepwhite= &{$token_func}($tok,@token_arg);
   }
   return;
}


# ft_runscript($sysh, $scriptpath, $qry, $auth, [$callback, [$extra...]])
#   Run the local or remote Backtalk CGI described by the system handle, and
#   return a page.  If $auth is true, we will attempt to login instead of
#   just running anonymously, unless $sysh->{amanon} is set or the user gives
#   no password.  If it isn't we may or may not login, whichever
#   is handier.  Scriptpath is like "/flavor/script".  If $callback is defined,
#   then instead of returning the page, we will make repeated calls to the
#   callback routine with ft_parse style tokens parsed out if it.  In this
#   case undef is returned.

sub ft_runscript {
    my ($sysh, $script, $qry, $auth);
    my ($page,$loc,$code, $cb);

    ($sysh, $script, $qry, $auth, $token_func, @token_arg)= @_;

    print "ft_runscript SCRIPT=$script QRY=$qry AUTH=$auth\n" if $debug;

    # Set up lower-level callback if we are supposed to do a callback
    $cb= \&ft_runscript_cb if $token_func;

    $ft_err= undef;
    $saved_text= undef;

    # Do an anonymous request if the user is already doing anonymous reading
    if ($auth == 1 and $sysh->{anon} and $sysh->{amanon})
    {
	$auth= 0;
    }
    # Do authentication if the script requested requires it
    elsif ($auth == 0 and !$sysh->{anon} and $script !~ /^\/public\//)
    {
	$auth= 1
    }

    if ($sysh->{direct})
    {
	require "net_exec.pl";
    	$page= ft_getexec($sysh->{location}, $script, $qry, $cb);
	$sysh->{username}= getpwuid($<) if !$sysh->{username};
    }
    else
    {
	require "net_http.pl";
	$page= ft_runhttp($sysh, $script, $qry, $auth, $cb);
	return undef if $callback or $ft_err;
    }

    # Check for Backtalk bomb screens
    if ($page =~ /^\s*<HTML/)
    {
    	if ($page =~ /Backtalk script named '(.*)'\./)
	{
	    ($ft_err,$ft_errmsg)= ('IFR3', "Backtalk script '$1' not found");
	    return undef;
	}
	elsif ($page =~ /ERROR:  (.*)<BR>/)
	{
	    ($ft_err,$ft_errmsg)= ('IFR4', "Backtalk script error: '$1'");
	    return undef;
	}
    }

    return $page;
}


# ft_init($syslist, \&loginfunc)
#   Initialize the FrontTalk Interface.  Get the system list.  loginfunc is
#   a reference to a function that will return a list containing a login and a
#   password for a system whose name is passed in as an arguement - it
#   normally prompts the user.

sub ft_init
{
    my ($syslist,$lf)= @_;

    $ft_err= undef;

    print "Syslist=$syslist Agent='$AGENT' Proxy=$PROXY\n" if $debug;

    $login_func= $lf;

    ft_getsyslist($syslist);

    @syslist > 0 or
    	($ft_err, $ft_errmsg)=
	    ('IFI1', "Could not load system list '$syslist'\n");
}


1;
