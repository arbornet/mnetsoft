/*
 * Test libgrex `utility' components.
 *
 * $Id: testutils.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stddef.h>
#include <string.h>

#include "minunit.h"
#include "testlibgrex.h"

#include "libgrex.h"

static int
test_progname_basic(void)
{
	char *pn = progname("/path/to/file");
	mu_assert(STREQ(pn, "file"), "test_progname_basic: pn == %s", pn);
	return(0);
}

static int
test_progname_nopath(void)
{
	char *pn = progname("file");
	mu_assert(STREQ(pn, "file"), "test_progname_nopath: pn == %s", pn);
	return(0);
}

static int
test_progname_null(void)
{
	char *pn = progname(NULL);
	mu_assert(pn == NULL, "test_progname_null: pn == %s", pn);
	return(0);
}

static int
test_progname_dir(void)
{
	char *pn = progname("/path/to/dir/");
	mu_assert(STREQ(pn, ""), "test_progname_dir: pn == %s", pn);
	return(0);
}

int
dotestutils(void)
{
	mu_run_test(test_progname_basic);
	mu_run_test(test_progname_nopath);
	mu_run_test(test_progname_null);
	mu_run_test(test_progname_dir);
	return(0);
}
