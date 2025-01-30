# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# Command sources values for global $cmd_source variable:
#  0 = remote system RC file
#  1 = remote .cfonce file
#  2 = local .cfonce file
#  3 = conference rc file
#  4 = remote .cfrc file
#  5 = local .cfrc file
#  6 = command line

@is_remote= (1,1,0,1,1,0,0);


# OK Prompt Commands
@okcmd= (
#        command   min/maxmatch function	require
	['read',      1, 4,     \&cmd_read],
	['next',      1, 4,     \&cmd_next],
	['join',      1, 4,     \&cmd_join],
	['server',    3, 6,     \&cmd_server],
	['host',      3, 4,     \&cmd_server],
	['system',    2, 6,     \&cmd_server],
	['connect',   2, 7,     \&cmd_server],
	['browse',    1, 6,     \&cmd_browse,	'cmd_scan.pl'],
	['scan',      1, 4,     \&cmd_browse,	'cmd_scan.pl'],
	['enter',     1, 5,     \&cmd_enter,	'cmd_enter.pl'],
	['respond',   3, 7,	\&cmd_resp,	'cmd_enter.pl'],
	['check',     2, 5,     \&cmd_check,	'cmd_scan.pl'],
	['find',      2, 4,     \&cmd_find,	'cmd_find.pl'],
	['favor',     3, 5,     \&cmd_favor,	'cmd_mode.pl'],
	['disfavor',  6, 8,     \&cmd_disfavor,	'cmd_mode.pl'],
	['forget',    1, 6,     \&cmd_forget,	'cmd_mode.pl'],
	['remember',  3, 8,     \&cmd_remember,	'cmd_mode.pl'],
	['freeze',    2, 6,     \&cmd_freeze,	'cmd_mode.pl'],
	['thaw',      4, 4,     \&cmd_thaw,	'cmd_mode.pl'],
	['retire',    3, 6,     \&cmd_retire,	'cmd_mode.pl'],
	['unretire',  5, 8,     \&cmd_unretire,	'cmd_mode.pl'],
	['retitle',   4, 7,     \&cmd_retitle,	'cmd_mode.pl'],
	['mytitle',   3, 7,     \&cmd_mytitle,	'cmd_mode.pl'],
	['kill',      4, 4,     \&cmd_kill,	'cmd_mode.pl'],
	['fixseen',   3, 7,     \&cmd_fixseen,	'cmd_mode.pl'],
	['linkfrom',  3, 8,     \&cmd_link,	'cmd_link.pl'],
	['participants',2, 12,  \&cmd_part,	'cmd_part.pl'],
	['set',       2, 3,     \&cmd_set],
	['change',    2, 6,     \&cmd_set],
	['display',   1, 7,     \&cmd_display,	'cmd_display.pl'],
	['define',    2, 6,     \&cmd_define],
	['undefine',  3, 8,     \&cmd_undef],
	['leave',     3, 5,     \&cmd_leave],
	['resign',    6, 6,     \&cmd_resign],
	['rewind',    3, 6,     \&cmd_rewind],
	['login',     3, 5,     \&cmd_login,	'cmd_login.pl'],
	['item',      1, 4,     \&cmd_read],
	['print',     2, 5,     \&cmd_print],
	['mail',      1, 4,     \&cmd_mail],
	['chat',      4, 4,     \&cmd_chat],
	['write',     2, 5,     \&cmd_chat],
	['who',       1, 3,     \&cmd_who],
	['unix',      1, 4,     \&cmd_unix],
	['sourcefile',2,10,     \&cmd_sourcefile],
	['echo',      4, 4,     \&cmd_echo,	'cmd_help.pl'],
	['echoe',     5, 5,     \&cmd_echoe,	'cmd_help.pl'],
	['echon',     5, 5,     \&cmd_echon,	'cmd_help.pl'],
	['echoen',    6, 6,     \&cmd_echoen,	'cmd_help.pl'],
	['echone',    6, 6,     \&cmd_echoen,	'cmd_help.pl'],
	['help',      1, 4,     \&cmd_help,	'cmd_help.pl'],
	['date',      2, 4,     \&cmd_date,	'cmd_date.pl'],
	['cdate',     3, 5,     \&cmd_cdate,	'cmd_date.pl'],
	['stop',      2, 4,     \&cmd_quit],
	['quit',      1, 4,     \&cmd_quit],
	['exit',      2, 4,     \&cmd_quit],
	['bye',       2, 3,     \&cmd_quit],
	['passwd',    5, 5,     \&cmd_passwd,	'cmd_passwd.pl'],
	);

