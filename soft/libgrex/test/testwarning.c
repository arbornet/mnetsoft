/*
 * Test the warning() function from libgrex.
 * This isn't really unit testable, so isn't in the `normal'
 * testrunner.
 *
 * $Id: testwarning.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stdio.h>
#include <stdlib.h>

#include "libgrex.h"

const char *file = "/file/that/does/not/exist";

int
main(void)
{
	FILE	*fp;

	fprintf(stderr, "TESTWARNING: IGNORE ERRORS UNTIL TOLD OTHERWISE\n");
	fp = fopen(file, "r");
	if (fp == NULL)
		warning("Can't open %s: %r %r %r", file);
	else
		fclose(fp);
	fprintf(stderr, "TESTWARNING: TESTING COMPLETE\n");

	return(EXIT_SUCCESS);
}
