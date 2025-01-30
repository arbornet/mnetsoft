/**
 * @name members.c
 *
 * Report users in various groups, looking the groups up by name
 * and writing the users to stdout.
 *
 * Rewritten 08-Mar-2005 by Dan Cross.
 *
 * @memo Brief description of this program.
 *
 * $Id: members.c 1485 2014-05-21 14:18:50Z cross $
 */

#include <sys/types.h>

#include <grp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

const char *OPTIONS = "cg:hilmpsuv?";

char *prog;
int prtgname;

static int cmp(const void *, const void *);
static void process(char *, int);
static void help(void);
static void usage(char *);

enum {
	LINE,
	COLUMN,
	COUNT,

	MAXLINE = 78
};

int
main(int argc, char *argv[])
{
	char *gname;
	int ch, outmode;

	prog = progname(argv[0]);
	outmode = LINE;
	prtgname = 0;
	gname = "members";
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case 'c':
			outmode = COUNT;
			break;
		case 'g':
			gname = optarg;
			break;
		case 'i':
			gname = "internet";
			break;
		case 'l':
			outmode = LINE;
			break;
		case 'm':
			gname = "members";
			break;
		case 'p':
			prtgname++;
			break;
		case 's':
			outmode = COLUMN;
			break;
		case 'u':
			gname = "usenet";
			break;
		case 'v':
			gname = "voters";
			break;
		case '?':	/* FALLTHROUGH */
		case 'h':	/* FALLTHROUGH */
			help();
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 0)
		usage(prog);

	process(gname, outmode);

	return (EXIT_SUCCESS);
}

static int
cmp(const void *a, const void *b)
{
	return (cmpstring(*(const char **) a, *(const char **) b));
}

static void
process(char *gname, int mode)
{
	struct group *gp;
	char **pp, *prefix, *user;
	int col, len, nmem;

	gp = getgrnam(gname);
	if (gp == NULL) {
		warning("Group \"%s\" lookup failure: %r.", gname);
		return;
	}
	for (nmem = 0, pp = gp->gr_mem; *pp != NULL; pp++)
		nmem++;
	qsort(gp->gr_mem, nmem, sizeof(char *), cmp);
	if (mode == COUNT) {
		printf("%d name%s in the %s group.\n",
		       nmem, (nmem == 1) ? "" : "s", gname);
	} else if (mode == COLUMN) {
		col = 0;
		if (prtgname)
			col = printf("%s: ", gname);
		prefix = "";
		for (pp = gp->gr_mem; *pp != NULL; pp++) {
			user = *pp;
			len = strlen(user);
			col = col + len + strlen(prefix);
			if (col > MAXLINE) {
				prefix = ",\n";
				col = len;
			}
			printf("%s%s", prefix, user);
			prefix = ", ";
		}
		printf("\n");
	} else {
		if (prtgname)
			printf("%s:\n", gname);
		for (pp = gp->gr_mem; *pp != NULL; pp++)
			printf("%s\n", *pp);
	}
}

static void
help(void)
{
	printf("This program prints the members of a group.\n");
	printf("\n");
	printf("Recognized options:\n");
	printf("\t%-10s -- %s.\n", "-h", "Print this message");
	printf("\t%-10s -- %s.\n", "-g <group>", "print members of \"group\"");
	printf("\t%-10s -- %s.\n", "-i", "Same as \"-g internet\"");
	printf("\t%-10s -- %s.\n", "-m", "Same as \"-g members\"");
	printf("\t%-10s -- %s.\n", "-u", "Same as \"-g usenet\"");
	printf("\t%-10s -- %s.\n", "-v", "Same as \"-g voters\"");
	printf("\t%-10s -- %s.\n", "-l", "Print members one per line (the default)");
	printf("\t%-10s -- %s.\n", "-s", "Print multiple members per line");
	printf("\t%-10s -- %s.\n", "-c", "Print a count of users instead of displaying them");
	printf("\n");
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -clsh ] [ -iumv | -g group ].\n", prog);
	exit(EXIT_FAILURE);
}
