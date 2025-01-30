#
# Awk script to update /usr/local/www/stats/index.html with the latest
# web stats from the Analog program.
#
# Written by Valerie Mates, 12/11/96.
#
BEGIN {

   #
   # Split passed in string called "parameters" into an array called "list".
   # list[1] is the file name, for example "httpd961207.html"
   # list[2] is the start month, eg "December"
   # list[3] is the start day, eg "01"  (would like to trim the 0, but oh well)
   # list[4] is the end month, eg "December"
   # list[5] is the end day, eg "07"
   # list[6] is the year, eg "1996"

   split(parameters, list);
}

{
   #
   # Automatically write out every line that is passed in.
   #
   printf("%s\n", $0);

   #
   # If the current line is our special comment, print out an
   # extra line following it.
   #
   if ($0 == "<!-- Don't change this comment!  A cron job scans for it to add entries. -->") {
      printf("<li><a href=%s>%s %s-%s %s %s</a>\n", list[1], list[2], list[3],
         list[4], list[5], list[6])
   }
}
