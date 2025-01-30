/*
 * realnames.c - Print real names and login's of specified users.
 *
 *	Dan Cross <cross@cyberspace.org>
 *
 * $Id: realnames.c 1485 2014-05-21 14:18:50Z cross $
 */

#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libgrex.h"

char *prog;

int
main(int argc, char *argv[])
{
	struct passwd *pw;
	char *p;

	prog = progname(argv[0]);
	if (argc <= 1) {
		fprintf(stderr, "Usage: %s user[s].\n", prog);
		return (EXIT_FAILURE);
	}
	while (*++argv) {
		pw = getpwnam(*argv);
		if (pw == NULL) {
			warning("Unknown user: %s.\n", *argv);
			continue;
		}
		p = strchr(pw->pw_gecos, ',');
		if (p != NULL)
			*p = '\0';
		printf("%s (%s)\n", pw->pw_gecos, pw->pw_name);
	}

	return (EXIT_SUCCESS);
}
