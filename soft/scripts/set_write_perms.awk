BEGIN {
   found_it = 0;
}

{
   if ($1 == "mesg") {
      printf("\tmesg %s\n", parameters);
      found_it = 1;
   } else if ((found_it == 0) && (($1 == "menu") || ($1 == "lynx"))) {
      printf("\tmesg %s\n", parameters);
      printf("%s\n", $0);
      found_it = 1;
   } else printf("%s\n", $0);
}

END {
   if (found_it == 0) printf("mesg %s\n", parameters);
}
