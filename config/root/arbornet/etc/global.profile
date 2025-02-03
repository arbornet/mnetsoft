#
# $Id: global.profile 1040 2011-01-15 10:14:36Z cross $
#
# Global .profile file.  See also sh(1), ksh(1), bash(1), zsh(1).
# Note that the path is set in /etc/login.conf, not here,
# though we do make small adjustments.
#

# Set umask (-rw-r--r--)
umask 022

ARCH=`uname -m`
OPSYS=`uname -s`
SYS="${OPSYS}/${ARCH}"
BLOCKSIZE=1k
VISUAL=nano
EDITOR=${VISUAL}
HOSTNAME=`hostname -s`
MAIL=${HOME}/Mailbox
MAILDROP=${MAIL}
NNTPSERVER=news.eternal-september.org
export ARCH OPSYS SYS BLOCKSIZE VISUAL EDITOR HOSTNAME MAIL MAILDROP NNTPSERVER

# Set user timezone from file, if it exists.
if [ -f $HOME/.tz ]; then
	TZ=`cat $HOME/.tz`
	export TZ
fi

unset LD_LIBRARY_PATH
unset LD_LOAD_PATH
unset LD_LINKER_PATH
unset LD_PRELOAD

#
# Various shell limits.
# Real limits are enforced in /etc/login.conf, we just set defaults 
# for things the user can tweak.
#
ulimit -c 0

#
# Path adjustments, if applicable.  Check for porters (users who assist
# with community validation of new accounts) and personal bin directories,
# if they exist. Porters need /cyberspace/sbin in their path to run the
# validate script.
#
if ( id -G -n | grep -q porters ); then
	PATH=/cyberspace/sbin:$PATH
fi

if [ -d ${HOME}/bin/${SYS} ]; then
	PATH=${HOME}/bin/${SYS}:${PATH}
fi

if [ -d ${HOME}/bin/${OPSYS} ]; then
	PATH=${HOME}/bin/${OPSYS}:${PATH}
fi

if [ -d ${HOME}/bin ]; then
	PATH=${HOME}/bin:${PATH}
fi
export PATH

if test -t 0
then
	PS1="${HOSTNAME}$ "
	export PS1

	#
	# Set the user's Orville Write mesg preferences.
	# 1. Not a helper (-h n),
	# 2. Accept writes and tel's (-p a)
	# 3. Accept char-by-char writes (-m a)
	# 4. Don't record messages (-r n)
	# 5. Beep on telegram arrival (-b y)
	#
	MESG=mesg
	export MESG
	mesg -h n -p a -m a -r n -b y

	# Set up the user's terminal.
	#eval `tset -s -m'dialup:?vt100'`
	stty crt echoe -xcase -iuclc -olcuc

	# Fun stuff....
	if [ ! -e ${HOME}/.hushall ] && [ ! -e ${HOME}/.hushlogin ]; then
		( echo -n 'Currently logged on: ' && users ) | fmt -78
	fi

	if [ ! -e ${HOME}/.hushall ] && [ -x /usr/games/fortune ]; then
		echo ''
		/usr/games/fortune
		echo ''
	fi

	# Msgs is sufficiently important that all users should see it.
	msgs -fp

	# If we need the user to change his/her password for whatever
	# reason, do so here.
	if [ -f $HOME/.needpwchange ]; then
		while ! passwd; do
			true
		done
		rm -f $HOME/.needpwchange
	fi
fi
