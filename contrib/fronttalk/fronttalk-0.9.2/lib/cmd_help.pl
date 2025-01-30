# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# DEFINE help topics

my @helpdef= (
	['bullmsg',   7, 7,   'def_bullmsg'],
	['checkmsg',  8, 8,   'def_checkmsg'],
	['confmsg',   7, 7,   'def_conf'],
	['fsep',      4, 4,   'def_fsep'],
	['ftsep',     4, 4,   'def_fsep'],
	['indxmsg',   8, 8,   'def_indxmsg'],
	['isep',      4, 4,   'def_isep'],
	['ishort',    6, 6,   'def_ishort'],
	['joinmsg',   7, 7,   'def_joinmsg'],
	['linmsg',    6, 6,   'def_linmsg'],
	['loutmsg',   7, 7,   'def_loutmsg'],
	['nsep',      4, 4,   'def_nsep'],
	['partmsg',   7, 7,   'def_partmsg'],
	['printmsg',  8, 8,   'def_printmsg'],
	['rsep',      4, 4,   'def_rsep'],
	['txtsep',    6, 6,   'def_txtsep'],
	['zsep',      4, 4,   'def_zsep'],
	['prompt',    6, 6,   'def_prompt'],
	['noconfp',   7, 7,   'def_noconfp'],
	['rfpprompt', 9, 9,   'def_prompt'],
	['obvprompt', 9, 9,   'def_prompt'],
	['joqprompt', 9, 9,   'def_prompt'],
	['editor',    6, 6,   'def_editor'],
	['pager',     5, 5,   'def_pager'],
	);

# Top-Level Help topics
@helptopic= (
#        command   min/maxmatch  filename
	['again',        2, 5,     'again' ],
	['browse',       1, 6,     'browse' ],
	['bye',          2, 3,     'quit' ],
	['cdate',        3, 5,     'date' ],
	['change',       2, 6,     'set' ],
	['chat',         4, 4,     'write' ],
	['check',        2, 5,     'check' ],
	['connect',      2, 7,     'server' ],
	['confconditionals',5, 16, 'confcond' ],
	['conffunctions',5, 13,    'conffunc' ],
	['date',         2, 4,     'date' ],
	['define',       2, 6,     'define' ],
	['differences',  3, 11,    'differences' ],
	['display',      1, 7,     'display' ],
	['disfavor',     6, 8,     'disfavor' ],
	['echo',         4, 4,     'echo' ],
	['echoe',        5, 5,     'echo' ],
	['echoen',       6, 6,     'echo' ],
	['echon',        5, 5,     'echo' ],
	['enter',        1, 5,     'enter' ],
	['erase',        5, 5,     'erase' ],
	['exit',         2, 4,     'quit' ],
	['expurgate',    3, 9,     'hide' ],
	['favor',        3, 6,     'favor' ],
	['find',         2, 4,     'find' ],
	['forget',       1, 6,     'forget' ],
	['freeze',       2, 6,     'freeze' ],
	['header',       3, 6,     'head' ],
	['help',         1, 4,     'help' ],
	['hide',         2, 4,     'hide' ],
	['hold',         2, 4,     'hold' ],
	['introduction', 1, 12,    'intro' ],
	['item',         1, 4,     'read' ],
	['itemconditionals',5, 16, 'itemcond' ],
	['itemfunctions',5, 13,    'itemfunc' ],
	['itemrange',    5, 9,     'range' ],
	['join',         1, 4,     'join' ],
	['kill',         4, 4,     'kill' ],
	['fixseen',      3, 7,     'fixseen' ],
	['participants', 2, 12,    'part' ],
	['leave',        2, 5,     'leave' ],
	['link',         2, 4,     'link' ],
	['login',        3, 5,     'login' ],
	['mytitle',      3, 7,     'mytitle' ],
	['new',          2, 3,     'new' ],
	['next',         1, 4,     'next' ],
	['only',         1, 4,     'only' ],
	['pass',         1, 4,     'pass' ],
	['passwd',       6, 6,     'set_passwd' ],
	['postpone',     2, 8,     'hold' ],
	['preserve',     3, 8,     'hold' ],
	['print',        2, 5,     'print' ],
	['pseudonym',    2, 9,     'pseudonym' ],
	['quit',         1, 4,     'quit' ],
	['read',         1, 4,     'read' ],
	['resign',       6, 6,     'resign' ],
	['remember',     3, 8,     'remember' ],
	['ranges',       2, 6,     'range' ],
	['respond',      1, 7,     'respond' ],
	['retire',       3, 6,     'retire' ],
	['retitle',      4, 7,     'retitle' ],
	['rewind',       3, 6,     'rewind' ],
	['rp',           2, 2,     'rp' ],
	['scribble',     5, 8,     'erase' ],
	['separators',   3, 10,    'separators' ],
	['server',       3, 6,     'server' ],
	['set',          2, 3,     'set' ],
	['show',         2, 4,     'show' ],
	['stop',         2, 4,     'quit' ],
	['system',       2, 6,     'server' ],
	['thaw',         4, 4,     'thaw' ],
	['topics',       2, 6,     'topics' ],
	['unexpurgate'  ,5, 11,    'show' ],
	['unhide',       4, 6,     'show' ],
	['unix',         1, 4,     'unix' ],
	['unretire',     5, 8,     'unretire' ],
	['who',          1, 3,     'who' ],
	['write',        2, 5,     'write' ],
	);


