# Copyright 2001, Jan D. Wolter, All Rights Reserved.
#
#  Separator compilation and evaluation.  Kinda slick for a Perl program.

%item_func= (
    K  => "sf_unimp",
    L  => "sf_line",
    N  => "sf_lnum",
    Q  => "sf_unimp",
    S  => "sf_plural",
    T  => "sf_setlead",
    X  => "sf_printlead",
    a  => "sf_authname",
    d  => "sf_date1",
    h  => "sf_title",
    i  => "sf_ino",
    k  => "sf_rlenk",
    l  => "sf_authid",
    n  => "sf_maxresp",
    q  => "sf_rlen",
    r  => "sf_rno",
    s  => "sf_rline",
    t  => "sf_date0",
    u  => "sf_authuid",
    );

%conf_func= (
    A=> "sf_confalias",
    D=> "sf_conftypetext",
    H=> "sf_hostname",
    N=> "sf_now",
    Q=> "sf_fail",
    S=> "sf_plural",
    T=> "sf_setlead",
    X=> "sf_printlead",
    b=> "sf_brandnew",
    c=> "sf_nonl",
    d=> "sf_unimp",
    g=> "sf_show",
    f=> "sf_first",
    h=> "sf_hostalias",
    i=> "sf_items",
    k=> "sf_total",
    l=> "sf_last",
    n=> "sf_new",
    o=> "sf_lastdate",
    p=> "sf_partfile",
    d=> "sf_confalias",
    r=> "sf_newresp",
    s=> "sf_confname",
    t=> "sf_conftype",
    u=> "sf_myname",
    v=> "sf_myid",
    w=> "sf_mydir",
    y=> "sf_unseen",
    );

%item_cond= (
    '(' => '(',
    ')' => ')',
    '|' => ' or ',
    '&' => ' and ',
    '!' => '!',
    '~' => '!',				# undocumented alternate not
     p  => '$was_plural',
     B  => 'sc_once()',
     D  => '$rdflg->{date}',
     I  => 'sc_diff(0)',
     L  => 'defined($total)',
     R  => 'sc_diff(1)',
     S  => '0',				# don't implement party items
     F  => '$rdflg->{num}',
     O  => 'sc_diff(0)',
     N  => '($rdflg->{new} and $itemh->{firstresp} <= $itemh->{maxresp})',
     T  => '$rdflg->{ff}',
     U  => '$rdflg->{uid}',
     V  => '$resph->{hidden}',
     W  => '$resph->{erased}',
     Q  => '$itemh->{favorite}',
     M  => '$itemh->{linked}',
     X  => '$itemh->{retired}',
     Y  => '$itemh->{forgotten}',
     Z  => '$itemh->{frozen}',
    );

%conf_cond= (
    '('  => '(',
    ')'  => ')',
    '|'  => ' or ',
    '&'  => ' and ',
    '!'  => '!',
    '~'  => '!',			# undocumented alternate not
     b   => '($confh->{newi} > 0)',
     f   => '1',			# sum file exists - assume always
     i   => '($confh->{items} > 0)',
     j   => '($confh->{lastin} == 0 and !$sysh->{amanon})',
     l   => '(!$confh->{sysh}->{username})',
     m   => '0',			# you have new mail - not implemented
     n   => '($confh->{newr} + $confh->{newi} > 0)',
     p   => '$was_plural',
     r   => '($confh->{newr} > 0)',
     s   => '$confh->{amfw}',
     y   => '($confh->{unseen} > 0)',
     O   => '0',			# not yet implemented
     C   => '1',			# fronttalk always in a conference
    '0N' => '($confh->{logindate} > $confh->{lastin})',
    '1N' => '($confh->{logoutdate} > $confh->{lastin})',
    '2N' => '($confh->{indexdate} > $confh->{lastin})',
    '3N' => '($confh->{bulldate} > $confh->{lastin})',
    '4N' => '($confh->{welcomedate} > $confh->{lastin})',
    '5N' => '($confh->{rcdate} > $confh->{lastin})',
    '6N' => '($confh->{ulistdate} > $confh->{lastin})',
    '7N' => '($confh->{glistdate} > $confh->{lastin})',
    '0F' => '$confh->{logindate}',
    '1F' => '$confh->{logoutdate}',
    '2F' => '$confh->{indexdate}',
    '3F' => '$confh->{bulldate}',
    '4F' => '$confh->{welcomedate}',
    '5F' => '$confh->{rcdate}',
    '6F' => '$confh->{ulistdate}',
    '7F' => '$confh->{glistdate}',
    '0x' => '0',
    '1x' => '($flagbits&1)',
    '2x' => '($flagbits&2)',
    '3x' => '($flagbits&3)',
    '4x' => '($flagbits&4)',
    '5x' => '($flagbits&5)',
    '6x' => '($flagbits&6)',
    '7x' => '($flagbits&7)',
    );

