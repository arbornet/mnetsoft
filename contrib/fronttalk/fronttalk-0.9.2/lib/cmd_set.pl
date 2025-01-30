# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# List of things we can set.  Current value is stored in %settings hash, not
# in this table.
@setlist= (
#   keyword     min/maxmatch  argument         function         help file
    ['name',         1,  4,     undef,         \&set_confname,  'set_name'],
    ['user',         1,  4,     undef,         \&set_confname,  'set_name'],
    ['resign',       6,  6,     undef,         \&cmd_resign,    'resign'],
    ['join',         4,  4,     undef,         \&set_join,      'set_join'],
    ['register',     3,  7,     undef,         \&set_join,	'set_join'],
    ['reload',       4,  6,    'reload',       \&set_unimp],
    ['newresponses', 6, 12,    'newresponses', \&set_unimp],
    ['saveseen',     4,  8,    'saveseen',     \&set_unimp],
    ['autosave',     4,  8,    'autosave',     \&set_nop],
    ['noautosave',   6, 10,    'noautosave',   \&set_unimp],
    ['password',     4,  8,     undef,         \&set_passwd,	'set_passwd'],
    ['passwd',       6,  6,     undef,         \&set_passwd,	'set_passwd'],
    ['forget',       1,  6,     'forget',      \&set_on,	'set_forget'],
    ['noforget',     3,  8,     'forget',      \&set_off,	'set_forget'],
    ['hide',         2,  4,     'hide',        \&set_on,	'set_hide'],
    ['nohide',       4,  6,     'hide',        \&set_off,	'set_hide'],
    ['censor',       3,  6,     'hide',        \&set_on,	'set_hide'],
    ['nocensor',     5,  8,     'hide',        \&set_off,	'set_hide'],
    ['date',         1,  4,     'date',        \&set_on,	'set_date'],
    ['nodate',       3,  6,     'date',        \&set_off,	'set_date'],
    ['uid',          2,  3,     'uid',         \&set_on,	'set_uid'],
    ['nouid',        4,  5,     'uid',         \&set_off,	'set_uid'],
    ['numbered',     3,  8,     'numbered',    \&set_on,	'set_numbered'],
    ['nonumbered',   5, 10,     'numbered',    \&set_off,	'set_numbered'],
    ['unnumbered',   5, 10,     'numbered',    \&set_off,	'set_numbered'],
    ['default',      3,  7,     'default',     \&set_on,	'set_default'],
    ['nodefault',    5,  9,     'default',     \&set_off,	'set_default'],
    ['mailtext',     5,  8,     'mailtext',    \&set_on],
    ['nomailtext',   7, 10,     'mailtext',    \&set_off],
    ['edalways',     4,  8,     'edalways',    \&set_localon,	'set_edalways'],
    ['noedalways',   6, 10,     'edalways',    \&set_localoff,	'set_edalways'],
    ['favfirst',     8,  8,     'nofavfirst',  \&set_off],
    ['nofavfirst',  10, 10,     'nofavfirst',  \&set_on],
    ['dot',          3,  3,     'dot',         \&set_on],
    ['nodot',        5,  5,     'dot',         \&set_off],
    ['stay',         3,  4,     'stay',        \&set_on,	'set_stay'],
    ['nostay',       5,  6,     'stay',        \&set_off,	'set_stay'],
    ['modestay',     7,  8,     'modestay',    \&set_on,	'set_modestay'],
    ['nomodestay',   9, 10,     'modestay',    \&set_off,	'set_modestay'],
    ['metoo',        2,  5,     'metoo',       \&set_on],
    ['nometoo',      4,  7,     'metoo',       \&set_off],
    ['source',       2,  6,     'source',      \&set_on,	'set_source'],
    ['nosource',     4,  8,     'source',      \&set_off,	'set_source'],
    ['remote',       3,  6,     1,             \&set_remote,	'set_remote'],
    ['noremote',     5,  8,     0,             \&set_remote,	'set_remote'],
    ['someremote',   7, 10,     2,             \&set_remote,	'set_remote'],
    ['ignoreeof',    2,  9,     'ignoreeof',   \&set_on,       'set_ignoreeof'],
    ['noignoreeof',  4, 11,     'ignoreeof',   \&set_off,      'set_ignoreeof'],
    ['strip',        3,  5,     'strip',       \&set_on],
    ['nostrip',      5,  6,     'strip',       \&set_off],
    ['readline',     5,  8,     1,             \&set_readline],
    ['noreadline',   7, 10,     0,             \&set_readline],
    ['mesg',         4,  4,     1,             \&set_mesg,	'set_chat'],
    ['nomesg',       6,  6,     0,             \&set_mesg,	'set_chat'],
    ['chat',         2,  4,     1,             \&set_mesg,	'set_chat'],
    ['nochat',       4,  6,     0,             \&set_mesg,	'set_chat'],
    ['write',        2,  5,     1,             \&set_mesg,	'set_chat'],
    ['nowrite',      4,  7,     0,             \&set_mesg,	'set_chat'],
    ['sane',         4,  4,      undef,        \&set_conf_dflt, 'set_sane'],
    ['supersane',    9,  9,      undef,        \&set_supersane, 'set_sane'],
    ['list',         4,  4,      'cflist',     \&set_file,	'set_list'],
    ['cfonce',       6,  6,      'cfonce',     \&set_file,	'set_cfonce'],
    ['cfrc',         4,  4,      'cfrc',       \&set_file,	'set_cfrc'],
    ['bulletin',     4,  8,      'bull',       \&set_file_conf, 'set_bulletin'],
    ['index',        5,  5,      'index',      \&set_file_conf, 'set_index'],
    ['agenda',       2,  5,      'index',      \&set_file_conf, 'set_index'],
    ['login',        5,  5,      'login',      \&set_file_conf, 'set_login'],
    ['logout',       6,  6,      'logout',     \&set_file_conf, 'set_logout'],
    ['ulist',        5,  5,      'ulist',      \&set_file_conf, 'set_ulist'],
    ['allowed',      5,  7,      'ulist',      \&set_file_conf, 'set_ulist'],
    ['glist',        5,  5,      'glist',      \&set_file_conf, 'set_ulist'],
    ['rc',           2,  2,      'confrc',     \&set_file_conf, 'set_rc'],
    ['welcome',      7,  7,      'welcome',    \&set_file_conf, 'set_welcome'],
    ['debug',        3,  5,      1,            \&set_debug,	'set_debug'],
    ['nodebug',      5,  7,      0,            \&set_debug,	'set_debug'],
    );


