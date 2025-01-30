/**
 * @name setmyshell.c
 *
 * @memo Set a newly validate user's shell to something real.
 *
 * $Id: setmyshell.c 1573 2017-05-18 19:35:07Z cross $
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <grp.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

const char *NEWLY_VALIDATED_SHELL = "/cyberspace/bin/newly-validated";
const char *OPTIONS = "v?";
const char *NOT_NEWLY_VALIDATED_ERROR =
"Sorry, you can only run this program if you are newly validated.\n"
"Please run chsh instead.\n";
const char *VALIDATED_GROUP = "people";
const char *VERIFIED_GROUP = "verified";

char *prog;
int verbose;

static void usage(char *);
static void process(char *);

int
main(int argc, char *argv[])
{
	int ch;

	prog = progname(argv[0]);
	verbose = 0;
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case 'v':
			verbose = 1;
			break;
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage(prog);
	process(argv[0]);

	return (EXIT_SUCCESS);
}

static void
process(char *shell)
{
	struct passwd *pw;
	struct group *gr;
	gid_t pgid, vgid;

	pw = getpwuid(getuid());
	if (pw == NULL) {
		perror("Who are you?");
		exit(EXIT_FAILURE);
	}
	if (!STREQ(pw->pw_shell, NEWLY_VALIDATED_SHELL)) {
		fprintf(stderr, "%s", NOT_NEWLY_VALIDATED_ERROR);
		exit(EXIT_FAILURE);
	}
	gr = getgrnam(VALIDATED_GROUP);
	if (gr == NULL) {
		perror("Validated group is unknown?");
		exit(EXIT_FAILURE);
	}
	pgid = gr->gr_gid;
	gr = getgrnam(VERIFIED_GROUP);
	if (gr == NULL) {
		perror("Verified group is unknown?");
		exit(EXIT_FAILURE);
	}
	vgid = gr->gr_gid;
	if ((pgid != pw->pw_gid) && (vgid != pw->pw_gid)) {
		fprintf(stderr, "You are not validated.  Goodbye.\n");
		exit(EXIT_FAILURE);
	}
	if (!isshellvalid(shell)) {
		fprintf(stderr,
		      "Sorry, the shell \"%s\" is not valid.  Try again.\n",
			shell);
		exit(EXIT_FAILURE);
	}
	setuid(geteuid());
	if (verbose)
		printf("Setting your shell to %s....\n", shell);
	execl("/usr/bin/chsh", "chsh", "-s", shell, pw->pw_name, (const char *) NULL);
	perror("exec chsh");
	exit(EXIT_FAILURE);
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ XXX options ].\n", prog);
	exit(EXIT_FAILURE);
}
