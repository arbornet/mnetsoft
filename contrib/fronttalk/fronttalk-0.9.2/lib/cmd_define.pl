# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# deftab is our table of defines.  It's an array of arrays, with each inner
# array having the following columns
#
#  0:  name
#  1:  minmatch  -  minimum number of letters that must be given.
#  2:  maxmatch  -  length of name
#  3:  defmask   -  default mask value (undef if there is no default)
#  4:  defvalue  -  default value (undef if there is no default)
#  5:  func      -  if defined, call this with (name,mask,value) when changed
#  6:  curmask   -  current mask, if default value has been changed
#  7:  curvalue  -  current value, if default value has been changed.
#
#  Flag bits in mask values are:
#    1  OK prompt macro
#    2  variable
#    4  item ranges
#    8  RFP prompt macro
#   16
#   32
#   64  things set in system rc file
#  128  things set in conf rc file
#  256  environment variables
#  512  things not to print when listing definitions
# 1024  things set by remote rc files
#
# We initialize the array with the compiled in default values:
 
my @deftab= (
    ['bullmsg',   7, 7,   2, '%(1x%3g%)%c', \&defmsg],
    ['checkmsg',  8, 8,   2, '%(1x\nnew resp items  conference\n%)%(2x%(k--%E  %)%(B>%E %)%4r %4b %s%(B  (where you currently are!)%)%)', \&defmsg],
    ['confmsg',   7, 7,   2, 'Conference: %d (%s)\nServer: %h (%H)\nParticipation file: %p\nConference type: %t (%D)\n%(i%i item%S\nFirst item %f, last %l%ENo items yet.%)', \&defmsg],
    ['fsep',      4, 4,   2, '%(Ritem %i response %r (%l)%(L:\n%)%)%(L%7N: %L%)' , \&defsep],
    ['ftsep',     5, 5,   2, 'item %i (%l) title:\n         %h' , \&defsep],
    ['indxmsg',   8, 8,   2, '%(1x%2g%)%c', \&defmsg],
    ['isep',      4, 4,   2, '\nItem %i entered %d by %a%(U uid %u%)\n %h' , \&defsep],
    ['ishort',    6, 6,   2, '%(B\nitem nresp header\n\n%)%3i %3n %h' , \&defsep],
    ['joinmsg',   7, 7,   2, '%QFile is: %d (%s)', \&defmsg],
    ['linmsg',    6, 6,   2, '%(1x%(2x\n%)%0g%)%(2x%(3N%3g%)%(n%(r%r newresponse item%S%)%(b%(r and %)%b brandnew item%S%)\n%)%(iFirst item %f, last %l\n%)%(sYou are a fair-witness in this conference.\n%)%(lYou are an observer of this conference.\n%)%(jYour name is \"%u\" in this conference.\n%(~O\n>>>> New users: type HELP for help.\n%)%)%)%c', \&defmsg],
    ['loutmsg',   7, 7,   2, '%(1x%1g%)%c', \&defmsg],
    ['mailmsg',   7, 7,   2, 'You have %(2xmore %)mail.\n%c', \&defmsg],
    ['nsep',      4, 4,   2, '\n%(N%r new of %)%zn response%S total.\n', \&defsep],
    ['partmsg',   7, 7,   2, '%(2x%10v %o %u%)%(4x\n%k participant%S total.%)%(1x\n   loginid        last time on      name\n%)', \&defmsg],
    ['printmsg',  8, 8,   2, '%QPrinted from %H conference %s on %N', \&defmsg],
    ['rsep',      4, 4,   2, '#%r %a:', \&defsep],
    ['txtsep',    6, 6,   2, '%(L%(F%7N:%)%X%L%)', \&defsep],
    ['zsep',      4, 4,   2, '%(T\f%)%c', \&defsep],
    ['all',       3, 3, 516, '*'],
    ['first',     3, 5, 524, '^'],
    ['last',      3, 4, 524, '$'],
    ['this',      3, 4, 516, '.'],
    ['next',      3, 4, 516, '>'],
    ['previous',  3, 8, 516, '<'],
    ['text',      3, 4, 520, '0'],
    ['current',   3, 7, 520, '.'],
    ['^',         1, 1, 520, '1'],
    ['prompt',    6, 6,   2, "\nOk: ", \&defprompt],
    ['noconfp',   7, 7,   2, "\n(no conf): ", \&defprompt],
    ['rfpprompt', 9, 9,   2, "\nRespond or pass? ", \&defprompt],
    ['obvprompt', 9, 9,   2, "\nResponse not possible.  Pass? ", \&defprompt],
    ['joqprompt', 9, 9,   2, "\nJoin, quit or help? ", \&defprompt],
    ['text',      4, 4,   2, '>', \&defprompt],
    ['escape',    6, 6,   2, ':', \&defprompt],
    ['gecos',     5, 5,   2, '', \&defprompt],
    ['editor',    6, 6,   2, $DEFAULT_EDITOR, \&defprompt],
    ['pager',     5, 5,   2, $DEFAULT_PAGER, \&defprompt],
    );

