# strparty.sh by Rob Henderson (robh@cyberspace.org)
# This script allows Lynx users to search a party log
# for a particular string.  It lists the available
# party logs, then runs grep to search.

ls -C /usr/spool/party/log
echo ''

echo "What channel log would you like to search?"
read log

echo ''
echo "And what string would you like to search for?"
read string

if [ -f /usr/spool/party/log/$log ]
then
	echo "Searching $log..."
	echo ''
	grep -i $string /usr/spool/party/log/$log | more
else
	echo "Sorry, $log is not a valid channel log."
fi
