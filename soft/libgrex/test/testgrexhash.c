/*
 * Test libgrex `XXX' components.  See notes before `dothisssuite'
 * function definition for how to integrate this into the testrunner.
 *
 * $Id: testgrexhash.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stddef.h>
#include <string.h>

#include "minunit.h"
#include "testlibgrex.h"

#include "libgrex.h"

static int
test_grexhash_basic(void)
{
	const char *knownhash = "%!!CwIRH@^RNwvp]i-@`5WdM82'5";
	char *pass = "test", *user = "test", *h;
	char res[PASSRESLEN];
	h = grexhash_r(pass, user, res, sizeof(res));
	mu_assert(STREQ(h, knownhash),
	    "test_grexhash_basic: h (%s) != knownhash (%s)", h, knownhash);
	return(0);
}

int
dotestgrexhash(void)
{
	mu_run_test(test_grexhash_basic);
	return(0);
}
