/* Copyright 2005, Jan D. Wolter, All Rights Reserved. */

#include "common.h"
#include "date.h"

main()
{
    time_t utime;
    char bf[BFSZ+1];
    struct tm tm;

    while (fgets(bf,BFSZ,stdin))
    {
	utime= strtime(bf, 0, 1, NULL, &tm);
	if (utime < 0)
	    printf("Bad date\n");
	else
	    printf("%d : %s", utime, ctime(&utime));
    }
}