@fileno= ('login', 'logout', 'index', 'bull', 'welcome', 'confrc',
	  'ulist', 'glist');

# compile_sep($str,$is_item)
#  Given a separator string, compile it into a string that, if evaluated with
#  Perl's "eval" function evaluates to an array of elements suitable for
#  passing to "print" or "join".

sub compile_sep {
    my ($str, $isitem)= @_;
    my $functab= $isitem ? \%item_func : \%conf_func;
    my $condtab= $isitem ? \%item_cond : \%conf_cond;
    my ($c,@stack,$st);
    my $comma= '';

    while ($str)
    {
	print "LOOP ***>$str<***\n" if $debug{sep};
     	if ($str=~ s/^([^%\\]+)//)
	{
	    # plain old text
	    print "plain --->$1<---\n" if $debug{sep};
	    $c.= "$comma'$1'";
	    $comma= ',';
	}
	elsif ($str=~ s/^%%//)
	{
	    # a double %%
	    print "percent\n" if $debug{sep};
	    $c.= "$comma'%'";
	    $comma= ',';
	}
	elsif ($str=~ s/^\\(.)//)
	{
	    # backslashed character code
	    print "backslash --->\\$1<---\n" if $debug{sep};
	    my $char= $1;
	    if ($char =~ /[nrtbafeE]/)
	    {
	    	$c.= "$comma\"\\" . lc($char) . '"';
	    }
	    elsif ($char eq 'c')
	    {
		$c.= "$comma sf_nonl()";
	    }
	    else
	    {
	    	$c.= "$comma'$char'";
	    }
	    $comma= ',';
	}
	elsif ($str=~ s/^%\((\d*)c//)
	{
	    # the "count" pseudo-conditional
	    print "count n=$1\n" if $debug{sep};
	    my $count= ($1 eq '') or $1;
	    $c.="$comma(sc_repeat($count,";
	    $comma= ',';
	    push @stack, 1;  # don't need an else
	}
	elsif ($str=~ s/^%\(([!~]?\d*\D)//)
	{
	    # a normal conditional
	    my $cond= $1;

	    # collect up the rest of a parenthesized conditional
	    if ($cond eq '(' or $cond eq '!(' or $cond eq '~(')
	    {
		my $n= 1;
		while ($str=~ s/^(.)//)
		{
		    $cond.= $1;
		    if ($1 eq '(')
		    {
		     	$n++;
		    }
		    elsif ($1 eq ')')
		    {
		        last if (--$n);
		    }
		}
	    }
	    print "cond --->$cond<---\n" if $debug{sep};

	    # convert the conditional to Perl
	    $cond= join('', map {$condtab->{$_}} ($cond =~ /\d*\D/g));

	    $c.= "$comma($cond?(";
	    $comma= '';

	    push @stack, 0;  # need an else
	}
	elsif ($str=~ s/^%[e|E]//)
	{
	    # else
	    print "else\n" if $debug{sep};
	    next if pop(@stack);	# already have else?
	    $c.= "):(";
	    $comma= '';
	    push @stack, 1;  # need no more elses
	}
	elsif ($str=~ s/^%\)//)
	{
	    # end conditional
	    print "endcond\n" if $debug{sep};
	    $st= pop(@stack);
	    next if !defined $st;
	    $c.= "):(" if !$st;
	    $c.= '))';
	    $comma= ',';
	}
	elsif ($str=~ s/^%([zZ]?)(-?\d*)(?:\.(\d+))?(.)//)
	{
	    my $code= $4;
	    my $arg;
	    print "func zflag=$1 width=$2 prec=$3 code=$code\n" if $debug{sep};
	    $arg.= $1 ? "'$1'" : 'undef' if $1 or ($2 ne '') or ($3 ne '');
	    $arg.= ($2 ne '') ? ",$2" : ',undef' if ($2 ne '') or ($3 ne '');
	    $arg.= ",$3" if $3 ne '';

	    if ($code eq '<' and $str=~ s/^(.*)>//)
	    {
		# a %<name> - lookup name
		$c.= "$comma sf_def('$1'";
		$c.= ','.$arg if defined $arg;
		$c.= ')';
		$comma= ',';
	    }
	    elsif (defined $functab->{$code})
	    {
	    	# a normal function
		$c.= $comma . $functab->{$code} . "($arg)";
		$comma= ',';
	    }
	}
    }
    # close unclosed conditionals
    while (defined ($st= pop(@stack)))
    {
    	$c.= ($st ? '))' : ":''))");
	$comma= ',';
    }
    $c.= "$comma(\$finalnl?\"\\n\":'')";
    return "($c)";
}

# We use some globals during evaluation of a separator:

local $was_plural;	# was last function plural?
local $leadin;		# current number of leadin spaces
local $isitem;		# are we evaluating an item separator or a confsep?
local $functab;		# reference to %item_func or %conf_func
local $condtab;		# reference to %item_cond or %conf_cond
local $finalnl;		# should we end with a newline?
local $flagbits;	# flags passed into exec_sep() to test with %(#x
local $total;		# various running totals.
local $confh;
local $itemh;
local $resph;
local $rdflg;

# exec_sep( <compiled conference separator>, 0, <conference handle>,<flags>,
#            <total>)
#
# exec_sep( <compiled item separator>, 1, <item handle>, <response handle>,
#           <readflag hashref>, <linenumber>, <line>)
#
#   Evaluate a conference or item separator previously compiled.

sub exec_sep {
    my ($t,$out,$n,$i,@txt);
    my $c= shift;
    my $isit= shift;
    if ($isit)
    {
	($itemh, $resph, $rdflg, $flagbits, $total)= @_;
    }
    else
    {
	($confh, $flagbits, $total)= @_;
    }
    
    # Set up some globals for this scan
    $isitem= $isit;
    $functab= $isit ? \%item_func : \%conf_func;
    $condtab= $isit ? \%item_cond : \%conf_cond;

    $was_plural= 0;
    $leadin= 1;
    $finalnl= 1;

    # do it - pass errors on along
    @txt= eval $c;
    die($@) if $@;
    return @txt;
}


# sepjust ($zflag, $width, $precission, $value)
#   Do formatting of a value.  If $zflag is 'z' or 'Z' change zero values to
#   'no' or 'No'.  If precission is defined and the string is longer than that,
#   truncate the string to that length.  If the string is shorter than the
#   absolute value  of the $width, pad it with space up to that length.  If
#   $width is negative, right justify.  Also set the plural flag if the value
#   is numeric.

sub sepjust {
    my ($zflag, $wid, $prec, $val)= @_;
    my ($len,$width);
    $was_plural= ($val != 1) if $val =~ /^\d+$/;
    $val= (($zflag eq 'z') ? 'no' : 'No') if ($zflag && $val eq '0');
    $val= substr($val,0,$prec) if defined $prec;
    $width= ($wid < 0) ? -$wid : $wid;
    if (($len= length($val)) < $width)
    {
    	if ($wid > 0)
	{
	    $val= (' ' x ($width - $len)) . $val;
	}
	else
	{
	    $val.= (' ' x ($width - $len));
	}
    }
    return $val;
}

# sepdate ($fmt, $time)
#  Format a time passed in as a unix integer time.

sub sepdate {
    my ($fmt, $tm)= @_;
    my ($dname,$mname);

    ($fmt <= 0 or $fmt > 15) and
	return (scalar localtime($tm));
    
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst)= localtime($tm);
    ($fmt < 3) and
	$dname= (Sun,Mon,Tue,Wed,Thu,Fri,Sat)[$wday];
    ($fmt < 4) and
	$mname= (Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec)[$mon];

    ($fmt == 1) and
        return sprintf("$dname, $mname %2d, %4d (%02d:%02d)",
	    $mday,$year+1900,$hour,$min);

    ($fmt == 2) and
    	return $dname;

    ($fmt == 3) and
    	return $mname;

    ($fmt == 4) and
    	return sprintf("%2d",$mday);

    ($fmt == 5) and
    	return sprintf("%02d",$year % 100);

    ($fmt == 6) and
    	return $year + 100;

    ($fmt == 7) and
    	return sprintf("%02d",$hour);

    ($fmt == 8) and
    	return sprintf("%02d",$min);

    ($fmt == 9) and
    	return sprintf("%02d",$sec);

    ($fmt == 10) and
    	return sprintf("%2d",($hour > 12) ? $hour-12 : $hour);

    if ($fmt == 11 or $fmt == 12)
    {
        my $m= ($hour*3600 + $min*60 + sec < 43200) ? 'am' : 'pm';
    	return ($fmt == 11) ? $m : uc($m);
    }

    ($fmt == 13) and
    	return sprintf("%2d",$mon+1);

    ($fmt == 14) and
    	return sprintf("%03d",$yday);

    ($fmt == 15) and
    	return sprintf("%2d",$wday);
}


sub sf_plural {
    my ($zflag, $width, $prec)= @_;
    return sepjust($zflag, $width, $prec, $was_plural ? 's' : '');
}

sub sf_printlead {
    my ($zflag, $width, $prec)= @_;
    return sepjust($zflag, $width, $prec, ' ' x $leadin);
}

sub sf_setlead {
    my ($zflag, $width)= @_;
    $leadin= $width;
    return '';
}

sub sf_nonl {
    $finalnl= 0;
    return '';
}

sub sf_def {
    my ($name, $zflag, $width, $prec,)= @_;
    my $value= defval($name,2);
    return sepjust($zflag, $width, $prec, $value);
}

sub sf_unseen {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{unseen});
}

sub sf_new {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{newr} + $confh->{newi});
}

sub sf_brandnew {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{newi});
}

sub sf_newresp {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{newr});
}

# total number of participants (in PARTMSG only)
sub sf_total {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $total);
}

sub sf_myname {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{myname});
}

sub sf_myid {
    my ($z, $w, $p)= @_;
    my $id= defined($confh->{myid}) ? $confh->{myid} :
    				      $confh->{sysh}->{username};
    return sepjust($z, $w, $p, $id);
}

sub sf_mydir {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, 'UNIMPLEMENTED');
}

sub sf_first {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{first});
}

sub sf_last {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{last});
}

sub sf_fail {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, '');
}

