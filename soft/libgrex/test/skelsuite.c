/**
 * @name skelsuite.c
 *
 * Test libgrex `XXX' components.  See notes before `dothisssuite'
 * function definition for how to integrate this into the testrunner.
 *
 * $Id: skelsuite.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stddef.h>

#include "minunit.h"
#include "testlibgrex.h"

#include "libgrex.h"

static int
test_something(void)
{
	mu_assert(1 == 1, "Error: test_something");
	return(0);
}

/**
 * NOTE: Make this name meaningful, add a prototype
 * to testlibgrex.h, and add a call to mu_run_suite(dothissuite);
 * in the `run_all_tests()' function in testrunner.c.
 * Finally, adjust the Makefile accordingly, adding thisfile.$O
 * to the OBJS= line.
 */
int
dothissuite(void)
{
	mu_run_test(test_something);
	return(0);
}