# Respond/Forget/Pass prompt commands
@rfpcmd= (
#        command   min/maxmatch function	require
	['pass',       1, 4,     \&rfp_pass],
	['respond',    1, 7,     \&rfp_resp,	'cmd_enter.pl'],
	['pseudonym',  2, 9,     \&rfp_pseudo,	'cmd_enter.pl'],
	['enter',      1, 5,     \&rfp_enter,	'cmd_enter.pl'],
	['only',       1, 4,     \&rfp_only],
	['again',      2, 5,     \&rfp_again],
	['header',     3, 6,     \&rfp_head],
	['new',        1, 3,     \&rfp_new],
	['$',          1, 1,     \&rfp_last],
	['last',       1, 4,     \&rfp_last],
	['postpone',   2, 8,     \&rfp_hold],
	['preserve',   3, 8,     \&rfp_hold],
	['hold',       2, 4,     \&rfp_hold],
	['expurgate',  3, 9,     \&rfp_hide,	'cmd_mode.pl'],
	['hide',       2, 4,     \&rfp_hide,	'cmd_mode.pl'],
	['unhide',     4, 6,     \&rfp_show,	'cmd_mode.pl'],
	['unexpurgate',5, 11,    \&rfp_show,	'cmd_mode.pl'],
	['show',       2, 4,     \&rfp_show,	'cmd_mode.pl'],
	['scribble',   5, 8,     \&rfp_erase,	'cmd_mode.pl'],
	['erase',      5, 5,     \&rfp_erase,	'cmd_mode.pl'],
	['forget',     1, 6,     \&cmd_forget,	'cmd_mode.pl'],
	['remember',   3, 8,     \&cmd_remember,'cmd_mode.pl'],
	['freeze',     2, 6,     \&cmd_freeze,	'cmd_mode.pl'],
	['login',      3, 5,     \&cmd_login,	'cmd_login.pl'],
	['thaw',       4, 4,     \&cmd_thaw,	'cmd_mode.pl'],
	['retire',     3, 6,     \&cmd_retire,	'cmd_mode.pl'],
	['unretire',   5, 8,     \&cmd_unretire,'cmd_mode.pl'],
	['kill',       4, 4,     \&cmd_kill,	'cmd_mode.pl'],
	['retitle',    4, 7,     \&cmd_retitle,	'cmd_mode.pl'],
	['mytitle',    3, 7,     \&cmd_mytitle,	'cmd_mode.pl'],
	['find',       2, 4,     \&cmd_find,	'cmd_find.pl'],
	['favor',      3, 5,     \&cmd_favor,	'cmd_mode.pl'],
	['disfavor',   6, 8,     \&cmd_disfavor,'cmd_mode.pl'],
	['unix',       1, 4,     \&cmd_unix],
	['help',       1, 4,     \&cmd_help,	'cmd_help.pl'],
	['date',       2, 4,     \&cmd_date,	'cmd_date.pl'],
	['cdate',      3, 5,     \&cmd_cdate,	'cmd_date.pl'],
	['stop',       2, 4,     \&cmd_quit],
	['quit',       1, 4,     \&cmd_quit],
	['exit',       2, 4,     \&cmd_quit],
	['bye',        2, 3,     \&cmd_quit],
	['set',        2, 3,     \&cmd_set],
	['mail',       1, 4,     \&cmd_mail],
	['define',     2, 6,     \&cmd_define],
	['undefine',   3, 8,     \&cmd_undef],
	['chat',       1, 4,     \&cmd_chat],
	['display',    1, 7,     \&cmd_display, 'cmd_display.pl'],
	['echo',       4, 4,     \&cmd_echo,	'cmd_help.pl'],
	['echoe',      5, 5,     \&cmd_echoe,	'cmd_help.pl'],
	['echon',      5, 5,     \&cmd_echon,	'cmd_help.pl'],
	['echoen',     6, 6,     \&cmd_echoen,	'cmd_help.pl'],
	['echone',     6, 6,     \&cmd_echoen,	'cmd_help.pl'],
	);

