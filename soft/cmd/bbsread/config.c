/*
 * Read conference Configuration files.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version May 29, 1993 - Jan Wolter:
 * Revised to read conference modes. Dec  2, 1995 - Jan Wolter:  Ansification
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bbsread.h"

#define CONFIGFILE	"/config"

/*
 * READCONFIG -- Given the directory path for a conference, return the name
 * of the participation file and the  mode of the conference.  Modes 4, 5 and
 * 6 indicate closed conferences of various kinds.  If the format of the file
 * is strange print an error message and exit.
 */

void
readconfig(char *dir, char *pfname, int *mode)
{
	FILE *fp;
	static char buf[BFSZ];
	char *b;

	strlcat(buf, dir, sizeof buf);
	strlcat(buf, CONFIGFILE, sizeof buf);

	if ((fp = fopen(buf, "r")) == NULL) {
		fprintf(stderr, "%s: could not open %s\n", progname, buf);
		exit(1);
	}
	/* Read and check magic code number */
	if (fgets(buf, BFSZ, fp) == NULL || strcmp(buf, "!<pc02>\n"))
		goto weird;

	/* Read participation file name */
	if (fgets(buf, BFSZ, fp) == NULL)
		goto weird;

	if ((b = index(buf, '\n')) == NULL)
		goto weird;
	*b = '\0';
	strlcpy(pfname, buf, BFSZ);

	/* Read the line that always has a zero on it */
	if (fgets(buf, BFSZ, fp) == NULL)
		goto weird;

	/* Read the list of fairwitnesses */
	if (fgets(buf, BFSZ, fp) == NULL)
		goto weird;

	/* Read the mode line */
	if (fgets(buf, BFSZ, fp) == NULL)
		*mode = 0;
	else
		*mode = atoi(buf);

	fclose(fp);
	return;

weird:
	fprintf(stderr, "%s: conference configuration file in weird format\n",
		progname);
	exit(1);
}
