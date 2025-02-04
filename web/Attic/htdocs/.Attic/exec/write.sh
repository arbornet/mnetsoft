# write.sh by Rob Henderson (robh@cyberspace.org)
# This shell allows Lynx users to run write, which requires
# a userid as an argument.

echo "Who would you like to write to?"
read write_to

echo ''
echo "When you're finished, type Ctrl-D to return to Lynx."

write -c $write_to