# ($type,$value)= parse_cmd($strref, $lc)
#    Given a reference to a string, return the next keyword, stripping it out
#    of the string.  Return the type and value.  Types are
#      'K'  -  keyword.
#      'Q'  -  quoted string
#      'S'  -  selector (any sequence of numbers, plus - , $ * . ^
#    If $lc is true, convert the result to lower case.

sub parse_cmd {
    my ($strref, $lc)= @_;
    $$strref =~ s/^\s*//;

    if ($$strref =~ s/^"((?:[^"\\]|\\.)*)("|$)\s*// or
        $$strref =~ s/^'((?:[^'\\]|\\.)*)('|$)\s*//)
    {
        my $str= $1;
        $str=~ s/\\n/\n/g;
        $str=~ s/\\t/\t/g;
        $str=~ s/\\f/\f/g;
        $str=~ s/\\b/\b/g;
        $str=~ s/\\e/\e/g;
        $str=~ s/\\E/\e/g;
        $str=~ s/\\r/\r/g;
        $str=~ s/\\c/\377/g;
        $str=~ s/\\([^\\])/\1/g;
        $str=~ s/\\\\/\\/g;
	$str= lc($str) if $lc;
        return ('Q',$str);
    }
    elsif ($$strref =~ s/^([0-9,\$*.^-]+)\s*//)
    {   
        return ('S',$1);
    }
    elsif ($$strref =~ s/^(\S+)//)
    {   
	my $str;
	$str= $lc ? lc($1) : $1;
        return ('K',$str);
    }
    else
    {   
        return (undef, undef);
    }
}

# ($cmd,$rest)= split_cmds($line)
#    Given a line that may contain semicolon-separated commands, return the
#    the first command, and a string containing everything after the first
#    semicolon.

sub split_cmds
{
    my ($line)= @_;

    # Spit lines without semicolons and shell escapes back real fast
    return ($line,undef) if $line !~ /;/ or $line =~ /^\s*!/;

    # Feed other lines to this horrible regular expression
    if ($line =~ /^
	(		    # The first command, which consists of...
	  (?:		   
	    [^;\\"']	        # ... characters that are not ; \ " or '
	    |
	    \\.			# ... single characters preceeded by one \
	    |
	    "(?:[^"\\]|\\.)*"	# ... double quoted strings
	    |
	    '(?:[^'\\]|\\.)*'	# ... single quoted strings
	  )*
	)
	;		  # a semi-colon
	(.*)		  # the rest of the line
	$/x)
    {
	return ($1, $2);
    }
    else
    {
	return ($line, undef);
    }
}


# do_cmd($cmd,$args,$ok,$otherargs...)
#   Given a command keyword and a argument string, look the command up in
#   the referenced table and run the appropriate function.  Any other args
#   are just passed along to the function that implements the command.

sub do_cmd {
    my ($cmd, $args, $ok, @etc)= @_;
    my ($row, $len, $type);
    my $tabref= $ok ? \@okcmd : \@rfpcmd;
    my $mask= $ok ? 1 : 8;

    print "do_cmd($cmd, $args, $ok)\n" if $debug;

    expand_cmd_macro(\$type, \$cmd, \$args, $mask);
    print "expand to '$cmd' '$args'\n" if $debug;

    if (!$ok)
    {
	# some oddball numeric rfp commands
	return rfp_range($1, '$', @etc)
	    if ($cmd =~ /^(\d+)$/);
	return rfp_range($1, $2, @etc)
	    if ($cmd =~ /^(\d+)-(\d+)$/);
	return rfp_relative($1, @etc)
	    if ($cmd =~ /^([\+\-]\d+)$/ or $cmd eq '.');
    }

    $len= length($cmd);

    foreach $row (@$tabref)
    {
	if ($len >= $row->[1] and $len <= $row->[2] and
	    substr($row->[0], 0, $len) eq $cmd)
	{
	    require $row->[4] if $row->[4];
	    return &{$row->[3]}($args, @etc);
	}
    }
    print  "I don't understand \"$cmd\" - type HELP for help\n";
    return 1;
}


