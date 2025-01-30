/*
 * `Shell' to be used when running grexmore as the interpreter for
 * scripts that don't do anything except print text files.
 *
 *	Dan Cross <cross@cyberspace.org>
 *
 * $Id: grexmoresh.c 1485 2014-05-21 14:18:50Z cross $
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	char **av;

	argc++;			/* To include argv[argc] == NULL */
	av = malloc((argc + 2 + 1) * sizeof(char *));
	if (av == NULL) {
		perror("Can't malloc");
		exit(EXIT_FAILURE);
	}
	memmove(&av[3], &argv[1], (argc + 1 - 1) * sizeof(char *));
	av[0] = "grexmore";
	av[1] = "-n";
	av[2] = "2";
	execvp("grexmore", av);
	perror("execvp of grexmore failed");

	return (EXIT_FAILURE);
}
