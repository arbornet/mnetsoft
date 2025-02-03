#
# ~/.profile
#
# This is the initialization file for sh, ksh, bash, and zsh.  As such,
# it only uses a common subset of commands.
#
# The commands in this file are executed each time you *login* to Grex,
# not on each shell invocation. Be careful editing this file; a mistake
# can make for a most unpleasant experience.
#
# See also sh(1), environ(7).
#

# WARNING: Do not remove or change the next line.
. /cyberspace/etc/global.profile

#
# The following lines will only be executed for interactive shells.
# Theoretically, this shouldn't be necessary, but people sometimes
# invoke csh and friends in strange ways, so we test for the existence
# of a tty just in case.
#

if test -t 0
then
	# Set up the terminal.
	eval `tset -s -m'dialup:?vt100'`
fi
