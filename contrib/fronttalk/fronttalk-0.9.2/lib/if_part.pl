# Copyright 2001, Jan D. Wolter, All Rights Reserved.  #


#  ft_part($confh, $users, $cb, $cb_extra...)
#  Do a participants command.  If users is defined, it is a list of users.
#  Otherwise we do all users.  For each user found we call $cb passing it
#  a participant handle, which has fields like this:
#
#    sysh       - $confh->{sysh}
#    alias      - $confh->{alias}
#    name       - $confh->{name}
#    myname     - fullname of user.
#    myid       - id of user.
#    myuid      - uid of user.
#    mystatus   - status of user (V=valid, I=invalid, U=unvalidated).
#    lastin     - last date user was in conference.
#    error      - error message if user was not valid.
#
#  Returns number of participants.

sub ft_part {
    my ($confh, $users, $cb, @extra)= @_;
    my $parth;

    # initialize and response handles
    $parth= {sysh => $confh->{sysh},
	     alias => $confh->{alias},
	     name => $confh->{name},
	     partcnt => 0};

    ft_runscript($sysh, "fronttalk/part",
    	"conf=$confh->{alias}&users=".ft_httpquote($users), 1,
	\&ft_part_cb, $parth, $cb, @extra);

    return $parth->{partcnt};
}


sub ft_part_cb {
    my ($tok, $parth, $cb, @extra)= @_;

    if ($tok->{TOKEN} eq 'USER')
    {
	$parth->{myid}= $tok->{ID};
	$parth->{myname}= ft_unquote($tok->{NAME});
	$parth->{myuid}= $tok->{UID};
	$parth->{lastin}= $tok->{LASTIN};
	$parth->{mystatus}= $tok->{STATUS};

	&{$cb}($parth, @extra);
    }
    elsif ($tok->{TOKEN} eq 'COUNT')
    {
	$parth->{partcnt}= $tok->{N};
    }
    elsif ($tok->{TOKEN} eq 'ERROR')
    {
	if ($tok->{ID})
	{
	    $parth->{myid}= $tok->{ID};
	    $parth->{error}= ft_unquote($tok->{MSG});
	    
	    &{$cb}($parth, @extra);
	}
	else
	{
	    ($ft_err,$ft_errmsg)= ('BTE1',"Request failed: ".
		ft_unquote($tok->{MSG}));
	}
    }
    return 0;
}


1;