# This is the number of table entries which are system defines, basically the
# compiled-in defines with compiled in defaults.  Later @deftab will grow as
# more defines are created, but the first $nsysdef will always be there.
my $nsysdef= scalar @deftab;


# defsep($name, $value)
# defmsg($name, $value)
#   These are called when system variables are redefined.  They compile the
#   new value and save the result in the %defhash hash.

sub defsep {
    my ($name, $value)= @_;
    print "DEFSET($name,$value)\n" if $debug;

    # Fix-up certain values
    if ($name eq 'rsep')
    {
    	$value.= "%(U uid %u%)" if $value !~ /%\d*\.?\d*u/;
    	$value.= "%(D on %0d%)" if $value !~ /%\d*\.?\d*[dt]/;
    	$value.= "%(V   <hidden>%)%(W   <erased>%)"
	    if $value !~ /%\([VW]/;
    	print "rsep=$value\n" if $debug;
    }
    if ($name eq 'isep' or $name eq 'ishort')
    {
	$value.= "%(M\\n   <item is linked>%)" if $value !~ /%\(M/;
	$value.= "%(X\\n   <item is retired>%)" if $value !~ /%\(X/;
	$value.= "%(Y\\n   <item is forgotten>%)" if $value !~ /%\(V/;
	$value.= "%(Z\\n   <item is frozen>%)" if $value !~ /%\(Z/;
	$value.= "%(Q\\n   <item is a favorite>%)" if $value !~ /%\(Q/;
    	$value.= "%(V\\n   <hidden>%)%(W\n   <erased>%)"
	    if $value !~ /%\([VW]/;
    	print "$name=$value\n" if $debug;
    }

    print "COMPILING $value\n" if $debug;
    $defhash{$name}= compile_sep($value, 1);
}

sub defmsg {
    my ($name, $value)= @_;
    $defhash{$name}= compile_sep($value, 0);
}

sub defprompt {
    my ($name, $value)= @_;
    $defhash{$name}= $value;
}


# init_def()
#   After we are done loading all the source files, we should compile any
#   uncompiled separators.  Note that we only compile default value, as
#   any changed value will have been compiled when it was changed.

sub init_def {
    my ($def, $val, $i);

    for ($i= 0; $i < $nsysdef; $i++)
    {
	$def= $deftab[$i];

	defined $def->[7] or !defined $def->[5] or
	    &{$def->[5]}($def->[0], $def->[4]);
    }
}


# maskandval($row)
#   Given an index into the deftab table, return the ($mask, $value) for
#   that.

sub maskandval
{
    my ($row)= @_;

    return defined($row->[7]) ?
	($row->[6], $row->[7]) :
	($row->[3], $row->[4]);
}


# defval($name, $mask)
#   Return the value of the named define.  If mask is defined, return only
#   values matching the mask.

sub defval {
    my ($name, $mask)= @_;
    my ($i, $len);
    $len= length($name);

    for ($i= 0; $i < @deftab; $i++)
    {
    	if ($len >= $deftab[$i]->[1] and $len <= $deftab[$i]->[2] and
	    substr($deftab[$i]->[0],0,$len) eq $name)
	{
	    my ($thismask, $thisval)= maskandval($deftab[$i]);
	    return (!$mask or ($mask & $thismask)) ?  $thisval : undef;
	}
    }
    return undef;
}


# new_define($name, $mask, $value)
#   Store a new value for a definition

sub new_define {
    my ($name, $mask, $value)= @_;
    my ($i, $minlen, $maxlen, $minname);

    $mask&= ~1216;
    $mask|= 64 if $cmd_source == 0;
    $mask|= 128 if $cmd_source == 3;
    $mask|= 1024 if $remote_source;

    # Remove any underscores, figuring out min and max lengths.
    $minlen= $maxlen= length($name);
    for ($i= 1; $i < $maxlen; $i++)
    {
    	if (substr($name,$i,1) eq '_')
	{
	    $minlen= $i;
	    $name=~ s/_//g;
	    $maxlen= length($name);
	    last;
	}
    }

    # Things we don't do when 'someremote' is set
    return if (flagval('remote') == 2 and $remote_source and
	       ($name eq 'editor' or $name eq 'pager' or
		(($mask & 10) and $value =~ /^\s*unix\s/)));

    # Set environment variables in environment as well as in deftab
    $ENV{uc($name)}= $value if $mask & 256;
   
    # Is there already such a define?
    $minname= substr($name,0,$minlen);
    for ($i= 0; $i < @deftab; $i++)
    {
    	if ($minlen >= $deftab[$i]->[1] and $minlen <= $deftab[$i]->[2] and
	    substr($deftab[$i]->[0],0,$minlen) eq $minname)
	{
	    if ($name eq $deftab[$i]->[0])
	    {
		# conference RC may not redefine previously existing defines.
		return if $cmd_source == 3 and defined $deftab[$i]->[7];

	    	$deftab[$i]->[6]= $mask;
	    	$deftab[$i]->[7]= $value;
		&{$deftab[$i]->[5]}($name,$value) if $deftab[$i]->[5];
		return;
	    }
	    else
	    {
	    	print "Define $name conflicts with previous define $deftab[$i]->[0]\n";
		return;
	    }
	}
    }

    # Add to definition table - note there are is no default value here.
    push @deftab, [$name, $minlen, $maxlen, undef, undef, undef, $mask, $value];
}


# undeftab($i)
#   Undefine $deftab[$i].  This means revert to the compiled in value, if
#   there is one, or delete the row, if there is none.

sub undeftab
{
    my ($i)= @_;
    my $name= $deftab[$i]->[0];

    if (!defined $deftab[$i]->[4])
    {
	# There was no default value, discard whole table row
	delete $ENV{uc($name)} if $deftab[$i]->[6] & 256;
	$defhash{$name}= undef;
	splice(@deftab, $i, 1);
    }
    elsif (defined $deftab[$i]->[7])
    {
	# There was a default value, revert to that
	delete $ENV{un($name)} if $deftab[$i]->[6] & 256;
	$ENV{uc($name)}= $deftab[$i]->[4] if $deftab[$i]->[3] & 256;
	$deftab[$i]->[6]= $deftab[$i]->[7]= undef;
	&{$deftab[$i]->[5]}($name,$deftab[$i]->[4]) if $deftab[$i]->[5];
    }
}


# undefine_one($name)
#   undefine a defined value

sub undefine_one {
    my ($name)= @_;
    my ($i, $len);

    $name=~ s/_//g;
    $len= @deftab;
    for ($i= 0; $i < $len; $i++)
    {
        if ($deftab[$i]->[0] eq $name)
        {
	    undeftab($i);
            return;
        }
    }
    print "$name not found\n.";
}


# undefine_mask($mask)
#   Undefine all values whose mask intersects with the given mask.
#   In particular, undefine_mask(128) erases all conference rc defines,
#   and undefine_mask(64) erases all system rc defines.

sub undefine_mask {
    my ($mask)= @_;
    my ($i, $thismask);

    for ($i= 0; $i < @deftab; $i++)
    {
	# if there is not a non-default value defined, can't undefine it nohow
	defined $deftab[$i]->[7] or next;

	# undefine it if the mask matchs.
	undeftab($i) if ($deftab[$i]->[6] & $mask);
    }
}


# show_defines()
#    list definitions

sub show_defines {
    my ($row);

    print "What       Is Short For\n";
    foreach $row (@deftab)
    {
	my ($mask,$val)= maskandval($row);
	next if $mask&512;
	$val=~ s/\n/^J/g;
	$val=~ s/\r/^M/g;
        printf("%-11s%3d %s\n", $row->[0], $mask, $val);
    }
}


# show_def($name)
#   Show definition of one keyword

sub show_def {
    my ($name)= @_;
    my ($i, $len);

    $len= length($name);

    for ($i= 0; $i < @deftab; $i++)
    {
    	if ($len >= $deftab[$i]->[1] and $len <= $deftab[$i]->[2] and
	    substr($deftab[$i]->[0],0,$len) eq $name)
	{
	    my ($mask,$val)= maskandval($deftab[$i]);
	    $val=~ s/\n/^J/g;
	    $val=~ s/\r/^M/g;
	    $val=~ s/"/\\"/g;
	    printf("%s=\"%s\" (mask=%d)\n", $deftab[$i]->[0], $val, $mask);
	    return;
        }
    }
    print "$name not defined\n.";
}


# expand_cmd_macro($typeref, $cmdref,$argsref,$ok)
#   Given a command keyword and it's argument string, check if it is a macro.
#   If not, do nothing.  If so, do command macro expansion, and return
#   the new command and arguments.  Mask should be
#      1 for OK prompt commands.
#      4 for range argument macros.
#      8 for rfp prompt commands.

sub expand_cmd_macro {
    my ($typeref, $cmdref, $argsref, $mask)= @_;
    my ($row, $len, $thismask, $thisval);

    RERUN: while (1)
    {
	$len= length $$cmdref;
	foreach $row (@deftab)
	{
	    ($thismask, $thisval)= maskandval($row);

	    if (($mask & $thismask) and
	        $len >= $row->[1] and $len <= $row->[2] and
		substr($row->[0], 0, $len) eq $$cmdref)
	    {
		$$argsref= $thisval.' '.$$argsref;
		($$typeref,$$cmdref)= parse_cmd($argsref,1);
		defined $$typeref or return;
		next RERUN;
	    }
	}
	return;
    }
}

sub cmd_define {
    my ($args)= @_;
    my ($type, $name, $mask, $value);
    ($type, $name)= parse_cmd(\$args,1);
    if (!defined $type)
    {
	show_defines();
	return 1;
    }
    ($type, $mask)= parse_cmd(\$args,0);
    if (!defined $type)
    {
	show_def($name);
	# undefine_one($name);
	return 1;
    }
    if ($type eq 'S')
    {
    	($type, $value)= parse_cmd(\$args,0);
	if (defined $type)
	{
	    new_define($name,$mask,$value);
	    return 1;
	}
    }
    new_define($name,2,$mask);
    return 1;
}

sub cmd_undef {
    my ($args)= @_;
    my ($type, $name);

    if ($cmd_source == 3)
    {
	print "Cannot undefine values from conference rc file\n";
	return 1;
    }

    ($type, $name)= parse_cmd(\$args,1);
    if (!defined $type)
    {
	# undefine everything
	undefine_mask(32767);
	return 1;
    }
    if ($name =~ /^\d+$/)
    {
	# undefine everything matching mask
	undefine_mask($name);
	return 1;
    }
    while ($type)
    {
    	undefine_one($name);
	($type, $name)= parse_cmd(\$args,1);
    }
    return 1;
}

1;
