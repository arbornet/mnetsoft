#!/bin/sh
#
# M-Net's MENU shells;  written by Dave Parks, copyright: 1994
# Help offered and excepted from: Marc Unangst (mju), Russ Cage (russ),
# Jim Knight (jfk), Jeff Spindler (uhclem) & Dave Thaler (thaler).
#
# DATE LAST MODIFIED: 10/08/1994  By: Dave Parks
#
#
# Request (not demand), mail any changes to kite@m-net.arbornet.org
# M-Net has my permission to do with these menu shells as they please,
# as long as the copyright is NOT removed.
#					   >< Dave Parks ><
#
#
# Modification History:
#     20010314  by Rex Roof (trex)
#               changed E) option to exec csh instead of exit
#	980318	by Leeron Kopelman (lk)
#		removed calendar cf entry
#	980308	by Leeron Kopelman (lk)
#		changed "cat /etc/motd" to "motd"
# 1.06	970901	by Leeron Kopelman (lk)
#		added !faq option
#	970504	by Leeron Kopelman (lk)
#		Commented out Vote option
#	970414	by Leeron Kopelman (lk)
#		Added V) vote option -- moved View MOTD to D)
#	970109	by Leeron Kopelman (lk)
#		applied "basename" to editor and shell so that long names
#		won't mess screen display
#	970104	by Leeron Kopelman (lk)
#		Added "I) Info on Support" and "X) Express Access Upgrade"
#		to main menu
#
#

EDITOR=/usr/local/bin/nano
PATH=/arbornet/bin:/usr/local/bin:/usr/bin:/bin:$HOME/bin
SHELL=/usr/local/bin/bash
VISUAL="$EDITOR"
export EDITOR PATH SHELL VISUAL

func() {
# SPEED=`stty | head -1 | awk '{print $2 " " $3}' | tr ";" "."`
LOGNAME=`logname`
TTY=`tty | sed 's,/dev/,,'`
USERS=`who | wc -l`
USERS="`expr $USERS + 0` total"
DATE=`date | awk '{print $1"   "$2" "$3}'`
PTERM=`echo "$TERM" | awk '{printf "%-20.20s", $0}'`
# USERS="`expr $USERS + 0` total"
}

#	Locking scheme.

if [ -z "$in_menu" ]; then
	in_menu=true
	export in_menu
else
	echo " "
	echo -n "Sorry, You are already running menu once!   (try: exit)."
	echo " "
	echo " "
	exit 1
fi

#	Checking for expert or beginner mode setting to run in
if [ ! -z "$MODE" ]
then
	MODE=$MODE
	export MODE
else
	MODE=beginner
	export MODE
fi

if [ "$1" = ex -o "$1" = expert -o "$1" = EX -o "$1" = EXPERT ]
then
	MODE=expert
	export MODE
else
	MODE=$MODE
	export MODE
fi

#	Copyright information.

trap '' 2
clear
echo "
			       M-Net Menu 3.1
			       Copyright 1994
			  	 Dave Parks"



#	Main body of menu system.

while true
do

beginner () {
func
# Available letters for menu:
# HJKNQZ
echo -n "
Port: $TTY                  `hostname`               Login: $LOGNAME
Editor: `basename $EDITOR`	                $DATE	              Users: $USERS
Terminal: $PTERM	                              Shell: `basename $SHELL`
			    *  M A I N     M E N U  *

	I). Info on Supporting M-Net		X). Express Access Upgrade
	W). Who (who is on the system)		Y). Yell for help!
	B). BBS (Conferencing/YAPP)	        C). Change Password
	M). Mail (Check your mail)	        R). Run a Unix Program
	S). Send Mail			        F). File Utilities
	P). Party (M-Net Multi user chat)       O). Other MENUS
	G). Games (M-Net Unix games menu)       U). Utilities (basic)
	A). Answer (Answer talk)	        D). Display Message Of The Day
	T). Talk (Talk to another user)	        E). Exit menu system
	Q). Help: Frequently Asked Questions    L). Logoff M-Net

Command: "
}

expert () {
echo -n "
System: [`hostname`]

MAIN MENU:

I,X,W,B,M,S,P,G,A,T,CC,Y,C,R,F,O,U,D,Exit,Logout: "
}

  if [ "$MODE" = expert ]
  then
  expert
  else
  beginner
  fi

read ans
 if [ $? -eq 1 ]
 then
 echo " "
 echo " "
 echo "Received: \"EOF\" (End Of File) ..."
 echo " "
 echo "Leaving M-Net's menu system..."
 echo "Type: menu (to go back to it)"
 echo " "
 rm /tmp/sh$$ 2> /dev/null
 exit 1
 fi
	case $ans in


i|I)	# Info on Support
	support
	clear
	;;

x|X)	# Express Access Upgrade
	express
	clear
	;;

v|V)	# Vote Program
	echo "There is no election in progress; no V)otes taken."
	#echo "NOTE: only Members/Patrons can vote.  See I)nfo for becoming a supporter"
	#vote
	;;

w|W)	# Who is on the system.

	clear
	num=`who | wc -l`
	echo "Unix: who or finger"
	echo ""
	echo "M-Net's current users...   $num total."
	echo ""
	finger -g | less -rEX
	#w
	echo " "
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk0
	 clear;;




