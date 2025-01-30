#include <stdlib.h>

#include "zapuser.h"

#define CMD_END		0
#define CMD_STRING	1	/* Store one arg in (char **)val */
#define CMD_INT		2	/* atoi one arg and store in (int *)val */
#define CMD_FUNC	3	/* Call void (*val)(char *) on each arg */

struct cmd {
	char *cmd;
	int type;
	void *val;
} cmds[] = {
	{
		"logfile", CMD_STRING, (void *) &log_file
	},
	{
		"errfile", CMD_STRING, (void *) &err_file
	},
	{
		"immortalfile", CMD_STRING, (void *) &immortal_file
	},
	{
		"mathomdir", CMD_STRING, (void *) &mathom_dir
	},
	{
		"outboundfile", CMD_STRING, (void *) &outbound_file
	},
	{
		"minuid", CMD_INT, (void *) &min_uid
	},
	{
		"protectgroup", CMD_FUNC, (void *) &protect_group
	},
	{
		"homedirprefix", CMD_FUNC, (void *) &dir_prefix
	},
	{
		NULL, CMD_END, NULL
	}
};


/*
 * FIRSTIN, FIRSTOUT Jan's habitual parsing routines.  Find first character
 * of string s that does (or doesn't) appear in the string l.  On failure,
 * point to terminating nil character of s.
 */

char *
firstin(char *s, char *l)
{
	return s + strcspn(s, l);
}

char *
firstout(char *s, char *l)
{
	return s + strspn(s, l);
}


/*
 * READ_CONFIG Load configuration information from configuration file.
 */

void
read_config()
{
	FILE *fp;
	char bf[BFSZ];
	char *b, *e, *a;
	int i;

	if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
		fprintf(stderr, "cannot open configuration file " CONFIG_FILE "\n");
		exit(1);
	}
	while (fgets(bf, BFSZ, fp) != NULL) {
		if (bf[0] == '#')
			continue;
		b = firstout(bf, " \t\n\r");
		if (*b == '\0')
			continue;
		e = firstin(b, " \t\n\r");
		a = firstout(e, " \t\n\r");
		*e = '\0';

		for (i = 0; cmds[i].type != CMD_END; i++) {
			if (!strcasecmp(b, cmds[i].cmd)) {
				switch (cmds[i].type) {
				case CMD_STRING:
					e = firstin(a, " \t\n\r");
					*e = '\0';
					*((char **) cmds[i].val) = (*a == '\0') ? NULL : strdup(a);
					break;

				case CMD_INT:
					if (*a < '0' || *a > '9') {
						error("%s command in " CONFIG_FILE
						      " should have integer argument", cmds[i].cmd);
						continue;
					}
					*((int *) cmds[i].val) = atoi(a);
					break;

				case CMD_FUNC:
					while (*a != '\0') {
						e = firstin(a, " \t\n\r");
						b = firstout(e, " \t\n\r");
						*e = '\0';
						(*(void (*) ()) cmds[i].val) (a);
						a = b;
					}
					break;
				}
				break;
			}
		}
		if (cmds[i].type == CMD_END)
			error("unknown command in " CONFIG_FILE ": %s", b);
	}
}
