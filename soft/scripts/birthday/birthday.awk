# Usage:  awk -f birthday.awk date="`date`" birthday.list

BEGIN {
   got_one = 0
}

{
   thismonth = substr(date, 5, 3)
   today = substr(date, 9, 2)

   month = $1
   day = $2

   if ((today + 0 == day + 0) && (thismonth == month))  {
      printf "     %s\n", substr($0, length($1) + length($2) + 2, length())
      got_one = 1
   }
}

END {
   if (got_one == 1) printf("\n")
}
