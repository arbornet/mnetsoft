/*
 * Test vector.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"

FILE *logfp = stderr;

int
main(void)
{
	testroboserver();

	return(EXIT_SUCCESS);
}
