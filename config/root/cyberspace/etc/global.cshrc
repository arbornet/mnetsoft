#
# $Id: global.cshrc 1040 2011-01-15 10:14:36Z cross $
#
# Common .cshrc file.  See also csh(1) and environ(7).
# Note that the path is set in /etc/login.conf, not here.
#

unalias	ls
unalias	cp
unalias	rm
unalias	mv

alias	f		finger
alias	c		clear
alias	h		history 25
alias	ls		ls -ACF
alias	la		\ls -a
alias	lf		\ls -FA
alias	ll		\ls -lA

if ($?prompt) then
	set filec notify ignoreeof
	set history = 100
	set savehist = 100
	set mail = (1 /var/mail/${LOGNAME})

	switch ($shell)
	case *tcsh*:
		#set watch = (1 any any)
		set prompt = "%m% "
		unset autologout
		breaksw
	default:
		set prompt = "`hostname -s`% "
		breaksw
	endsw
endif
