#
# ~/.cshrc
#
# The commands in this file are executed each time the C shell or any
# of its derivatives (such as tcsh) are invoked.  Be careful editing this
# file; a mistake can make for a most unpleasant experience.
#
# See also csh(1), environ(7).
#

# WARNING: Do not remove or change the next line.
source /cyberspace/etc/global.cshrc

#
# The following lines will only be executed by interactive shells.
#
if ( $?prompt ) then
	alias hi history
	alias so source
endif
