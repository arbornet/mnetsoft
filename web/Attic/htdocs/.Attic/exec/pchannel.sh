# pchannel.sh by Rob Henderson (robh@cyberspace.org)
# This shell allows Lynx users to select which party channel
# they want to enter.  If none is entered, they are dumped
# into "party".

echo ''
echo "What channel would you like to start in?"
read channel

echo ''
if [ "$channel" = "" ]
then party
else party start=$channel
fi
