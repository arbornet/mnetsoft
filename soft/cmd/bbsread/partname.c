/*
 * PARTNAME -- Return the name of a participation file of a conference.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <stdio.h>

char *progname;


char *cfpath(char *);
void readconfig(char *, char *, int *);
void exit();

int
main(int argc, char **argv)
{
	char *path;
#define BFSZ 1024
	char part[BFSZ];
	int mode;

	progname = argv[0];

	if (argc != 2) {
		fprintf(stderr,
			"usage: %s <conference name>|<conference dir>\n",
			progname);
		exit(1);
	}
	/* Get the path name for the conference */
	if (argv[1][0] == '/')
		path = argv[1];
	else {
		if ((path = cfpath(argv[1])) == NULL) {
			fprintf(stderr, "%s: conference %s does not exist\n",
				progname, argv[1]);
			exit(1);
		}
	}

	readconfig(path, part, &mode);

	printf("%s\n", part);

	exit(0);
}