# flagval($name)
#    Return the current value of the named flag within the current
#    conference.

sub flagval {
    my ($flag)= @_;
    defined $settings[1]->{$flag} and
	return $settings[1]->{$flag};
    return $settings[0]->{$flag};
}


# set_sys_dflt()
#   Set system settings to their builtin defaults.  This is called before
#   loading the system rc file and the .cfonce file.

sub set_sys_dflt {
    $settings[0]= {
    	date => 1,
	forget => 1,
	hide => 1,
	uid => 0,
	numbered => 0,
	default => 1,
	mailtext => 0,
	edalways => 0,
	dot => 1,
	stay => 0,
	modestay => 1,
	metoo => 1,
	source => 1,
	ignoreeof => 0,
	strip => 1,
	readline => 1,
	remote => 2,
	};
}


# set_supersane()

sub set_supersane {
    if ($cmd_source == 3)
    {
	print "Cannot set supersane from conference rc files\n";
	return 1;
    }

    my $oldsource= $settings[0]->{source};
    set_sys_dflt();
    $settings[0]->{source}= $oldsource;
    undefine_mask(192);
    return 1;
}


# set_conf_dflt()
#  erase the settings for the current conference, so that system defaults rule.

sub set_conf_dflt {
    $settings[1]= {};
    undefine_mask(128);
    return 1;
}


# do_set($lvl, $keyword);
#   Given a set keyword, set it.  Level is 0 if it is the system RC file
#   or the user's .cfonce file, and 1 if it is the conference rc file or
#   the user's .cfrc file.

sub do_set {
    my ($lvl, $key)= @_;
    my ($row, $len);
    print "do_set($lvl, $key)\n" if $debug;

    $len= length($key);

    foreach $row (@setlist)
    {
	if ($len >= $row->[1] and $len <= $row->[2] and
	    substr($row->[0], 0, $len) eq $key)
	{
	    return &{$row->[4]}($lvl, $row->[3]);
	}
    }
    print  "Bad parameters near \"$key\"\n";
}

# do nothing
sub set_nop { return 1; }

# Turn flags on and off

sub set_on {
    my ($lvl, $flag)= @_;

    $settings[$lvl]->{$flag}= 1;
    return 1;
}

sub set_off {
    my ($lvl, $flag)= @_;

    $settings[$lvl]->{$flag}= 0;
    return 1;
}

# Turn flag on and off except maybe if the command is from a non-local source

sub set_localon {
    my ($lvl, $flag)= @_;

    $settings[$lvl]->{$flag}= 1 if !$remote_source or flagval('remote') == 1;
    return 1;
}

sub set_localoff {
    my ($lvl, $flag)= @_;

    $settings[$lvl]->{$flag}= 0 if !$remote_source or flagval('remote') == 1;
    return 1;
}


