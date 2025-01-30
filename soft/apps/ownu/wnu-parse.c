/* PARSE -- low-level routines to support parsing - ubiquitous Jan Wolter code.
 *
 * Firstin() returns a pointer to the first character in s that is in l.
 * \0 is always considered to be in the string l.
 *
 * Firstout() returns a pointer to the first character s that is not in l.
 * \0 is always considered to be not in the string l.
 *
 * Note that unlike strpbrk() these never return NULL.  They always return
 * a valid pointer into string s, if only a pointer to it's terminating
 * \0.  They are amazingly useful for simple tokenizing.
 *
 * VERSION HISTORY:
 * 04/15/93 - Original version.  [jdw]
 * 12/02/95 - Ansification. [jdw]
 */

#include <stdio.h>
#include "wnu.h"

char *firstin(char *s, char *l)
{
	for (;*s;s++)
		if (index(l,*s)) break;
	return(s);
}

char *firstout(char *s, char *l)
{
	for (;*s;s++)
		if (!index(l,*s)) break;
	return(s);
}
