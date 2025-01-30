/*
 * Restricted login shell, for brand-new users.
 *
 * $Id: resh.c 1561 2017-05-13 20:36:52Z cross $
 */

#include <sys/stat.h>
#include <sys/types.h>

#include <pwd.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"
#include "resh.h"

char *env[] = {
	"SHELL=resh",
	"EDITOR=reshe",
	"PAGER=pg",
	"PATH=/cyberspace/grex/resh/bin:/cyberspace/bin",
	NULL
};

void
die(int signo)
{
	if (signo != SIGHUP)
		printf("Goodbye.\n");
	exit(EXIT_SUCCESS);
}

void
initsigs(void)
{
	int i;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	for (i = 0; i < NSIG; i++)
		sigaction(i, &sa, NULL);
	sa.sa_handler = die;
	sigaction(SIGHUP, &sa, NULL);
	sa.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sa, NULL);
}

#define MAX(a, b) (((a) < (b)) ? (b) : (a))
void
initfds(void)
{
	int fd;

	fd = MAX(STDERR_FILENO, STDIN_FILENO);
	fd = MAX(fd, STDOUT_FILENO);

	for (fd = fd + 1; fd < sysconf(_SC_OPEN_MAX); fd++)
		(void)close(fd);
}

void
initdir(void)
{
	struct passwd *pw;

	pw = getpwuid(getuid());
	if (pw == NULL) {
		fprintf(stderr, "Who are you?\n");
		exit(EXIT_FAILURE);
	}
	if (chdir(pw->pw_dir) != 0) {
		perror(pw->pw_dir);
		exit(EXIT_FAILURE);
	}
}

void
initenv(void)
{
	extern volatile char **environ;
	char **pp, *tz, *term;
	int i;

	tz = getenv("TZ");
	term = getenv("TERM");
	environ = calloc(1, sizeof(char *));
	if (environ == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	for (pp = env; *pp; pp++)
		if (putenv(*pp) < 0) {
			perror("putenv");
			printf("%s\n", *pp);
			exit(EXIT_FAILURE);
		}
	if (tz != NULL)
		setenv("TZ", tz, 1);
	if (term != NULL)
		setenv("TERM", term, 1);
}

void
initpasswd(void)
{
	struct stat sb;

	memset(&sb, 0, sizeof(sb));
	if (stat(".needpwchange", &sb) == 0) {
		char *cmd[] = { "passwd", NULL };
		run_command(cmd[0], 1, cmd);
		unlink(".needpwchange");
	}
}


void
resh_init(void)
{
	initsigs();
	initfds();
	initdir();
	initenv();
	initpasswd();

	printf("Welcome to Grex.  This is resh.  Type 'help' if you are lost.\n");
}

struct rcommand *
resh_parse(char *str)
{
	struct rcommand *c;

	c = malloc(sizeof(*c));
	memset(c, 0, sizeof(*c));
	c->argc = tokenizeq(str, c->argv, NELEM(c->argv) - 1);
	c->argv[c->argc] = NULL;

	return(c);
}

struct rresult *
resh_eval(struct rcommand *cmd)
{
	char *c;

	c = cmd->argv[0];
	if (c == NULL)
		return(NULL);
	if (STREQ(c, "exit") ||
	    STREQ(c, "quit") ||
	    STREQ(c, "logoff") ||
	    STREQ(c, "logout") ||
	    STREQ(c, "bye"))
	{
		printf("Goodbye.\n");
		exit(EXIT_SUCCESS);
	}
	if (STREQ(c, "reboot")) {
		printf("Rebooting....\n");
		printf("resh: Terminated by signal 15.\n");
		exit(EXIT_SUCCESS);
	}
	if (!dynegg(c))
		run_command(c, cmd->argc, cmd->argv);

	return malloc(1);
}

void
resh_print(struct rresult *result)
{
}

void
resh_cleanup(void)
{
	int fd = openr("/etc/motd");
	close(fd);
}

int
main(void)
{
	struct rcommand *cmd;
	struct rresult *res;
	char *str;

	resh_init();
	for ( ; ; ) {
		str = resh_read("grex; ");
		if (str == NULL)
			break;
		cmd = resh_parse(str);
		if (cmd == NULL)
			break;
		res = resh_eval(cmd);
		if (res == NULL)
			continue;
		resh_print(res);
		free(res);
		free(cmd);
		free(str);
	}
	resh_cleanup();

	return(EXIT_SUCCESS);
}
