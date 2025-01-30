# Copyright 2005, Jan D. Wolter, All Rights Reserved.

require "if_passwd.pl";


# Password command.  If connected to a remote server that can edit passowrds,
# this changes the remote password.  If doing direct execution, this just
# runs the unix "passwd" program.

sub cmd_passwd {

    if ($cmd_source < 6)
    {
	print "Passwords can only be set from the command line\n";
	return 1;
    }

    if ($sysh->{direct})
    {
	# When direct connected to a server with no password editing, just
	# use the Unix 'passwd' command.
	if (defined $PASSWD_PATH)
	{
	    print "Changing password for $sysh->{name}\n";
	    cmd_unix($PASSWD_PATH);
	}
	else
	{
	    print "Password setting not enabled in this client.\n";
	}
    }
    elsif ($sysh->{pwedit})
    {
	my $op, $np, $np2, $rop;

	# Check server version
	if ($sysh->{versnum} < 9001)
	{
	    print "Server version ", $sysh->{version},
		" does not change password changing.\n";
	    return 1;
	}

	# anonymous users can't change their passwords
	if ($sysh->{amanon} or $sysh->{username} !~ /\S/)
	{
	    print "You must login to ", $sysh->{name},
		" before you can change your password\n";
	    return 1;
	}

	print "Changing password for $sysh->{name}:\n";

	# Get old password
	echo(0);
	print "old password: ";
	chop($op= <STDIN>);
	print "\n";
	if ($op !~ /\S/)
	{
	    echo(1);
	    print "Password change canceled\n";
	    return 1;
	}
	if ($op ne $sysh->{password})
	{
	    echo(1);
	    print "Password incorrect.  Password change canceled\n";
	    return 1;
	}

	# Get the new password twice
	print "new password: ";
	chop($np= <STDIN>);
	print "\n";
	if ($np !~ /\S/)
	{
	    echo(1);
	    print "Password change canceled\n";
	    return 1;
	}

	print "new password again: ";
	chop($np2= <STDIN>);
	echo(1);
	print "\n";
	if ($np2 !~ /\S/)
	{
	    print "Password change canceled\n";
	    return 1;
	}

	if ($np ne $np2)
	{
	    print "New passwords do not match.  Password change canceled.\n";
	    return 1;
	}

	my $err= ft_passwd($np);
	if ($err)
	{
	    print "Password not changed: $err\n"
	}
	else
	{
	    $sysh->{password}= $np;
	}
    }
    else
    {
	print "Password setting not possible on this server.\n";
    }
    return 1;
}

1;
