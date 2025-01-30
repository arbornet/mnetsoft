/*
 * BBSREAD -- This is the main routine for a program to read PicoSpan
 * conferences with an PicoSpan-like user interface.  It understands and
 * updates the user's configuration files.
 *
 * Mar 16, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification Dec  3, 1995 - Jan Wolter:  response ranges on command line.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bbsread.h"

char *progname;
char *confdir;

bool dont_update = false;	/* Don't update the user's participation file */
bool dont_join = false;		/* Don't join the conf if not already in it */
bool dont_page = false;		/* Don't use the more pager */

void main_loop();
void usage();


int
main(int argc, char **argv)
{
	char *confname;
	char pfnam[BFSZ];
	int i, j;
	int mode;
	FILE *fp;

	progname = argv[0];
	confname = NULL;
	dont_page = !isatty(1);

	/* Parse the arguments */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j] != '\0'; j++)
				switch (argv[i][j]) {
				case 'm':
					dont_page = true;
					break;
				case 'p':
					dont_update = true;
					break;
				case 'j':
					dont_join = true;
					break;
				default:
					usage();
				}
		else if (confname == NULL)
			confname = argv[i];
		else
			parse_arg(argv[i]);
	}
	if (confname == NULL)
		usage();

	/* Get the directory name */
	if (confname[0] == '/')
		confdir = confname;
	else if ((confdir = cfpath(confname)) == NULL) {
		fprintf(stderr, "%s: conference %s does not exist\n", progname,
			confname);
		exit(1);
	}
	/* Load in the configuration file */
	readconfig(confdir, pfnam, &mode);

	/*
	 * In case we are suid, open only open conferences, or conferences
	 * which contain a readable "readonly" file.
	 */
	if (mode >= 4 && mode <= 6) {
		char buf[BFSZ];
		snprintf(buf, sizeof buf, "%s/readonly", confdir);
		if ((fp = fopen(buf, "r")) == NULL) {
			fprintf(stderr, "%s: %s conference is closed\n",
				progname, confname);
			exit(1);
		}
		fclose(fp);
	}
	/* Load in the participation file */
	if (!(read_brandnew && read_newresp == 2 && read_old && read_forgotten)) {
		/* Either read or create the part file */
		if (pfread(pfnam)) {
			if (!dont_join)
				pfnew(pfnam);
		}
	}
	/* Open up the sum file */
	open_sum(confdir);

	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, pipeintr);

	main_loop();

	if (!dont_update) {
		/* Must abandon cfadm status to be able to write to .cfdir */
		setuid(getuid());
		pfwrite();
	}
	return (0);
}


/*
 * MAIN_LOOP -- this is the main loop, which displays the selected items,
 * prompts for a command, and then executes that command.
 */
void
main_loop()
{
	char buf[BFSZ];
	int rc;

	/* Find sum-file entry for first item in range */
	seek_item(reverse ? item.last : item.first);

	if (extracting) {
		if (!do_extract())
			fprintf(stderr, "No such item\n");
		return;
	}
	if (!do_pass()) {
		fprintf(stderr, "No items selected for display\n");
		return;
	}
	do {
		if (prompt_rfp) {
			fprintf(stderr, "\nResponse not possible.  Pass? ");
			if (fgets(buf, BFSZ, stdin) == NULL) {
				putchar('\n');
				break;
			}
			rc = rfp_command(buf);
		} else
			rc = do_pass();
	} while (rc);
}
