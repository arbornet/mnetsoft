# tailparty.sh by Rob Henderson (robh@cyberspace.org)
# This shell sllows Lynx users to read the last ten
# lines of a party log.  It lists the available party
# logs, then runs tail.

ls -C /usr/spool/party/log
echo ''

echo "What channel log would you like to read?"
read log

if [ -f /usr/spool/party/log/$log ]
then
	echo "Tailing $log."
	echo ''
	tail /usr/spool/party/log/$log
else
	echo "Sorry, $log is not a valid channel log."
fi