sub sf_items {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{items});
}

sub sf_confalias {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{alias});
}

sub sf_hostname {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $sysh->{name});
}

sub sf_hostalias {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $sysh->{alias});
}

sub sf_confname {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{name});
}

sub sf_partfile {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $confh->{partfile});
}

sub sf_conftype {
    my ($z, $w, $p)= @_;
    my $t;
    if ($confh->{userlist})
    { 
    	$t= $confh->{password} ? 6 : 4;
    }
    else
    {
    	$t= $confh->{password} ? 5 : 0;
    }
    $t+= 16 if ($confh->{fishbowl});
    return sepjust($z, $w, $p, $t);
}

sub sf_conftypetext {
    my ($z, $w, $p)= @_;
    my $t;
    if ($confh->{fishbowl})
    {
    	$t= 'Fishbowl with '.
	    ($confh->{userlist} ? 'User List' : '').
	    (($confh->{userlist} and $confh->{password}) ? ' and ' : '').
	    ($confh->{password} ? 'Password' : '');
    }
    elsif ($confh->{userlist} or $confh->{password})
    {
    	$t= 'Closed with '.
	    ($confh->{userlist} ? 'User List' : '').
	    (($confh->{userlist} and $confh->{password}) ? ' and ' : '').
	    ($confh->{password} ? 'Password' : '');
    }
    else
    {
    	$t= 'Open';
    }
    return sepjust($z, $w, $p, $t);
}

