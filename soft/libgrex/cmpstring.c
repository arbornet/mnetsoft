/**
 * @name cmpstring
 *
 * Comparison function suitable for use with library routines
 * such as qsort(), bsearch(), etc.  This just calls strcmp()
 * with the arguments cast to character pointers, but handles
 * NULL pointers correctly.  See the comp.lang.c FAQ listing
 * for why we do this instead of just passing strcmp to qsort
 * et al.
 *
 * @param a void pointer to first string.
 * @param b void pointer to second string.
 * @return Equivalent to strcmp(a, b)
 * @see qsort(3), bsearch(3), strcmp(3)
 * @memo String comparision function for qsort(3), bsearch(3), tsearch(3), etc.
 */

#include <stddef.h>
#include <string.h>

#include "libgrex.h"

int
cmpstring(const void *a, const void *b)
{
	if (a == NULL && b == NULL)
		return 0;
	if (a == NULL)
		return -1;
	if (b == NULL)
		return 1;
	return(strcmp((char *)a, (char *)b));
}
