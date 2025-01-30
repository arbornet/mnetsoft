#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <limits.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libgrex.h"

const char *OPTIONS = "?";

char *prog;

static void usage(char *);

int
main(int argc, char *argv[])
{
	struct passwd *pw;
	struct stat st;
	struct timeval now;
	char *user, *err;
	uid_t uid;
	int ch, notok;

	prog = progname(argv[0]);
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case '?':
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;

	/* Make sure we have something to do. */
	if (argc == 0)
		usage(prog);

	/* Read the clock */
	gettimeofday(&now, (struct timezone *) NULL);

	/* Loop over users and get information about them. */
	while ((user = *argv++) != NULL) {
		uid = strtol(user, &err, 0);
		if (err != NULL && *err == '\0')
			pw = getpwuid(uid);
		else
			pw = getpwnam(user);
		if (pw == NULL) {
			fprintf(stderr, "%s: No such user\n", user);
			continue;
		}
		user = pw->pw_name;
		notok = 0;
		if (pw->pw_expire != 0 && now.tv_sec >= pw->pw_expire)
			notok += printf("%s: Account expired\n", user);
		if (pw->pw_change != 0 && now.tv_sec >= pw->pw_change)
			notok += printf("%s: Password expired\n", user);
		if (stat(pw->pw_dir, &st) != 0 || !(st.st_mode & S_IFDIR))
			notok += printf("%s: No home directory\n", user);
		printf("%s: %sOK\n", user, notok ? "Not " : "");
	}
	return (EXIT_SUCCESS);
}

static void
usage(char *prog)
{
	fprintf(stderr, "usage: %s < login(s) | uid(s) >.\n", prog);
	exit(1);
}
