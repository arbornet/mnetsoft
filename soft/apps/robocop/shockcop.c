#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#define SIGWAKE SIGHUP

int             sig[2] = {SIGWAKE, SIGTERM};
char           *action[2] = {"awaken", "kill"};

main(int argc, char **argv)
{
	FILE           *fp;
	int             copid = 0;
	int             mission;

	if (argc == 1)
		mission = 0;
	else if (argc == 2 && !strcmp(argv[1], "-k"))
		mission = 1;
	else {
		printf("Usage: %s [-k]\n", argv[0]);
		exit(1);
	}

	if ((fp = fopen(PIDFILE, "r")) == NULL) {
		printf("Cannot find robocop - %s not present\n", PIDFILE);
		exit(1);
	}
	fscanf(fp, "%d", &copid);
	if (copid < 2) {
		printf("Cannot find robocop - %s is wierd\n", PIDFILE);
		exit(1);
	}
	if (kill(copid, sig[mission])) {
		printf("Cannot %s robocop %d - dead or inaccessible\n",
		       action[mission], copid);
		exit(1);
	}
	printf("Robocop %d %sed\n", copid, action[mission]);
	exit(0);
}
