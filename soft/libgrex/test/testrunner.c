/*
 * Test harness for running libgrex tests.
 *
 * $Id: testrunner.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "minunit.h"
#include "testlibgrex.h"

int	mu_num_tests_run;
int	mu_num_tests_failed;

static int
run_all_suites(void)
{
	mu_run_suite(dotestutils);
	mu_run_suite(dotestgrexhash);
	mu_run_suite(dotestisshellok);
	mu_run_suite(dotestcmpstring);
	/*mu_run_suite(dotestwarnerr);*/
	return(mu_num_tests_failed);
}

void
mu_print(const char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	fprintf(stderr, "ERROR: ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

int
main(void)
{
	int res;

	mu_num_tests_run = 0;
	mu_num_tests_failed = 0;
	res = run_all_suites();
	if (res == 0)
		printf("ALL TESTS PASSED.\n");
	printf("Tests run: %d, total failures: %d\n", mu_num_tests_run, res);

	return(res != 0);
}
