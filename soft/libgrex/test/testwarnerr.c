/**
 * @name testwarnerr.c
 *
 * Test libgrex warning and error components.
 *
 * $Id: testwarnerr.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "minunit.h"
#include "testlibgrex.h"

#include "libgrex.h"

char *expected;
void (*func)(void);

static int
test_warning(void)
{
	int	fds[2];
	int	pid, ret;
	char	fromchld[10240];	/* 10KB from child. */

	if (pipe(fds) < 0) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}
	pid = fork();
	if (pid < 0) {
		perror("can't fork");
		exit(EXIT_FAILURE);
	}
	if (pid == 0) {		/* Child */
		close(fds[0]);
		dup2(fds[1], 2);
		func();
		exit(0);
	}
	close(fds[1]);
	memset(fromchld, 0, sizeof(fromchld));
	ret = read(fds[0], fromchld, sizeof(fromchld) - 1);
	if (ret < 0) {
		perror("read error");
	}
	close(fds[0]);
	mu_assert(STREQ(expected, fromchld),
	    "Error: test_warning: Got a bad value: %s != %s\n", fromchld, expected);
	return(0);
}

static void
dowarn(char *exp, char *g, void (*f)(void))
{
	expected = exp;
	func = f;
	mu_run_test(test_warning);
}

/**
 * NOTE: Make this name meaningful, add a prototype
 * to testlibgrex.h, and add a call to mu_run_suite(dothissuite);
 * in the `run_all_tests()' function in testrunner.c.
 * Finally, adjust the Makefile accordingly, adding thisfile.$O
 * to the OBJS= line.
 */
void
f1(void)
{
	errno = ENOENT;
	warning("test1: %r");
}

void
f2(void)
{
	char	fmt[1024];
	errno = ENOENT;
	memset(fmt, 'a', sizeof(fmt));
	fmt[sizeof(fmt) - 3] = '%';
	fmt[sizeof(fmt) - 2] = 'r';
	fmt[sizeof(fmt) - 1] = '\0';
	warning(fmt);
}

int
dothissuite(void)
{
	/*dowarn("test1");*/
	return(0);
}