b|B)	# BBS menu

	/arbornet/libexec/menu/bbsmenu
	sleep 1
	clear;;



m|M)	# Mail scheme (checks your mail in three different programs).

	echo " "
	echo "Choices: (mail), (mutt) or (alpine)"
	echo " "
	echo -n "Mail program to use? "
	read ans
	case $ans in

	m|M|mail|MAIL)
	echo " "
	/usr/bin/Mail
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk
	clear;;

	mu|Mu|mU|mutt|Mutt|MUTT)
	echo " "
	mutt;;

	a|A|alpine|Alpine|ALPINE|p|P|pine|PINE|Pine)
	echo " "
	alpine;;

	*)
	echo " "
	echo -n "Try: mail, mutt or alpine. "
	sleep 2
	esac
	clear;;



s|S)	# Send mail (in three different programs).

	echo " "
	echo "Choices: (mail), (mutt) or (alpine)"
	echo " "
	echo -n "Mail program to use? "
	read ans
	case $ans in

	m|M|mail|MAIL)
	echo " "
	echo -n "Send mail to who? "
	read who
	echo " "
	echo "Terminate your entry with a ctrl-D"
	echo "or a \".\" on a separate line."
	echo " "
	/usr/bin/Mail $who
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk
	clear;;

	mu|Mu|mU|mutt|Mutt|MUTT)
	echo " "
	mutt;;

	a|A|alpine|Alpine|ALPINE|p|P|pine|PINE|Pine)
	echo " "
	alpine;;

	*)
	echo " "
	echo -n "Try: mail, mutt or alpine. "
	sleep 2
	esac
	clear;;



p|P)	# Party, runs the parey menu system.

	trap 'clear ; continue' 2
	clear
	/arbornet/libexec/menu/parmenu
	trap '' 2
	clear;;




g|G)	# Runs the games menu system.

	/arbornet/libexec/menu/gmenu;;



a|A)	# Answers when someone is chatting you.

	echo " "
	echo -n "Answering Call... "
	chat
	sleep 1
	echo " "
	clear;;



t|T)	# Talk, does a who then asks who to chat.

	num=`who | wc -l`
	echo " "
	echo "Unix: chat <userid>"
	echo " "
	echo "Login    Line     Date  Time $num users."
	echo "-----------------------------"
	who | less -rEX
	echo "-----------------------------"
	echo " "
	echo -n "Who would you like to talk to? "
	read junk6
	echo " "
	echo "Please wait for answer...     (ctrl-D to exit)"
	echo " "
	mesg y;/arbornet/bin/write -c $junk6
	clear;;



c|C)	# Changes your passwd.

	echo " "
	/usr/bin/passwd
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read pass
	clear;;



r|R)	# Runs a UNIX program from within the menu shell.

	echo " "
	echo -n "What Unix program would you like to run? "
	read program
	echo " "
	$program
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junkr
	clear;;



u|U)	# Runs the basic programs menus system.

	/arbornet/libexec/menu/bmenu
	clear;;



o|O)	# Runs the OTHER MENUS menu system.

	/arbornet/libexec/menu/mmenu
	clear;;



f|F)	# Runs the FILES menu system.

	/arbornet/libexec/menu/fmenu
	clear;;



ex|EX|expert|EXPERT)	# Changes menu mode to expert
MODE=expert
export $MODE
clear;;

be|BE|beginner|BEGINNER)	# Changes menu mode to beginner
MODE=beginner
export $MODE
clear;;


e|exit|E|Exit|EXIT)	# Exits the entire menu system.

	echo " "
	echo "Leaving M-Net's menu system..."
	echo "Type: menu (to go back to it)"
	echo " "
	# exit;;
	# changed by rex, since we now run the menu as a shell
	unset in_menu
	export in_menu
	exec /usr/local/bin/bash -l;;



l|logout|L|Logout|LOGOUT)	# Logs you all the way off the system.

	tty=`tty`
	echo " "
	echo "Thanks for calling M-Net ..."
	echo "Disconnecting line: $tty"
	echo " "
	echo " "
	sleep 1
	kill -9 -1 0;;                  #changed from -1 to -1,0 (jfk) 



#cc|CC)	# Throws you into Community Calendar.
#
#	echo " "
#	echo -n "Loading YAPP ... "
#	bbs calendar
#	echo " "
#	echo -n "Please hit <RETURN> to continue "
#	read junk
#	clear;;



d|D)	# Views the /etc/motd.

	clear
	echo " "
	echo " "
	motd
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk
	clear;;



q|Q)	# calls faq for Frequently Asked Questions help preprocessor
	/arbornet/bin/faq
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk
	clear;;



y|Y)	# Yells for help to anyone online with a "?" as the mesg.

	echo " "
	echo "M-Net is looking for someone to help you."
	echo " "
	echo "Please wait for an answer...     (ctrl-D to exit)"
	echo " "
	echo " "
	echo " "
	echo " "
	chat help
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk
	clear;;




!*)	# Special shell escape.

	prog=`echo $ans | sed 's/^!//'`
	echo " "
	eval $prog
	echo " "
	echo -n "Please hit <RETURN> to continue "
	read junk
	clear;;




*)	# Catches everything else.

	echo " "
	echo -n "$ans:  Is not supported in this version of the menu ..."
	sleep 1
	clear;;

	esac
done
