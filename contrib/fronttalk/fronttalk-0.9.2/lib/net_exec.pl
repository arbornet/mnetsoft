# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #

# ft_getexec($fn, $extra, $qry, [$callback])
#   Run the Backtalk CGI program at the given path in an environment that
#   makes it look like got a POST query with the given extra path info and
#   query string.  If $callback is defined, we call that with small hunks
#   of data instead of returning the whole thing.

sub ft_getexec {
    my ($file, $extra, $query, $callback)= @_;
    my ($resp, $ln);

    print "getexec($file, $extra, $query)\n" if $debug;

    close FROM_PARENT;
    close TO_PARENT;
    close FROM_CHILD;
    close TO_CHILD;
    if (!pipe(FROM_PARENT,TO_CHILD) or
	!pipe(FROM_CHILD,TO_PARENT))
    {
	($ft_err, $ft_errmsg)= ('IFE1', "Could not create pipes: $!");
    	return undef;
    }

    if ($child_pid= fork)
    {
	# parent process
        close TO_PARENT;
        close FROM_PARENT;
        #select((select(TO_CHILD), $|= 1)[0]);   # set autoflush

        print TO_CHILD "$query";
        close TO_CHILD;
    }
    elsif (defined $child_pid)
    {
	# child process
	$SIG{INT}= 'IGNORE';
	$SIG{PIPE}= 'IGNORE';
        close TO_CHILD;
        close FROM_CHILD;
        select((select(TO_PARENT), $|= 1)[0]);  # set autoflush

        # dup2 from_parent to stdin 0
        open STDIN,'>&'.fileno(FROM_PARENT);

        # dup2 to_parent to stdout 1
        open STDOUT, '>&'.fileno(TO_PARENT);

	# Set up environment - Just the minimum Backtalk must have
	$ENV{REQUEST_METHOD}= 'POST';
	$ENV{CONTENT_LENGTH}= length($query);
	$ENV{HTTP_USER_AGENT}= $AGENT;
	$ENV{REMOTE_ADDR}= '127.0.0.1';
	$ENV{PATH_INFO}= $extra;
	$ENV{SERVER_NAME}= 'localhost';

	# Run Backtalk
	exec $file, '-d';
	print STDERR "Could not exec Backtalk at '$file'\n";
	exit;
    }
    else
    {
	($ft_err, $ft_errmsg)= ('IFE3', "Could not fork: $!");
    	return undef;
    }

    # Skip HTTP headers
    while ($ln= <FROM_CHILD> && $ln !~ /^$/)
    {
    }

    # Get Response
    while ($ln= <FROM_CHILD>)
    {
	$resp.= $ln;
	if ($callback and length($resp) > 512)
	{
	    &{$callback}($resp);
	    $resp= undef;
	}
    }
    close FROM_CHILD;
    &{$callback}($resp) if $callback and $resp;
    waitpid $child_pid, 0;
    $child_pid= undef;

    return $callback ? undef : $resp;
}


1;
