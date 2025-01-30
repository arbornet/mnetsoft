/**
 * @filename checkgrps.c
 *
 * Read through the /etc/group file, looking for
 * stale entries.
 *
 * @memo Check groups for stale members.
 *
 * $Id: checkgrps.c 1494 2014-09-14 21:21:55Z cross $
 */

#include <sys/types.h>

#include <grp.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libgrex.h"

const	char *OPTIONS = "?";

char	*prog;

static void process(void);
static void usage(char *);

int
main(int argc, char *argv[])
{
	int	ch;

	prog = progname(argv[0]);
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage(prog);
	process();

	return(EXIT_SUCCESS);
}

static void
process(void)
{
	struct	group *g;
	char	*p;

	setgrent();
	while ((g = getgrent()) != NULL)
		while ((p = *g->gr_mem++) != NULL)
			if (getpwnam(p) == NULL)
				printf("%s:%d: %s\n", g->gr_name, g->gr_gid, p);
	endgrent();
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ XXX options ].\n", prog);
	exit(EXIT_FAILURE);
}
