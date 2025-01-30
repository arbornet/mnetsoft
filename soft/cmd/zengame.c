/*
 * zengame: a Zen game
 *
 * STeve Andre 25-May-96.  I lost the original version of this, so
 *                         I made a new one.
 *
 *                         No documentation is provided.  If you have
 *                         to ask what it does, you aren't old enough
 *                         to play it.
 *
 * $Id: zengame.c 1601 2017-05-25 19:22:38Z cross $
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

static void
soak(void)
{
	int ch;

	do {
		ch = getchar();
	} while (ch != EOF && ch != '\n');
}

int
main(void)
{
	printf("\nPlease enter a number--");
	fflush(stdout);
	soak();
	printf("\nPlease enter another number--");
	fflush(stdout);
	soak();
	printf("\nThank you.\n\n");

	return (EXIT_SUCCESS);
}
