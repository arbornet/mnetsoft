# Copyright 2005, Jan D. Wolter, All Rights Reserved.

use POSIX qw(:signal_h);

# This will be the handle for the readline if we have readline.  It
# is undefined if readline has not yet been initalized, or has been
# turned off via "set noreadline".
my $term= undef;

# The last line added to the history
my $lastline= undef;


# start_readline()
#
# Initialize the readline package, if there is one available.  Once
# this is called, iget() will use the readline package instead of just
# reading from STDIN.  If no readline library exists we return false.

sub start_readline
{
    print "start_readline()\n" if $debug;

    # if we aren't on a terminal, don't bother
    return 1 if ! -t STDIN;

    # if we already have readline on, do nothing
    return 1 if $term;

    # Try loading a readline package
    eval { require Term::ReadLine; };
    return 0 if $@;

    # Get readline handle
    eval { $term=  new Term::ReadLine 'FrontTalk'; };
    if ($@)
    {
	# Maybe tty is not set up yet ... wait a sec and try again
	sleep 1;
	eval { $term=  new Term::ReadLine 'FrontTalk'; };
	print "ReadLine error: $@\n" if $@;
    }
    return 1 if !$term;

    # If all we got was the stub, don't bother
    if ($term->ReadLine() =~ /::Stub$/)
    {
	$term= undef;
	return 0;
    }

    # We will add stuff to history ourselves
    $term->MinLine(10000);
    $term->ornaments(0);

    return 1;
}


# stop_readline()
#
# Turns readline off.  Called when a "set noreadline" command is called.

sub stop_readline
{
    $term= undef;
    $lastline= undef;
}


# disp_readline()
#
# Print a string describing readline's state.  Called when
# we do "display readline".

sub disp_readline
{
    print 'readline module is '.( $term ?
    	('on ('.$term->ReadLine().")\n") :
	"off\n");
}


# Signal handler for control-C interupts during readline.

sub rl_sigint
{
    $SIG{INT}= \&rl_sigint if $] < 5.007003;
    print "\n";
    die "RL_SIGNAL=INT";
}

# Signal handler for control-Z interupts during readline.

sub rl_sigtstp
{
    if ($] < 5.007003)
    {
	$SIG{TSTP}= 'DEFAULT';
    }
    else
    {
	POSIX::sigaction(&POSIX::SIGTSTP, POSIX::SigAction->new('DEFAULT'));
    }
    if ($term)
    {
	$term->cleanup_after_signal();
    }
    print "\n";
    kill 'TSTP', $$;
}

sub rl_sigcont
{
    if ($] < 5.007003)
    {
	$SIG{TSTP}= \&readline_sigtstp;
	$SIG{CONT}= \&readline_sigcont;
    }
    else
    {
	my $sigact= POSIX::SigAction->new(\&main::rl_sigtstp,
					  POSIX::SigSet->new, 0);
	POSIX::sigaction(&POSIX::SIGTSTP, $sigact);
    }
    if ($term)
    {
	$term->reset_after_signal();
	$term->forced_update_display();
    }
    else
    {
	print $currprompt;
    }
}


# $line= iget($prompt, $intreturn, $prompt_hook)
#
# Interactive line getting routine.  If we have some version of
# Term::ReadLine, we use it and record the input in the history
# buffer.  Otherwise, just grab a line from STDIN.  Line is returned
# without a trailing newline.
#
# The string prompt is printed before each input.  It can contain newlines.
# The prompt can be an empty string or undefined.
#
# On EOF, should return undef.
#
# If $intreturn is true, iget will return an empty line after the user
# presses ^C.
#
# If $prompt_hook is defined, it is a routine that is called before printing
# the prompt.
#
# This routine was originally derived from a similar one in Gregor N Purdy's
# psh program.  It has evolved a bit.

sub iget
{
    my ($prompt, $intreturn, $phook)= @_;
    my $preprompt= '';
    my $line;

    # Additional newline handling for prompts as Term::ReadLine::Perl
    # cannot use them properly
    if( $term && $term->ReadLine eq 'Term::ReadLine::Perl' &&
	$prompt=~ /^(.*\n)([^\n]+)$/)
    {
	$preprompt= $1;
	$prompt= $2;
    }
    $currprompt= $prompt;

    my $oldint, $oldtstp, $oldcont;

    if ($] < 5.007003)
    {
	# In older versions of Perl, this works fine
	$oldint= $SIG{INT};
	$SIG{INT}= \&rl_sigint;

	$oldtstp= $SIG{TSTP};
	$SIG{TSTP}= \&rl_sigtstp;

	$oldcont= $SIG{CONT};
	$SIG{CONT}= \&rl_sigcont;
    }
    else
    {
	# In newer versions we use Posix signals to avoid the deferred signal
	# behavior in which interrupts don't interrupt I/O.
	$oldint= POSIX::SigAction->new();
	my $sigact= POSIX::SigAction->new(\&main::rl_sigint,
					  POSIX::SigSet->new, 0);
	POSIX::sigaction(&POSIX::SIGINT, $sigact, $oldint);

	$oldcont= POSIX::SigAction->new();
	$sigact= POSIX::SigAction->new(\&main::rl_sigcont,
					  POSIX::SigSet->new, 0);
	POSIX::sigaction(&POSIX::SIGCONT, $sigact, $oldcont);

	$oldtstp= POSIX::SigAction->new();
	$sigact= POSIX::SigAction->new(\&main::rl_sigtstp,
					  POSIX::SigSet->new, 0);
	POSIX::sigaction(&POSIX::SIGTSTP, $sigact, $oldtstp);
    }

    # This loop iterates more than once only if input is interupted
    while(1)
    {

	# Trap ^C in an eval.  The sighandler will die which will be
	# trapped.  Then we reprompt
	if ($term)
	{
	    &$phook if $phook;
	    print $preprompt;
	    eval { $line= $term->readline($prompt); };
	} else {
	    eval {
		&$phook if $phook;
		print $prompt;
		$line= <STDIN>;
	    };
	    chomp $line;
	}

	# if eval was not interrupted by signal, we are done
	last if !$@;

	# Otherwise, print a message and try again
	if ( $@ =~ /^RL_SIGNAL=INT/)
	{
	    print "Input interrupted!\n";
	    $line= '';
	    last if $intreturn;
	}
	else
	{
	    print "Input interrupted: '$@'\n";
	}
    }

    # restore previous signals
    if ($] < 5.007003)
    {
	$SIG{INT}= $oldint;
	$SIG{TSTP}= $oldtstp;
	$SIG{CONT}= $oldcont;
    }
    else
    {
	POSIX::sigaction(&POSIX::SIGINT, $oldint);
	POSIX::sigaction(&POSIX::SIGTSTP, $oldtstp);
	POSIX::sigaction(&POSIX::SIGCONT, $oldcont);
    }

    # Add line to history
    if ($term and $line =~ /\S/ and $line ne $lastline)
    {
	$term->addhistory($line);
	$lastline= $line;
    }

    return $line;
}


# echo($turnon)
#   This turns echo on or off for normal reading from standard input (not
#   for readline).  Normally usage is to call echo(0) before the read and
#   echo(1) after.

my $echoon= 1;

sub echo
{
    my $turnon= shift;
    # if we are already in the desired state, do nothing
    return if ($turnon and $echon) or (!$turnon and !$echoon);
    $echoon= 0;  # state is unknown if interupted unix command assume the worst
    system $turnon ? "$PATH_STTY echo" : "$PATH_STTY -echo";
    $echoon= $turnon;
}

1;
