/**
 * @name minunit
 *
 * A Simple, Minimalist Unit Testing Framework.
 *
 * \Ref{minunit}
 *
 * To use this framework, you must have something similar
 * to the following in a file you're linking against:
 *
 * \begin{verbatim}
 *
 * #include <stdarg.h>
 * #include <stddef.h>
 * #include <stdio.h>
 *
 * #include "minunit.h"
 *
 * int mu_num_tests_run = 0, mu_num_tests_failed = 0;
 *
 * void
 * mu_print(const char *fmt, ...)
 * {
 * 	va_list ap;
 * 
 * 	va_start(ap, fmt);
 * 	fprintf(stderr, "ERROR: ");
 * 	vfprintf(stderr, fmt, ap);
 * 	fprintf(stderr, "\n");
 * 	va_end(ap);
 * }
 *
 * \end{verbatim}
 *
 * Write unit tests (usually in another source module, though
 * that's not necessary) as follows:
 *
 * \begin{verbatim}
 *
 * static int
 * test_something(void)
 * {
 * 	int bv = (1 == 1);
 * 	mu_assert(bv, "test_something: 1 != 1! bv == %d", bv);
 * 	mu_assert(2 != 1, "test_something 2 == 1?");
 * 	return(0);
 * }
 *
 * \end{verbatim}
 *
 * You then create a function that exposes a `suite' and runs
 * multiple tests:
 *
 * \begin{verbatim}
 *
 * int
 * dotestsuite(void)
 * {
 * 	mu_run_test(test_something);
 * 	return(0);
 * }
 *
 * \end{verbatim}
 *
 * mu_run_test() keeps track of how many tests have been
 * run and how many tests have failed (note that this is not
 * thread safe).
 *
 * Finally, your main test driver runs the various suites:
 *
 * \begin{verbatim}
 *
 * int
 * run_all_tests(void)
 * {
 * 	mu_run_suite(dotestsuite);
 *	return(mu_num_tests_failed);
 * }
 *
 * \end{verbatim}
 *
 * Your main() function may print out statistics or other
 * informational messages, if you so choose.  It is good
 * practice to add a `test:' target to your local Makefile
 * to build and run tests.
 */

#define mu_assert(test, ...) do { int r = (test); if (!r) { mu_print (__VA_ARGS__); return !r; } } while (0)
#define mu_run_test(test) do { int r = test(); mu_num_tests_run++; if (r) mu_num_tests_failed++; } while (0)
#define mu_run_suite(test) do { (test)(); } while (0)
extern void mu_print(const char *, ...);
extern int mu_num_tests_run;
extern int mu_num_tests_failed;