sub sf_lastdate {
    return sepdate($_[1], $confh->{lastin});
}

sub sf_now {
    return sepdate($_[1], time);
}

sub sf_date0 {
    return sepdate($_[1], $resph?$resph->{date}:$itemh->{linkdate});
}

sub sf_date1 {
    my $fmt= (defined $_[1]) ? $_[1] : 1;
    return sepdate($fmt, $resph?$resph->{date}:$itemh->{linkdate});
}

sub sf_unimp {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, 'UNIMPLEMENTED');
}

sub sf_show {
    my ($zflag,$what)= @_;
    my ($file)= $fileno[$what];
    return undef if !defined $confh;
    return $confh->{$file} if defined $confh->{$file};
    require "if_conf.pl";
    ft_getconf($confh,$file);
    return $ft_err ? $ft_errmsg : $confh->{$file};
}

sub sf_authname {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, ($resph?$resph:$itemh)->{authorname});
}

sub sf_authid {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, ($resph?$resph:$itemh)->{authorid});
}

sub sf_authuid {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, ($resph?$resph:$itemh)->{authoruid});
}

sub sf_title {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p,
	$itemh->{mytitle} ? $itemh->{mytitle} : $itemh->{title});
}

sub sf_ino {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $itemh->{number});
}

sub sf_rno {
    my ($z, $w, $p)= @_;
    if ($resph)
    {
	# In rsep and fsep this is response number
	return sepjust($z, $w, $p, $resph->{number}+0);
    }
    else
    {
	# In nsep this is number of new responses (never counting item)
	my $fr= $itemh->{firstresp} > 0 ? $itemh->{firstresp} : 1;
	return sepjust($z, $w, $p, $itemh->{maxresp} - $fr + 1);
    }
}

