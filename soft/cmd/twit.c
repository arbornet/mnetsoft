/*
 * Copyright 1987 Jon Zeeff (jon@ais.org) v1.5
 *
 * This program can be used as a FrontTalk pager to eliminate
 * responses and items by twits.
 * It requires the use of special rseps and iseps that put a "!user\n"
 * before each response.  Then just use this program by putting the
 * following lines into your .cfonce file:

define rsep "!%l\n----\n#%r %a (%l):"
define isep "!%l\nItem %i entered %d by %a (%l)\n %h"
define ishort "!%l\n %3i %h (%l)"
define pager "twit moron bozo etc"

 *
 * $Id: twit.c 1485 2014-05-21 14:18:50Z cross $
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libgrex.h"

static int ok(char *, char **);

int
main(int argc, char *argv[])
{
	int isok;
	char line[1024], *p;

	isok = 1;
	while (fgets(line, sizeof(line), stdin) != NULL) {
		if (line[0] == '!') {
			p = strrchr(line, '\n');
			if (p != NULL)
				*p = '\0';
			isok = ok(line + 1, argv);
			continue;
		}
		if (isok)
			fputs(line, stdout);
	}

	return (EXIT_SUCCESS);
}

/*
 * Check if the author is in our list of twits.
 * If so, return 0 (not ok).  If not, return 1
 * (okay).
 */
static int
ok(char *author, char **argv)
{
	while (*++argv != NULL)
		if (STREQ(author, *argv))
			return 0;
	return 1;
}
