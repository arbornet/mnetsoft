/*
 * FINDCONF -- This takes the name of a PicoSpan conference as an argument
 * and returns the pathname of the conference.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <stdio.h>

char *progname;

char *cfpath();
void exit();

int
main(int argc, char **argv)
{
	char *path;

	progname = argv[0];

	if (argc != 2) {
		fprintf(stderr, "usage: %s <conference name>\n", progname);
		exit(1);
	}
	if ((path = cfpath(argv[1])) == NULL) {
		fprintf(stderr, "%s: conference %s does not exist\n", progname,
			argv[1]);
		exit(1);
	}
	puts(path);
	exit(0);
}
