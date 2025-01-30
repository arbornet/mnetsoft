# weblint.sh by Rob Henderson (robh@cyberspace.org)
# This shell sllows users to specify files for weblint to check

echo ''
echo "What file would you like to check?"
read html_file

echo ''

/usr/local/www/exec/weblint $html_file
