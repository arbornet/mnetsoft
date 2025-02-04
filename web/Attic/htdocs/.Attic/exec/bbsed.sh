# bbsed.sh by Rob Henderson (robh@cyberspace.org)
# This shell allows Lynx users to use bbsed, which requires a
# file name as an argument.

echo -n "Enter the name of the file you'd like to edit:  "
read ed_file

bbsed $ed_file
