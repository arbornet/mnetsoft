/*
 * EXTRACT -- This is the main routine for a program to print out ranges of
 * response from Picospan items.  It never touches the users configuration
 * files and is basically a stripped-down version of bbsread.
 *
 * Dec  4, 1995 - Jan Wolter:  Original version
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbsread.h"

char *progname;


void
usage()
{
	fprintf(stderr, "usage: %s [-h] <confname> <item> [<resp>[-<resp>]|head]\n",
		progname);
	exit(1);
}


int
main(int argc, char **argv)
{
	char *confname;
	char *confdir;
	int curitem, max_resp;
	int header_only = false;
	int html = false;
	int i, j;

	progname = argv[0];

	/* Set various things to illegal values */
	confname = NULL;
	item.first = -1;
	resp.first = -1;

	/* Parse the arguments */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j] != '\0'; j++)
				switch (argv[i][j]) {
				case 'h':
					html = true;
					break;
				default:
					usage();
				}
		else if (confname == NULL)
			confname = argv[i];
		else if (item.first == -1)
			parse_range(&item, argv[i]);
		else if (resp.first == -1) {
			if (!strncmp(argv[i], "head", 4))
				header_only = true;
			else
				parse_range(&resp, argv[i]);
		} else
			usage();
	}
	if (confname == NULL || item.first < 1 || item.first != item.last)
		usage();
	if (resp.first == -1) {
		resp.first = 0;
		resp.last = -1;
	}
	/* Get the directory name */
	if (confname[0] == '/')
		confdir = confname;
	else if ((confdir = cfpath(confname)) == NULL) {
		fprintf(stderr, "%s: conference %s does not exist\n", progname,
			confname);
		exit(1);
	}
	/* Get response information from the sum file */
	open_sum(confdir);
	seek_item(item.first);
	if (next_item(&curitem, &max_resp) || curitem != item.first) {
		printf("%s: No item %d in the %s conference\n",
		       progname, item.first, confname);
		exit(1);
	}
	/* Display the responses */
	dispitem(stdout, confdir, item.first, resp.first, resp.last, --max_resp,
		 header_only ? 2 : (resp.first == 0), html);

	exit(0);
}

/* Dummy stuff */

void
lost_items(int a, int b)
{
}
int read_forgotten;
