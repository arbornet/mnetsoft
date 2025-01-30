/*
 * FORGET -- Given a conference name and an item number, mark that item
 * forgotten.
 *
 * Dec  3, 1995 - Jan Wolter:  Original version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bbsread.h"

char *progname;


int
main(int argc, char **argv)
{
	int i;
	int item_number = -1;
	char *confdir, *confname = NULL;
	char pfname[BFSZ];
	char item_name[BFSZ];
	int mode, join_conf = 0;
	int resp_number;
	bool forgotten;

	progname = argv[0];
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			switch (argv[i][1]) {
			case 'c':
				join_conf = 1;
				break;
			default:
				goto usage;
			}
		else if (confname == NULL)
			confname = argv[i];
		else if (item_number == -1)
			item_number = atoi(argv[i]);
		else
			goto usage;
	}
	if (confname == NULL || item_number <= 0)
		goto usage;

	/* Get the path name for the conference */
	if (confname[0] == '/')
		confdir = confname;
	else if ((confdir = cfpath(confname)) == NULL) {
		printf("%s: conference %s does not exist\n", progname, confname);
		exit(1);
	}
	/* Check if the item exists */
	snprintf(item_name, sizeof item_name, "%s/_%d", confdir, item_number);
	if (access(item_name, 0)) {
		/* Check if the conference exists */
		snprintf(item_name, sizeof item_name, "%s/config", confdir);
		if (access(item_name, 0))
			printf("%s: directory %s does not exist\n", progname, confdir);
		else
			printf("%s: no item %d in the %s conference\n", progname,
			       item_number, confname);
		exit(1);
	}
	/* Get particpation file name */
	readconfig(confdir, pfname, &mode);

	/* Load the participation file */
	if (pfread(pfname)) {
		if (join_conf)
			pfnew(pfname);
		else {
			printf("%s: You are not a member of the %s conference\n"
			 "Do '%s -c %s %d' to join %s and forget item %d\n",
			progname, confname, progname, confname, item_number,
			       confname, item_number);
			exit(1);
		}
	}
	/* Fetch status of item */
	last_read(item_number, &resp_number, &forgotten);

	if (forgotten) {
		update_item(item_number, -1);

		printf("Remembering %s conference item %d\n",
		       confname, item_number);
	} else {
		/* Mark item read if not already read -- dubious but OK */
		/* Don't do this - update_item does it for Yapp and you */
		/* don't need to do if for Picospan                     */
		/* if (resp_number == -1) update_item(item_number, 0);   */

		/* Mark the item forgotten */
		update_item(item_number, -1);

		printf("Forgetting %s conference item %d\n",
		       confname, item_number);
	}

	/* Write the participation file */
	pfwrite();

	exit(0);

usage:
	printf("usage: %s [-c] <conference> <item>\n", progname);
	exit(1);
}
