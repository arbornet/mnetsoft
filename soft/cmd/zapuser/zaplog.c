#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "zapuser.h"

char *log_file = NULL;
int silent = 0;

/* The following aren't used */
int min_uid = 500;
char *immortal_file = NULL;
char *outbound_file = NULL;
void
protect_group(char *gname)
{
}
char *err_file = NULL;
char *mathom_dir = NULL;
void
dir_prefix(char *prefix)
{
}

main(int argc, char **argv)
{
	struct rlimit rlim;
	char *userid = NULL;
	FILE *fp;
	int i, j, cnt = 0;
	int amroot = 0;
#define BFSZ 1024
	char bf[1024];
	char *p, *e;

	/* Don't dump core (could encrypted passwords from old accounts) */
	rlim.rlim_cur = rlim.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &rlim);

	/* Parse parameters */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			for (j = 1; argv[i][j] != '\0'; j++)
				switch (argv[i][j]) {
				case 's':
					silent = 1;
					break;
				default:
					goto usage;
				}
		else if (userid == NULL)
			userid = argv[i];
		else
			goto usage;
	}
	if (userid == NULL)
		goto usage;

	amroot = (getuid() == 0);

	/* Load config file */
	read_config();

	/* Check if we have log file name */
	if (log_file == NULL) {
		if (!silent)
			fprintf(stderr, "Configuration file doesn't define log file name\n");
		exit(1);
	}
	/* Open the log file */
	if ((fp = fopen(log_file, "r")) == NULL) {
		if (!silent)
			fprintf(stderr, "Cannot open log file %s to read\n", log_file);
		exit(1);
	}
	/* Read through the log file */
	while (fgets(bf, BFSZ - 1, fp) != NULL) {
		/* Find login name and check if it matches */
		if ((p = strchr(bf, '[')) == NULL)
			continue;
		p++;
		if ((e = strchr(p, ':')) == NULL)
			continue;
		*e = '\0';
		if (strcmp(p, userid))
			continue;

		/* Find full name */
		p = strchr(e + 1, ':');
		if (p)
			p = strchr(p + 1, ':');
		if (p)
			p = strchr(p + 1, ':');
		if (p) {
			p++;
			e = firstin(p, ":");
			*e = '\0';
		}
		/* Find reason */
		if (p) {
			e = firstin(e + 1, "]");
			e = firstout(e, "] ");
		}
		printf("Account %s (%s) deleted on %.20s.  Reason: %s",
		       userid, p ? p : "", bf, p ? e : "unknown");
		cnt++;
	}
	close(fp);

	if (cnt == 0 && !silent)
		printf("No record found of deletion of login %s\n", userid);
	exit(0);

usage:
	fprintf(stderr, "Usage: %s [-s] <userid>\n", argv[0]);
	exit(1);
}


void
error(char *fmt,...)
{
	va_list ap;

	va_start(ap, fmt);
	if (!silent)
		vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}
