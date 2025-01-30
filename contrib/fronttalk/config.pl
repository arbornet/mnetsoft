
# Copyright 2001, Jan D. Wolter, All Rights Reserved.

# SysList File/URL
#  Location of list of systems.  If it starts with "/" or "file:", then it is
#  the full pathname of a file on the local system.  If it starts with
#  "http:" then it is a URL.  Either way, we expect a plain text file that
#  gives locations of all Backtalk systems known to this front end.

$SYSLIST= "/cyberspace/lib/fronttalk-0.9.2/syslist";


# Help Directory
#   Full path name of the directory that contains all the help files.

$HELPDIR= "/cyberspace/lib/fronttalk-0.9.2/help";


# Proxy Path
#  If we need a proxy to do http, give the URL of the proxy.  Otherwise, leave
#  it undefined.

# Gryps?  Commented out on Fri Aug 8 2008 by cross.
#$PROXY= "http://216.93.104.35";


# Default Editor
#    Path of default editor to use for text entry

$DEFAULT_EDITOR= "/cyberspace/bin/gate";


# Default Pager
#    Path of default pager to use for text entry

$DEFAULT_PAGER= "more";


# Default Browse Style
#    The 'browse' command has two possible formats, long and short.  If
#    $BROWSE_SHORT is true, it defaults to short, otherwise it defaults to
#    long.
$BROWSE_SHORT= 1;


# Buffer File
#    Name of temporary file used for text entry

$BUFFER= "ft.buffer";

# Command paths
#    These command paths on your local server for various Unix commands that
#    Picospan traditionally knows about and uses.  Normally these should all
#    be in your user's path, so you don't need to give the full path name
#    here, just the command name, but you can change it to the full path if
#    you want.  If you don't have some of these, or don't want to enable the
#    Fronttalk commands that access them, leave them undefined (just comment
#    out the lines here):

$PATH_PASSWD= "passwd";         # command for changing local password
$PATH_WHO= "who";               # command to list local users
$PATH_WRITE= "write";           # command to chat with other local users
$PATH_MESG= "mesg";             # command to control write permissions

# A path to the stty command, is required:

$PATH_STTY= "stty"              # command to turn on and off echo