# length of response in kilobytes
sub sf_rlenk {
    my ($z, $w, $p)= @_;
    my $len= $resph ? length($resph->{text}) : $itemh->{size};
    return sepjust($z, $w, $p, int($len/1024));
}

# length of response in byte
sub sf_rlen {
    my ($z, $w, $p)= @_;
    my $len= $resph ? length($resph->{text}) : $itemh->{size};
    return sepjust($z, $w, $p, $len+0);
}

# length of response in lines
sub sf_rline {
    my ($z, $w, $p)= @_;
    my $lines= $resph ? ($resph->{text} =~ tr/\n//) : $itemh->{lines};
    return sepjust($z, $w, $p, $lines+0);
}

# max response number
sub sf_maxresp {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $itemh->{maxresp}+0);
}

# current line number (in TXTSEP, FSEP only)
sub sf_lnum {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $flagbits);
}

# text of current line (in TXTSEP, FSEP only)
sub sf_line {
    my ($z, $w, $p)= @_;
    return sepjust($z, $w, $p, $total);
}

sub sc_repeat {
    my $count= shift;
    return join('',@_) x $count;
}

sub sc_diff {
    my ($resp)= @_;
    my $rc= ($rdflg->{lastitem} != $itemh->{number} or
    	($resp and $rdflg->{lastresp} != $resph->{number}));
    $rdflg->{lastitem}= $itemh->{number};
    $rdflg->{lastresp}= $resph->{number} if $resp;
    return $rc;
}

sub sc_once {
    my $rc= !$rdflg->{once};
    $rdflg->{once}= 1;
    return $rc;
}


1;
