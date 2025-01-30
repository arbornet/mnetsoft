# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# disptab is list of things we can display.
@disptab= (
    ['forgotten',	2, 9,	\&disp_unimp],
    ['retired',		2, 7,	\&disp_unimp],
    ['frozen',		2, 6,	\&disp_unimp],
    ['new',		1, 3,	\&disp_new,	undef,	   'dsp_new'],
    ['conferences',	1, 11,	\&disp_confs,	undef,	   'dsp_conferences'],
    ['conflist',	5, 8,	\&disp_confs,	undef,	   'dsp_conflist'],
    ['thisserver',	5, 10,	\&disp_thisserv,undef,	   'dsp_thisserv'],
    ['thissystem',	6, 10,	\&disp_thisserv,undef,     'dsp_thisserv'],
    ['thishost',	6, 8,	\&disp_thisserv,undef,     'dsp_thisserv'],
    ['thisconference',	4, 14,	\&disp_cfsep,	'confmsg', 'dsp_thisconf'],
    ['systems',		2, 7,	\&disp_sys,     undef,     'dsp_servers'],
    ['servers',		3, 7,	\&disp_sys,     undef,     'dsp_servers'],
    ['hosts',		3, 5,	\&disp_sys,     undef,     'dsp_servers'],
    ['user',		1, 4,	\&disp_name,	undef,	   'set_name'],
    ['name',		2, 4,	\&disp_name,	undef,	   'set_name'],
    ['time',		2, 4,	\&disp_unimp],
    ['date',		2, 4,	\&disp_unimp],
    ['who',		1, 3,	\&disp_unimp],
    ['fws',		2, 3,	\&disp_unimp],
    ['fairwitnesses',	2, 13,	\&disp_unimp],
    ['login',		2, 5,	\&disp_log,	1,	   'set_login'],
    ['logout',		2, 6,	\&disp_log,	2,	   'set_logout'],
    ['bulletin',	2, 8,	\&disp_cfsep,	'bullmsg', 'set_bulletin'],
    ['index',		2, 5,	\&disp_cfsep,	'indxmsg', 'set_index'],
    ['agenda',		2, 5,	\&disp_cfsep,	'indxmsg', 'set_index'],
    ['logmessages',	4, 11,	\&disp_log,	3,	   'dsp_logmessages'],
    ['list',		2, 4,	\&disp_cflist,  undef,	   'set_list'],
    ['ulist',    	2, 5,	\&disp_file,	6,	   'set_ulist'],
    ['glist',    	2, 5,	\&disp_file,	7,	   'set_glist'],
    ['welcome',    	2, 7,	\&disp_file,	4,	   'set_welcome'],
    ['rc',      	2, 2,	\&disp_file,	5,	   'set_rc'],
    ['participants',	2, 12,	\&disp_unimp],
    ['definitions',	2, 11,	\&disp_unimp],
    ['seen',		2, 4, 	\&disp_unimp],
    ['size',		2, 4, 	\&disp_unimp],
    ['superuser',	2, 9, 	\&disp_unimp],
    ['fds',		2, 3, 	\&disp_unimp],
    ['versions',	1, 8, 	\&disp_version, undef,	  'dsp_version'],
    ['chat',		2, 4, 	\&disp_unimp],
    ['mesg',		2, 4, 	\&disp_unimp],
    ['write',		2, 5, 	\&disp_unimp],
    ['forget',		5, 6, 	\&disp_set,	undef,	  'set_forget'],
    ['uid',		2, 3,	\&disp_set,     undef,    'set_uid'],
    ['numbered',	3, 8,	\&disp_set,     undef,    'set_numbered'],
    ['default',		3, 7,	\&disp_set,	undef,	  'set_default'],
    ['mailtext',	5, 8,	\&disp_set],
    ['edalways',	4, 8,	\&disp_set,	undef,    'set_edalways'],
    ['dot',		3, 3,	\&disp_set],
    ['stay',		3, 4,	\&disp_set,	undef,    'set_stay'],
    ['modestay',	3, 8,	\&disp_set,	undef,    'set_modestay'],
    ['metoo',		2, 5,	\&disp_set],
    ['source',		2, 6,	\&disp_set,	undef,    'set_source'],
    ['ignoreeof',	2, 9,	\&disp_set,	undef,    'set_ignoreeof'],
    ['strip',		3, 5,	\&disp_set],
    ['readline',	5, 8,	\&disp_readline],
    );

