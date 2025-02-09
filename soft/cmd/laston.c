#include <sys/types.h>

#include <fcntl.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utmpx.h>

#include "libgrex.h"

char *prog;

static int isonline(const char *);
static void laston(const char *);
static void usage(const char *);

int
main(int argc, char *argv[])
{
	int i;

	prog = progname(argv[0]);
	while ((i = getopt(argc, argv, "v?")) != -1) {
		switch (i) {
		case 'v':
			printf("laston -- version 2.0  26-Feb-2013\n");
			exit(EXIT_SUCCESS);
			break;
		case '?':
		default:
			usage(prog);
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if (argc <= 0)
		usage(prog);
	for (i = 0; i < argc; i++)
		laston(argv[i]);

	return (EXIT_SUCCESS);
}

static int
isonline(const char *account)
{
	struct utmpx *u;

	setutxent();
	u = getutxuser(account);
	endutxent();

	return (u != NULL);
}

static void
laston(const char *account)
{
	struct utmpx *u;
	time_t lltime;
	char tmp[32];

	if (getpwnam(account) == NULL) {
		printf("%s does not exist\n", account);
		return;
	}
	setutxdb(UTXDB_LASTLOGIN, NULL);
	u = getutxuser(account);
	endutxent();
	if (u == NULL || u->ut_tv.tv_sec == 0L) {
		printf("%-12s no record of login\n", account);
		return;
	}

	lltime = u->ut_tv.tv_sec;
	strlcpy(tmp, ctime(&lltime), sizeof(tmp));
	tmp[strlen(tmp) - 1] = 0;

	printf("%-12s %s at %s", account, u->ut_line, tmp);
	if (u->ut_host[0] != '\0' && u->ut_host[0] != ' ')
		printf(" from %.*s", (int) sizeof(u->ut_host), u->ut_host);
	if (isonline(account))
		printf(" (on line)");
	printf("\n");
}

static void
usage(const char *prog)
{
	fprintf(stderr, "Usage: %s user[s].\n", prog);
	exit(EXIT_FAILURE);
}