# source_cmds($cmds)
#   Given a string with newline-separated commands, execute them.
#   This is for executing commands out of a file.

sub source_cmds {
    my @lines= (split /\n/, $_[0]);
    my ($line, $cmd, $type, $word, $rest);

    # set global flag telling if commands are from local or remote source
    $remote_source= $is_remote[$cmd_source] and !$sysh->{direct};

    foreach $line (@lines)
    {
	# skip blanks and comments - lines starting with #ft# are not comments
	$line=~ s/^\s"#ft#//;
    	next if $line =~ /^\s*#/ or $line =~ /^\s*$/;

	($cmd,$line)= split_cmds($line);

	if ($cmd =~ /^\s*!(.*)$/)
	{
	    # do shell escapes
	    cmd_unix($1);
	}
	else
	{
	    # parse keyword off command name
	    ($type,$word)= parse_cmd(\$cmd, 1);

	    # execute the command
	    !defined $type or do_cmd($word, $cmd, 1) or last;
	}

	# if there are more commands on the line, do them
	redo if defined $line;
    }
}


# do_ok_prompt
#   Prompt for, read, parse and execute an OK prompt command.  This keeps
#   asking for more commands until either we get EOF or a command returns
#   '0'.   Dies() on interupts, so should be called in an eval().

sub do_ok_prompt {
    my ($term,$line);

    $cmd_source= 6;
    $remote_source= 0;

    start_readline() if (flagval('readline'));

    while (1)
    {
	# Get a command
	if (!defined $line)
	{
	    my $prompt= $confh ? $defhash{prompt} : $defhash{noconfp};
	    $line= iget($prompt);
	    if (!defined($line))
	    {
		print "\n";
		last if !flagval('ignoreeof');
		print "type QUIT to leave.\n";
		next;
	    }
	}
	# Break off first of semicolon separated commands on line
	($cmd,$line)= split_cmds($line);

	if ($cmd =~ /^\s*!(.*)$/)
	{
	    # do shell escapes
	    cmd_unix($1);
	}
	else
	{
	    ($type,$word)= parse_cmd(\$cmd,1);
	    next if !defined $type;

	    do_cmd($word, $cmd, 1) or last;
	}
    }
}


# cmd_sourcefile()
#   Implements the "source" command

sub cmd_sourcefile {
    my ($args)= @_;
    my ($type, $name, $file);

    while (1)
    {
	($type, $name)= parse_cmd(\$args,1);
	$type or last;

	$file= ft_getfile($name);
	if (defined $file)
	{
	    source_cmds($file);
	}
	else
	{
	    print "Could not read file $name\n";
	}
    }
    return 1;
}


sub cmd_mail {
    my ($args)= @_;
    print "mail $args - NOT IMPLEMENTED\n";
    return 1;
}

sub cmd_chat {
    my ($args)= @_;
    if (defined $PATH_WRITE)
    {
	cmd_unix("$PATH_WRITE $args");
    }
    else
    {
	print "write command not enabled in this client\n";
    }
    return 1;
}

sub cmd_who {
    my ($args)= @_;
    if (defined $PATH_WHO)
    {
	cmd_unix("$PATH_WHO $args");
    }
    else
    {
	print "who command not enabled in this client\n";
    }
    return 1;
}

sub cmd_unix {
    my ($args)= @_;
    my $rc= system($args) & 0xffff;
    print "Command not found\n" if $rc == 0xff00 || $rc == 0xffff;
    return 1;
}


sub cmd_quit {
    return 0;
}


# inconf()
#   Check if we are in a conference.  If so, return true.  If not, print
#   an error message and return false.

sub inconf()
{
    print "Not in a conference.\n" if (!$confh);
    return $confh;
}


1;