sub cmd_display {
    my ($args)= @_;
    my ($type,$what,$row,$len);
    print "cmd_display($args)\n" if $debug;

    flag: while (1)
    {
	($type,$what)= parse_cmd(\$args,1);
	$type or last;

	# search display table
	$len= length($what);
	foreach $row (@disptab)
	{
	    #print "$what=$row->[0] ($len) ($row->[1],$row->[2])\n" if $debug;
	    if ($len >= $row->[1] and $len <= $row->[2] and
	    	substr($row->[0], 0, $len) eq $what)
	    {
	    	&{$row->[3]}($row->[0], $row->[4]);
		next flag;
	    }
	}
	print "Bad parameters near \"$what\"\n";
    }
    return 1;
}


sub disp_unimp {
    my ($what)= @_;
    print "DISPLAY ",uc($what)," not yet implemented in Fronttalk.\n";
}


sub disp_log {
    my ($what, $flag)= @_;

    print "login message:\n" if $flag == 3;
    print exec_sep($defhash{linmsg},0,$confh,1) if $flag & 1;
    print "logout message:\n" if $flag == 3;
    print exec_sep($defhash{loutmsg},0,$confh,1) if $flag & 2;
}


sub disp_new {
    print exec_sep($defhash{linmsg},0,$confh,2);
}


sub disp_version {
    print "Client Version: $VERSION\n";
    print "Server Version: $sysh->{version}\n" if $sysh->{version};
}


sub disp_cfsep {
    my ($what, $sep)= @_;
    print exec_sep($defhash{$sep},0,$confh,1);
}


sub disp_cflist {
    my $i;
    my $n= @{$sysh->{cflist}};

    if ($n == 0)
    {
    	print "No Conference List defined\n";
	return;
    }

    print "-->\n" if $sysh->{cfindex} < 0;

    for ($i= 0; $i < $n; $i++)
    {
    	print( (($i == $sysh->{cfindex}) ? '-->' : '   '), ' ',
	       $sysh->{cflist}[$i], "\n");
    }
}


sub disp_sys {
    my ($serv, $line);

    foreach $serv (@syslist)
    {
    	print "$serv->[0] ($serv->[1])\n";
	foreach $line (split /\n/, $serv->[2])
	{
	    print "  $line\n";
	}
    }
}


sub disp_confs {
    my ($conf, $des);
    require "if_scan.pl";
    my @conflist= ft_conflist();
    if ($ft_err)
    {
	print "Error: $ft_errmsg\n";
	return;
    }
    foreach $conf (@conflist)
    {
	$des= $conf->{des};
	chomp $des;
	if ($conf->{type} eq 'H')
	{
	    $des=~ s/\n/\n    /g;
	    print "\n$conf->{name}:\n";
	    print "    $des" if $des;
	}
	else
	{
	    $des=~ s/\n/\n                    /g;
	    printf "  %-15s - $des\n", $conf->{name};
	}
    }
}

sub disp_name {
    if (inconf())
    {
    	print "User: $confh->{myname}\n";
    }
}

sub disp_file {
    my ($what, $flag)= @_;
    if (inconf())
    {
	print sf_show(0, $flag);
    }
}

sub disp_thisserv {
    if (!$sysh)
    {
	print "No server\n";
	return;
    }
    print "Name:         $sysh->{name}\n";
    print "Version:      $sysh->{version}\n";
    print "Connection:   ",$sysh->{direct}?'direct':'network',' (',
    	$sysh->{secure}?'':'in', "secure)\n";
    print "Login Method: $sysh->{login}\n";
    print "Allow Anon:   ",defined($sysh->{anon}) ? 'yes' : 'no',"\n";
    print "Anon URL:     $sysh->{location}\n";
    print "Auth URL:     $sysh->{pwurl}\n";
}


1;