sub cmd_help {
    my ($args,$itemh)= @_;
    my ($type,$what1,$what2,$len,$file);

    ($type,$what1)= parse_cmd(\$args,1);
    ($type,$what2)= parse_cmd(\$args,1);
    $len= length($what1);

    if ($len > 4 and $what1 eq substr('conferences',0,$len))
    {
    	# List of conferences
	require "cmd_display.pl";
	disp_confs();
	return 1;
    }

    # Default topic - depends on whether we are at OK or Respond prompt.
    if ($len == 0)
    {
    	$file= $HELPDIR . '/' . ($itemh ? 'rp' : 'intro');
    }
    else
    {
        my $i;
	for ($i= 0; $i < @helptopic; $i++)
	{
	    if ($len >= $helptopic[$i]->[1] and
	        $len <= $helptopic[$i]->[2] and
		substr($helptopic[$i]->[0],0,$len) eq $what1)
	    {
	    	$file= $helptopic[$i]->[3];
		last;
	    }
	}
	if ($what2 and
	    ($file eq 'set' or $file eq 'define' or $file eq 'display'))
	{
	    my ($array, $col);
	    if ($file eq 'set')
	    {
		$array= \@setlist;  $col= 5;
	    }
	    elsif ($file eq 'define')
	    {
	        $array= \@helpdef;  $col= 3;
	    }
	    else
	    {
		require "cmd_display.pl";
	        $array= \@disptab;  $col= 5;
	    }
	    $file= undef;

	    $len= length($what2);

	    for ($i= 0; $i < @$array; $i++)
	    {
		if ($len >= ${$array}[$i]->[1] and
		    $len <= ${$array}[$i]->[2] and
		    substr(${$array}[$i]->[0],0,$len) eq $what2)
		{
		    $file= ${$array}[$i]->[$col];
		    last;
		}
	    }
	}
	if (!$file)
	{
	    print "Unknown help topic: '$what1",($what2 ? " $what2":''),"'\n",
		  "Type \"HELP TOPICS\" for list of topics.\n";
	    return 1;
	}
	$file= "$HELPDIR/$file";

    }


    # Show help file
    if (!open HELP, $file)
    {
    	print "Help topic '$what1", ($what2 ? " $what2":''),
	      "' not found ($file).\n";
	return 1;
    }

    pager_on();
    print <HELP>;
    pager_off();

    return 1;
}


sub printargs {
    my ($fh, $args, $nl)= @_;
    my ($type, $arg);

    while (1)
    {
	($type, $arg)= parse_cmd(\$args,0);
	last if !$type;
	print $fh $arg;
    }
    print "\n" if $nl;
}

sub cmd_echo   { printargs(\*STDOUT,$_[0],1); return 1; }
sub cmd_echon  { printargs(\*STDOUT,$_[0],0); return 1; }
sub cmd_echoe  { printargs(\*STDERR,$_[0],1); return 1; }
sub cmd_echoen { printargs(\*STDERR,$_[0],0); return 1; }


1;
