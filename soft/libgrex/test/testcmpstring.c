/*
 * Test libgrex `cmpstring' function.
 *
 * $Id: testcmpstring.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stddef.h>
#include <string.h>

#include "minunit.h"
#include "testlibgrex.h"

#include "libgrex.h"

static int
test_cmpstring_match(void)
{
	const void *a = "HI", *b = "HI";
	mu_assert(cmpstring(a, b) == 0, "test_cmpstring_match: res != 0");
	return(0);
}

static int
test_cmpstring_mismatch1(void)
{
	const void *a = "HI", *b = "THERE";
	mu_assert(cmpstring(a, b) < 0, "test_cmpstring_mismatch1: res >= 0");
	return(0);
}

static int
test_cmpstring_mismatch2(void)
{
	const void *a = "HI", *b = "THERE";
	mu_assert(cmpstring(b, a) > 0, "test_cmpstring_mismatch2: res <= 0");
	return(0);
}

static int
test_cmpstring_empty1(void)
{
	const void *a = "", *b = "THERE";
	mu_assert(cmpstring(a, b) < 0, "test_cmpstring_empty1: res >= 0");
	return(0);
}

static int
test_cmpstring_empty2(void)
{
	const void *a = "", *b = "THERE";
	mu_assert(cmpstring(b, a) > 0, "test_cmpstring_empty2: res <= 0");
	return(0);
}

static int
test_cmpstring_null1(void)
{
	const void *a = NULL, *b = "THERE";
	mu_assert(cmpstring(a, b) == -1, "test_cmpstring_null1: res != -1");
	return(0);
}

static int
test_cmpstring_null2(void)
{
	const void *a = NULL, *b = "THERE";
	mu_assert(cmpstring(b, a) == 1, "test_cmpstring_null2: res != 1");
	return(0);
}

int
dotestcmpstring(void)
{
	mu_run_test(test_cmpstring_match);
	mu_run_test(test_cmpstring_mismatch1);
	mu_run_test(test_cmpstring_mismatch2);
	mu_run_test(test_cmpstring_empty1);
	mu_run_test(test_cmpstring_empty2);
	mu_run_test(test_cmpstring_null1);
	mu_run_test(test_cmpstring_null2);
	return(0);
}
