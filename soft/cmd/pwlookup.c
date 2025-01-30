/*
 * $Id: pwlookup.c 1485 2014-05-21 14:18:50Z cross $
 */
#include <err.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

char *prog;

static int
display(struct passwd * p, char *u, char *prog)
{
	char *c;

	if (p == NULL) {
		warning((u == NULL) ? "Who are you?" : "User %s not found.", u);
		return (1);
	}
	if (STREQ(prog, "pwlookup") || STREQ(prog, "fullname")) {
		c = strchr(p->pw_gecos, ',');
		if (c != NULL)
			*c = '\0';
		printf("%s\n", p->pw_gecos);
	}
	if (STREQ(prog, "pwlookup") || STREQ(prog, "homedir"))
		printf("%s\n", p->pw_dir);
	if (STREQ(prog, "pwlookup") || STREQ(prog, "shell"))
		printf("%s\n", p->pw_shell);
	return (0);
}

int
main(int argc, char *argv[])
{
	int ret;

	prog = progname(argv[0]);
	ret = EXIT_SUCCESS;
	if (argc == 1) {
		ret += display(getpwuid(getuid()), "Who are you?", prog);
	} else
		while (*++argv)
			ret += display(getpwnam(*argv), *argv, prog);

	return (ret);
}
