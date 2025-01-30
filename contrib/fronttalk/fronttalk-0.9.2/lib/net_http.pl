# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #

# FrontTalk Interface

use LWP::UserAgent;
use HTTP::Cookies;

%httperr= (
   401 => 'Unauthorized',
   403 => 'Forbidden',
   404 => 'Not Found',
   407 => 'Proxy Authentication Required',
   408 => 'Request Timeout',
   410 => 'Gone',
   500 => 'Internal Server Error',
   501 => 'Not Implemented',
   502 => 'Bad Gateway',
   503 => 'Service Unavailable',
   504 => 'Gateway Timeout',
   );

my $ua;

# ($text, $code)= ft_gethttp($url, $qry, $login, $passwd, [$callback])
#   Hit "http://$url?$qry" and return the resulting page and response code.
#   Does a GET if the query is empty, and a POST otherwise.  If a callback
#   routine is given, this routine will be called with chunks of data as
#   they come in.

sub ft_gethttp {
    my ($url, $qry, $login, $passwd, $callback)= @_;
    my ($req,$res);
    print "ft_gethttp($url, $qry, $login, <password>",
    	($callback ? ", <callback>":''),")\n" if $debug;

    $ua or ft_initua();

    if ($qry)
    {
	$req= new HTTP::Request 'POST' => $url;
	$req->content_type('application/x-www-form-urlencoded');
	$req->content($qry);
	print "POST\n" if $debug;
    }
    else
    {
	$req= new HTTP::Request 'GET' => $url;
	print "GET\n" if $debug;
    }
    $req->authorization_basic($login, $passwd) if $login;

    $res= $ua->request($req, $callback, 512);

    return ($res->content, $res->code);
}


# $text= ft_runhttp($sysh, $script, $qry, $auth, $cb)
#   Slightly higher level http hit.  Given system handle and backtalk script
#   to run, construct the URL and return the result.  Prompts for login if
#   needed.

sub ft_runhttp {
    my ($sysh, $script, $qry, $auth, $cb)= @_;

    if (!defined $sysh->{username} and $auth)
    {
	# Get login ID from login callback function
	$auth= ft_login($sysh,$login_func);
	return undef if $ft_err;
    }

    if (!$auth or $sysh->{login} eq 'cookie')
    {
	# If not authenticating or server is cookie based, use unprotected cgi
	$loc= $sysh->{location};
	# If we haven't authenticated yet, add login/password to query.
	if ($auth and !$sysh->{session})
	{
	    $qry.= '&' if $qry;
	    $qry.= 'x1='.ft_httpquote($sysh->{username}).
		  '&x2='.ft_httpquote($sysh->{password});
	}
    }
    else
    {
	# If authenticating with basic auth, use protected cgi
	$loc= $sysh->{pwurl};
    }

    $loc= ($auth and $sysh->{login} eq 'basic') ?
	$sysh->{pwurl} : $sysh->{location};

    while (1)
    {
	($page,$code)= ft_gethttp($loc.$script, $qry,
	    $sysh->{username}, $sysh->{password}, $cb);
	return $page if $code < 400;

	if ($code == 401)
	{
	    # authentication failed, try again
	    $page= undef;
	    print STDERR "Access Denied, try again:\n";
	    $auth= ft_login($sysh,$login_func);
	    return undef if $ft_err;
	    $loc= $sysh->{location} if !$auth;
	}
	else
	{
	    ($ft_err,$ft_errmsg)= ('IFR2',
		 "HTTP request failed - error $code $httperr{$code}");
	    return undef;
	}
    }
}


# ft_login(<system-handle>,<callback-function>)
#  Stuff username and password into the system handle.  These can be undefined
#  if the user chooses not to login and the system allows anonymous reading.
#  Returns 1 if we got a login, 0 if we legally went anonymous, undef on error.

sub ft_login {
    my ($sysh, $login_func)= @_;
    my $rc;

    $ft_err= undef;

    # Get login ID from callback function
    ($rc, $sysh->{username}, $sysh->{password})=
	&{$login_func}($sysh->{anon},$sysh->{name});

    if (!$rc)
    {
	($ft_err,$ft_errmsg)= ('IFL1', "Login Cancelled");
	return undef;
    }

    if (defined $sysh->{username})
    {
	$sysh->{amanon}= 0;
	return 1;
    }

    # No login given - user wants to be anonymous
    if ($sysh->{anon})
    {
	print STDERR "Accessing $sysh->{name} in anonymous read-only mode\n";
	$sysh->{amanon}= 1;
	return 0
    }

    # Attempt to be anonymous on a system that doesn't allow it
    ($ft_err,$ft_errmsg)=
	('IFL2', "Unable to access $sysh->{name} without login");
    return undef;
}


# ft_initua()
#   Initialize the User Agent based on the global $AGENT and $PROXY variables.

sub ft_initua
{
    my $cookies= HTTP::Cookies->new;

    $ua= new LWP::UserAgent;
    $ua->agent($AGENT);
    $ua->cookie_jar($cookies);

    $ua->proxy(http => $PROXY) if $PROXY;
}


1;
