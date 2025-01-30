#include <stdio.h>
#include <stdlib.h>

#include "zapuser.h"

/* List of acceptable home directory prefixes */

char **dirlist = NULL;
int dirn = 0;
int dirsz = 0;


/*
 * DIR_PREFIX Add the given directory prefix to the list of good directory
 * prefixes.
 */

void
dir_prefix(char *prefix)
{
	if (dirn >= dirsz) {
		dirsz = dirn + 10;
		dirlist = (char **) Realloc(dirlist, dirsz * sizeof(char *));
		if (dirlist == NULL) {
			fprintf(stderr, "out of memory for dir prefix list\n");
			exit(1);
		}
	}
	if (verbose > 1)
		printf("adding directory prefix %s\n", prefix);
	dirlist[dirn++] = strdup(prefix);
}


/*
 * DIR_OK Return true if the given directory is a legitimate home directory.
 */

int
dir_ok(char *dir)
{
	int i;

	if (dirn == 0)
		return 1;

	for (i = 0; i < dirn; i++) {
		if (!strncmp(dir, dirlist[i], strlen(dirlist[i])))
			return 1;
	}
	return 0;
}
