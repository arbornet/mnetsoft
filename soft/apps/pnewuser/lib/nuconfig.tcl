# -+=Tcl/Tk=+-
#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: nuconfig.tcl 1006 2010-12-08 02:38:52Z cross $
#
# Tcl code and routines for dealing with the newuser configuration
# file.  This is much better than hacking all of this in C, and
# privilege separation allows us to keep it secure, too.
#

proc lshift arg {
	upvar $arg alist
	set head [lindex $alist 0]
	set alist [lrange $alist 1 end]
	return $head
}

proc check_boolean_option {option argv} {
	upvar $argv args
	foreach arg $args {
		if [string equal "$arg" "$option"] then { return 1 }
	}
	return 0
}

global config shells editors quotadevs userdev tmpdev maildev
set config(version) "tcl8.4:1.0:28-Jan-2007"
#set configdir "/etc/pnewuser"
#set configdir "/Users/cross/work/grex-not-grexconfig/grexsoft/apps/pnewuser/etc"
#set configdir "/var/users/cross/work/grex/grexsoft/apps/pnewuser/etc"
set configdir "/home/cross/work/grex/soft/apps/pnewuser/etc"

proc operator oper {
	global config
	set config(operator) $oper
}

proc quotadev {type device argv} {
	global config quotadevs
	set args $argv
	set h 0; set s 0; set ih 0; set is 0; set q 0
	while {[string match -* [set opt [lshift args]]]} {
		switch -- $opt {
			-quota { set q  1 }
			-hard  { set h  [lshift args] }
			-soft  { set s  [lshift args] }
			-ihard { set ih [lshift args] }
			-isoft { set is [lshift args] }
		}
	}
	lappend config($type) $device
	if { $q != 0 } {
		set quotadevs($device) [list $h $s $ih $is]
	}
}
proc userdev args { return [quotadev userdev [lshift args] $args] }
proc tmpdev  args { return [quotadev tmpdev  [lshift args] $args] }
proc maildev args { return [quotadev maildev [lshift args] $args] }

proc userpath procname {
	global config
	set config(userpath) $procname
}

proc uidrange {minuid maxuid} {
	global config
	set config(uidrange) "$minuid..$maxuid"
}

proc group groupname {
	global config
	set config(group) $groupname
}

proc loginclass classname {
	global config
	set config(loginclass) $classname
}

proc checkdirs flag {
	global config
	set config(checkdirs) $flag
}

proc maxauthtries maxtries {
	global config
	set config(maxauthfailures) $maxtries
}

proc log args {
	global config
	while {[string match -* [set opt [lshift args]]]} {
		switch -- $opt {
			-syslog { set config(syslog)  [lshift args] }
			-file   { set config(logfile) [lshift args] }
			-prog   { set config(logprog) [lshift args] }
		}
	}
}

proc mailerrs args {
	global config
	set opt [lshift args]
	switch -- $opt {
		-file { set config(mailerrsfile) [lshift args] }
	}
}

proc command_with_default {array default_key argv} {
	global config $array
	upvar 0 $array a
	set args $argv
	set shortcut [lshift args]
	set command  [lshift args]
	set isdefault [check_boolean_option "-default" args]
	lappend config($array) $shortcut
	set a($shortcut) $command
	if { $isdefault } { set config($default_key) $shortcut }
}

proc shell args { command_with_default shells defaultshell $args }
proc editor args { command_with_default editors defaulteditor $args }

proc lockdir {dir} {
	global config
	set config(lockdir) $dir
}

proc emptydir {dir} {
	global config
	set config(emptydir) $dir
}

proc skeldir {dir} {
	global config
	set config(skeldir) $dir
}

proc restrict args {
	global config
	set config(restrictedfile) [lshift args]
	set config(sharerestricted) [check_boolean_option "-share" args]
}

proc badnames args {
	global config
	while {[string match -* [set opt [lshift args]]]} {
		switch -- $opt {
			-aliases     { lappend config(aliases)     [lshift args] }
			-badnames    { lappend config(badnames)    [lshift args] }
			-badpatterns { lappend config(badpatterns) [lshift args] }
		}
	}
}

# Set defaults.
proc default_userpath {login} { return $login }
set config(checkdirs) false
set config(mkpath) default_userpath

# Bring in the program's configuration.
source "$configdir/newuser.conf"
