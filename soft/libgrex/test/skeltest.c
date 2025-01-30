/*
 * Skeleton test driver using the minunit framework.
 *
 * $Id: skeltest.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stdarg.h>
#include <stdio.h>
#include "minunit.h"

int	mu_num_tests_run;
int	mu_num_tests_failed;

void
mu_print(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static int
test_something(void)
{
	mu_assert(1 == 1, "Error: 1 != 1!");
	return(0);
}

static int
test_something_else(void)
{
	mu_assert(2 == 2, "Error: 2 != 2!");
	return(0);
}

static int
run_all_tests(void)
{
	mu_run_test(test_something);
	mu_run_test(test_something_else);
	return(mu_num_tests_failed);
}

int
main(void)
{
	int res;

	mu_num_tests_run = 0;
	mu_num_tests_failed = 0;
	res = run_all_tests();
	if (res == 0)
		printf("ALL TESTS PASSED.\n");
	printf("Number of tests run: %d, failed: %d\n", mu_num_tests_run, res);

	return(res != 0);
}
