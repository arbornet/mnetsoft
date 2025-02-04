# ntalk.sh by Rob Henderson
# This shell lets Lynx users run ntalk, which requires
# a userid as an argument.

echo "Who would you like to talk to?"
read talk_to

echo ''
echo "When you're finished, type Ctrl-C to return to Lynx."

ntalk $talk_to
