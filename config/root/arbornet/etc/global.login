#
# $Id: global.login 1040 2011-01-15 10:14:36Z cross $
#
# Global .login file.  See also csh(1), tcsh(1).
# Note that the path is set in /etc/login.conf, not here,
# though we do make small adjustments.
#

# Set umask (-rw-r--r--)
umask 022

setenv	ARCH		`uname -m`
setenv	OPSYS		`uname -s`
setenv	SYS		"${OPSYS}/${ARCH}"
setenv	BLOCKSIZE	1k
setenv	VISUAL		nano
setenv	EDITOR		${VISUAL}
setenv	HOSTNAME	`hostname -s`
setenv	MAIL		$HOME/Mailbox
setenv	MAILDROP	${MAIL}
setenv	NNTPSERVER	news.eternal-september.org

# Set user timezone from file, if it exists.
if ( -f ~/.tz ) then
	setenv TZ	`cat ~/.tz`
endif

unsetenv LD_LIBRARY_PATH
unsetenv LD_LOAD_PATH
unsetenv LD_LINKER_PATH
unsetenv LD_PRELOAD

#
# Various shell limits.
# Real limits are enforced in /etc/login.conf, we just set defaults
# for things the user can tweak.
#
#limit coredumpsize 0

#
# Path adjustments, if applicable.  Check for porters (users who assist
# with community validation of new accounts) and personal bin directories,
# if they exist. Porters need /cyberspace/sbin in their path to run the
# validate script.
#
( id -G -n | grep -q porters ) && set path = ( /cyberspace/sbin $path )

if ( -d ~/bin/${SYS} ) then
	set path = ( ~/bin/${SYS} $path )
endif

if ( -d ~/bin/${OPSYS} ) then
	set path = ( ~/bin/${OPSYS} $path )
endif

if ( -d ~/bin ) then
	set path = ( ~/bin $path )
endif


#
# Note that nothing following should be executed for a non-interactive
# session.  Theoretically, this file will only be read on an interactive
# session, but we test for a tty anyway, just in case a user ssh's in
# and does a /bin/csh -l for some bizarre reason.
#

test -t 0
if ( $status == 0 ) then
	#
	# Set the user's Orville Write mesg preferences.
	# 1. Not a helper (-h n),
	# 2. Accept writes and tel's (-p a)
	# 3. Accept char-by-char writes (-m a)
	# 4. Don't record messages (-r n)
	# 5. Beep on telegram arrival (-b y)
	#
	setenv MESG mesg
	mesg -h n -p a -m a -r n -b y

	# Set up the user's terminal.
	#set noglob histchars=""
	#eval `tset -s -m'dialup:?vt100'`
	#unset noglob histchars

	# If a user types capital letters at the login prompt, this
	# converts an all-uppercase session to mixed case.
	stty crt echoe -xcase -iuclc -olcuc

	# Fun stuff....
	if ( ! -e ~/.hushall && ! -e ~/.hushlogin ) then
		( echo -n 'Currently logged on: ' && users ) | fmt -78
	endif

	if ( ! -e ~/.hushall && -x /usr/games/fortune ) then
		echo ''
		/usr/games/fortune
		echo ''
	endif

	# Msgs is sufficiently important that all users should see it.
	msgs -fp

	# If we need the password to be changed for whatever reason,
	# do so here.
	if ( -f ~/.needpwchange ) then
		set status = 1
		while ( $status != 0 )
			echo ""
			echo "Your password must be changed; doing so now."
			echo ""
			passwd
		end
		if ( $status == 0 ) then
			rm -f ~/.needpwchange
		endif
	endif
endif