sub set_remmote {
    my ($lvl, $flag)= @_;

    $settings[$lvl]->{remote}= $flag;
    return ($flag != 0 or !$is_remote[$cmd_source]);
}


sub set_readline {
    my ($lvl, $flag)= @_;

    print "set_readline($lvl, $flag)\n" if $debug;

    # This flag is really only used to keep track of the state while the
    # system is initializing, so that if it defaults on, but is turned off
    # by an rc file, then we won't ever even try to load the Term::ReadLine
    # library.  Once we are up and running, the state of readline is tracked
    # by the $term variable in readline.pl, not by this.
    $settings[$lvl]->{readline}= $flag;

    if ($flag)
    {
	start_readline() or
	    print "Perl Term::ReadLine module not found on this system\n";
    }
    else
    {
	stop_readline();
    }
    return 1;
}


sub set_unimp {
    # Hey, some of this Picospan stuff makes no sense for Backtalk
    my ($lvl, $flag)= @_;
    print "SET ",uc($flag)," ignored in Fronttalk.\n";
    return 1;
}


sub set_passwd {
    my ($lvl, $flag)= @_;
    require "cmd_passwd.pl";
    return cmd_passwd();
}

# Get to this later
sub set_join {
    my ($lvl, $flag)= @_;
    print "set join not yet implemented\n";
    return 1;
}


sub set_mesg {
    my ($lvl, $flag)= @_;
    if (defined $PATH_MESG)
    {
	cmd_unix($flag ? "$PATH_MESG y" : "$PATH_MESG n");
    }
    else
    {
	print "Write permission setting not enabled in this client\n";
    }
    return 1;
}


sub set_confname {
    if ($sysh->{amanon})
    {
	print "Must be logged in to change your name.\n";
    }
    if (!$confh)
    {
    	print "Cannot change name - not in a conference.\n";
	return 1;
    }
    print "Your old name: $confh->{myname}\n",
    	  "Enter replacement or return to keep old? ";
    my $newname= <STDIN>;
    chomp($newname);
    return 1 if $newname =~ /^\s*$/;
    require "if_edit.pl";
    my $err= ft_changename($newname,$confh);
    if ($err)
    {
	print "Name change failed: $err\n";
    }
    else
    {
	$confh->{myname}= $newname;
	print "Name change succeeded.\n";
    }
    return 1;
}


sub set_debug {
    my ($lvl, $flag)= @_;
    $debug= $flag;
    return 1;
}

sub disp_set {
    my ($flag)= @_;
    print "$flag flag is ",(flagval($flag) ? 'on' : 'off'),"\n";
    return 1;
}

sub set_file {
    my ($lvl, $flag)= @_;
    require "cmd_enter.pl";
    require "if_edit.pl";
    if (!defined $sysh)
    {
    	print "Not connected to a server.\n";
	return 1;
    }
    my $old= ($flag eq 'cflist' ?
    	join("\n",@{$sysh->{cflist}})."\n" : $sysh->{$flag});
    my $text= get_text($old);
    if (!defined $text)
    {
    	print "Update canceled.\n";
	return 1;
    }
    my $err= ft_changefile($flag, $text);
    if ($err)
    {
	print "Update failed: $err\n";
    }
    else
    {
	$sysh->{$flag}= ($flag eq 'cflist') ? [split /[\s,]+/, $text] : $text;
	print "Update succeeded.\n";
    }
    return 1;
}

sub set_file_conf {
    my ($lvl, $flag)= @_;
    require "cmd_enter.pl";
    require "if_edit.pl";
    require "if_conf.pl";
    if (!defined $confh)
    {
    	print "No current conference.\n";
	return 1;
    }
    if (!defined $confh->{$flag})
    {
	ft_getconf($confh,$flag);
	if ($ft_err)
	{
	    print "Could not get old file contents: $ft_errmsg\n";
	    return 1;
	}
    }
    my $text= get_text($confh->{$flag});
    if (!defined $text)
    {
    	print "Update canceled.\n";
	return 1;
    }
    my $err= ft_changefile($flag, $text, $confh);
    if ($err)
    {
	print "Update failed: $err\n";
    }
    else
    {
	ft_updateconf($confh);
	print "Update succeeded.\n";
    }
    return 1;
}

sub cmd_set {
    my ($args, @etc)= @_;
    my ($type, $flag, $rc);

    while (1)
    {
    	($type, $flag)= parse_cmd(\$args,1);
	last if !$type;
	print "cmd_set doing $flag - remainder $args\n" if $debug;
	do_set($cmd_source > 2, $flag, @etc)
	    or return 0;
    }
    return 1;
}

1;
