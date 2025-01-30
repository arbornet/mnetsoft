/*
 * who.c
 */

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>

#include "libgrex.h"

int
main(int argc, char *argv[])
{
	int fd;
	struct utmp u;
	char utime[32];

	fd = open(_PATH_UTMP, O_RDONLY);
	if (fd < 0) {
		warning("who: can't open utmp: %r");
		return(EXIT_FAILURE);
	}
	while (read(fd, &u, sizeof(u)) == sizeof(u)) {
		if (*u.ut_name == '\0' || *u.ut_line == '\0')
			continue;
		ctime_r(&u.ut_time, utime);
		printf("%-8.*s %-8.*s %s",
		    UT_NAMESIZE, u.ut_name,
		    UT_LINESIZE, u.ut_line,
		    utime + 4);
	}
	close(fd);

	return(EXIT_SUCCESS);
}
