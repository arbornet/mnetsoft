/*
 * Test libgrex `isshellok' function.
 *
 * $Id: testisshellok.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stddef.h>

#include "minunit.h"
#include "testlibgrex.h"

#include "libgrex.h"

static int
test_isshellvalid_csh(void)
{
	mu_assert(isshellvalid("/bin/csh"),
	    "test_isshellvalid_csh: /bin/csh not ok");
	return(0);
}

static int
test_isshellvalid_nologin(void)
{
	mu_assert(!isshellvalid("/sbin/nologin"),
	    "test_isshellvalid_nologin: /sbin/nologin ok");
	return(0);
}

static int
test_isshellok_root(void)
{
	mu_assert(isshellok(0), "test_isshellok_root: root shell not ok");
	return(0);
}

static int
test_isshellok_daemon(void)
{
	mu_assert(!isshellok(1), "test_isshellok_daemon: daemon shell ok");
	return(0);
}

int
dotestisshellok(void)
{
	mu_run_test(test_isshellvalid_csh);
	mu_run_test(test_isshellvalid_nologin);
	mu_run_test(test_isshellok_root);
	mu_run_test(test_isshellok_daemon);
	return(0);
}
