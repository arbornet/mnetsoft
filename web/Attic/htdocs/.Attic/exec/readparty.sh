# readparty.sh by Rob Henderson (robh@cyberspace.org)
# This script allows Lynx users to read a party channel
# in real time.  It lists the available logs, then runs
# "tail -f" on the chosen log file.

ls -C /usr/spool/party/log
echo ''

echo "What channel log would you like to read?"
read log

if [ -f /usr/spool/party/log/$log ]
then
	echo "Reading $log, use Ctrl-C to quit."
	echo ''
	tail -f /usr/spool/party/log/$log
else
	echo "Sorry, $log is not a valid channel log."
fi
