/**
 * Copyright (C) 2010  Cyberspace Communications, Inc.
 * All Rights Reserved
 *
 * $Id: genbsalt.c 1566 2017-05-16 20:36:07Z cross $
 *
 * @name genbsalt.c
 *
 * A simple program to generate a salt for use with the blowfish
 * hashing method in OpenBSD's extended crypt(3).
 *
 * @memo Generate a salt suitable for OpenBSD's extended crypt(3).
 */

#include <libgen.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum
{
	DEFAULT_ROUNDS = 8,
	MIN_ROUNDS = 4,
	MAX_ROUNDS = 31
};

const char *OPTIONS = "n:?";
static void usage(char *);

char *prog;

int
main(int argc, char *argv[])
{
	char	*e;
	long	rounds;
	int	ch;

	prog = basename(argv[0]);
	rounds = DEFAULT_ROUNDS;
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case 'n':
			e = NULL;
			rounds = strtol(optarg, &e, 0);
			if (e != NULL && *e != '\0') {
				fprintf(stderr, "Bad integer: %s.\n", optarg);
				usage(prog);
			}
			break;
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage(prog);
	if (rounds < MIN_ROUNDS)
		rounds = MIN_ROUNDS;
	if (rounds > MAX_ROUNDS)
		rounds = MAX_ROUNDS;
	printf("%s\n", bcrypt_gensalt((int)rounds));

	return(EXIT_SUCCESS);
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s [ -n rounds ].\n", prog);
	exit(EXIT_FAILURE);
}
