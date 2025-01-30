# rununix.sh by Rob Henderson (robh@cyberspace.org)
# This command lets the user input a single Unix command, which
# will then be run.

echo "What command would you like to run?"
read command

echo ''
exec $command
