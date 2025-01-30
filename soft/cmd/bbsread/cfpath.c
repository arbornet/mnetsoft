/*
 * CFPATH -- Given a conference name, this returns the directory name.  It
 * returns null if the conference does not exist.  It exits with an error
 * message if some other weird error occurs.  It is designed to be reusable.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification Dec  4, 1995 - Jan Wolter:  Expansion of % in confname to
 * confdir
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbsread.h"

char *expand_path(char *);


void
weird_path()
{
	fprintf(stderr, "%s: conference list %s in weird format\n",
		progname, CONFLIST);
	exit(1);
}


char *
cfpath(char *name)
{
	FILE *fp;
	static char buf[BFSZ];
	char *a, *b;

	if ((fp = fopen(CONFLIST, "r")) == NULL) {
		fprintf(stderr, "%s: could not open %s\n", progname, CONFLIST);
		exit(1);
	}
	/* Read and check magic code number */
	if (fgets(buf, BFSZ, fp) == NULL || strcmp(buf, "!<hl01>\n"))
		weird_path();

	/* Read and ignore default conference path */
	if (fgets(buf, BFSZ, fp) == NULL)
		weird_path();

	/* Search list of conferences */
	while (fgets(buf, BFSZ, fp) != NULL) {
		if (buf[0] == '#' || buf[0] == '\n')
			continue;

		if (match(name, buf)) {
			if ((a = index(buf, ':')) == NULL)
				weird_path();
			a++;
			if ((b = index(a, '\n')) == NULL)
				weird_path();
			*b = '\0';
			fclose(fp);
			return (expand_path(a));
		}
	}
	fclose(fp);
	return (NULL);
}


/*
 * SUBCHAR - Replace the first character of "buf" with the given string. No
 * checking for buffer overflow is done.
 */

void
subchar(char *buf, char *string)
{
	char *p = index(buf, '\0');
	int len = strlen(string) - 1;

	for (p = index(buf, '\0'); p > buf; p--)
		p[len] = *p;

	for (p = buf; *string != '\0'; p++, string++)
		*p = *string;
}


/*
 * EXPAND_PATH - Expand out % sign in path.
 */

char *
expand_path(char *path)
{
	char *p;

	if ((p = index(path, '%')) != NULL)
		subchar(p, CONFDIR);
	return (path);
}
