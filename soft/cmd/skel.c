/**
 * @name skel.c
 *
 * A sample program that illustrates how to use some of the
 * features of libgrex.
 *
 * @memo Brief description of this program.
 *
 * $Id: skel.c 1485 2014-05-21 14:18:50Z cross $
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

const char *OPTIONS = "?";

char *prog;

static void usage(char *);
static int process(FILE *);

int
main(int argc, char *argv[])
{
	FILE *fp;
	char *filename;
	int ch;
	struct stat sb;

	prog = progname(argv[0]);
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		/* XXX - Your options here. */
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;

	/*
 	 * Loop over arguments, opening input files
	 * and processing them.
	 */
	if (argc == 0) {
		process(stdin);
	} else while ((filename = *argv++) != NULL) {
		if (STREQ(filename, "-")) {
			process(stdin);
			continue;
		}
		/*
		 * The next few stanzas make sure that the named
		 * file is regular, skipping the file otherwise.
		 * The case for directories is handled specially.
		 * Remove this code if this behavior is not needed
		 * or undesirable for other reasons.
		 */
		memset(&sb, 0, sizeof(sb));
		if (stat(filename, &sb) < 0) {
			warning("Couldn't stat %s: %r", filename);
			continue;
		}
		if ((sb.st_mode & S_IFMT) == S_IFDIR) {
			warning("Skipping %s (directory).", filename);
			continue;
		}
		if ((sb.st_mode & S_IFMT) != S_IFREG) {
			warning("Skipping %s (not a regular file).", filename);
			continue;
		}
		/*
		 * Assuming everything is fine, open the file and processit.
		 */
		if ((fp = fopen(filename, "r")) == NULL) {
			warning("Couldn't open %s: %r", filename);
			continue;
		}
		process(fp);
		fclose(fp);
	}

	return(EXIT_SUCCESS);
}

static int
process(FILE * fp)
{
	/* XXX */

	return(0);
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ XXX options ].\n", prog);
	exit(EXIT_FAILURE);
}
