# Copyright 2001, Jan D. Wolter, All Rights Reserved.

require "if_post.pl";

# get_text($oldtext)
#  This runs the editor to gather some text for a response.  If we already
#  have pre-existing text, it can be passed in.

sub get_text {
    my ($old)= @_;
    my ($line, $text);

    my $buffer= "$cfdir/$BUFFER";

    # If buffer already exists, rename it
    if (-e $buffer)
    {
    	my $i;
	for ($i= 0; !rename($buffer,$buffer.'.'.($$+$i)); $i++)
	{
	    $i < 10 or Fail("Cannot rename file $buffer\n");
	}
    }

    # If we have some old text, preload it into the buffer
    if (defined $old)
    {
    	open BUFF, ">$buffer" or
	    Fail("Cannot open $buffer\n");
	print BUFF $old;
	close BUFF;
	$old= undef;
    }

    EDIT:
    my $rcs= system("$defhash{editor} $buffer") & 0xffff;
    if ($rcs == 0xff00 || $rcs == 0xffff )
    {
	print "Editor $defhash{editor} not found\n";
	unlink $buffer;
	return undef;
    }
    my $rc= $rcs >> 8;

    # ABORT
    if ($rc == 66)
    {
    	unlink $buffer;
	return undef;
    }

    # ASKOK
    if ($rc == 66)
    {
	while (1)
	{
	    print "OK to enter? ";
	    my $ans= <STDIN>;
	    goto EDIT if $ans =~ /^\s*[n|N]/;
	    last if $ans =~ /^\s*[y|Y]/;
	}
    }

    if ($rc != 0 and $rc != 67)
    {
	print "Error - Strange return code from $defhash{editor} ($rc)\n";
	return undef;
    }

    # read back data
    open BUFF,"$buffer" or
    	return undef;
    $text= '';
    while ($line= <BUFF>)
    {
    	$text.= $line;
    }
    close BUFF;
    unlink $buffer;

    return $text;
}


sub rfp_enter {
    my ($args, $itemh, $rdflag)= @_;
    print "\nEntering new Item...\n";
    my $rc= cmd_enter($args);
    print "\n...Returning to item $itemh->{number}\n";
    return $rc;
}


sub cmd_enter {
    my ($args)= @_;
    my ($text,$title,$ok,$len,$n);

    inconf() or return 1;

    if ($sysh->{amanon} and !$confh->{anonpost})
    {
	print "You must login to be able to post items\n";
	return 1;
    }

    TEXT: while (1)
    {
	$text= get_text($text);
	defined($text) or return 1;
	while (1)
	{
	    print "Enter a one line header or ':' to edit\n? ";
	    $title= <STDIN>;
	    chomp $title;
	    if ($title =~ /^\s*$/)
	    {
	    	print "Ok to abandon item entry? ";
		$ok= <STDIN>;
		return 1 if $ok =~ /^\s*[y|y]/;
		next;
	    }
	    next TEXT if $title =~ /^\s*:/;
	    last;
    	}
	while (1)
	{
	    print "Ok to enter this item? ";
	    $ok= <STDIN>;
	    $ok=~ s/^\s*//;
	    $ok=~ s/\s*$//;
	    $ok= lc($ok);
	    $len= length($ok);

	    return 1 if $ok eq 'n' or $ok eq 'no';
	    last TEXT if $len >= 1 and $ok eq substr('yes',0,$len);
	    next TEXT if ($len >= 2 and $ok eq substr('again',0,$len)) or
	                 ($len >= 1 and $ok eq substr('continue',0,$len)) or
	                 ($len >= 2 and $ok eq substr('edit',0,$len));
	    if ($len >= 2 and $ok eq substr('print',0,$len))
	    {
		my $line;
	    	foreach $line (split /^/m, $text)
		{
		    print ' ',$line;
		}
		next;
	    }
	    if ($len >= 2 and ($ok eq substr('empty',0,$len) or
		 	       $ok eq substr('clear',0,$len)))
	    {
	    	$text= undef;
		next TEXT;
	    }
	    if ($ok eq '?' or ($len >= 1 and $ok eq substr('help',0,$len)))
	    {
	    	print "commands legal here:\n",
		      "n_o     abort text entry and command.\n",
		      "y_es    done entering this text, proceed.\n",
		      "ed_it   invoke editor on this\n",
		      "ag_ain,c_ontinue        continue entering text\n",
		      "pr_int  display current text\n",
		      "em_pty,cl_ear   empty buffer and start over\n";
	    	next;
	    }
	    print "I don't understand \"$ok\" - type HELP for help\n";
	}
    }

    $n= ft_enter($confh, $title, $text);
    if (defined $n)
    {
	print "Entering as item $n.\n";
    }
    else
    {
    	print "Item not entered: $ft_errmsg\n";
    }
    return 1;
}


sub respond {
    my ($pseudo, $args, $itemh)= @_;
    my ($text, $n);

    if ($sysh->{amanon} and !$confh->{anonpost})
    {
	print "You must login to be able to respond to items\n";
	return 1;
    }

    $text= get_text($text);
    if (defined $text)
    {
	$n= ft_respond($confh, $itemh->{number}, $text, $pseudo);
	if (!defined $n)
	{
	   print "Response not entered: $ft_errmsg\n";
	   return 1;
	}
	if ($n > $itemh->{maxresp} + 1)
	{
	    print 'Response';
	    if ($n == $itemh->{maxresp} + 2)
	    {
		print ' ',($n - 1);
	    }
	    else
	    {
		print 's ',($itemh->{maxresp} + 1),' through ',($n - 1);
	    }
	    print " slipped in.\n";
	}
	$itemh->{maxresp}= $n;
	return (flagval('stay') ? 1 : 2);
    }
    else
    {
    	print "Response aborted!  Returning to current item.\n";
	return 1;
    }
}


sub cmd_resp {
    my ($itemno) = @_;

    if (!($itemno =~ /\d+/))
    {
        print "Invalid argument!  Item number required.\n";
        return 1;
    }
    $itemh = ft_nextitem($confh, $itemno, "", 1, \&resp_item_cb, \&resp_resp_cb);
    if (!defined $itemh)
    {
        print "Item does not exist! Check argument.\n";
        return 1;
    }
    return respond(undef, undef, $itemh);
}

sub rfp_resp {
    return respond(undef, @_);
}

sub rfp_pseudo {
    print "What's your handle? ";
    my $pseudo= <STDIN>;
    if ($pseudo =~ /^\s*$/)
    {
    	print "Response aborted!  Returning to current item.\n";
	return 1;
    }
    return respond($pseudo, @_);
}

sub resp_item_cb
{
}

sub resp_resp_cb
{
}


1;
