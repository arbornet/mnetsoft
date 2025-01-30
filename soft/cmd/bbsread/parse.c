/*
 * PARSE -- low-level routines to support parsing.
 *
 * Firstin() returns a pointer to the first character in s that is in l. \0 is
 * always considered to be in the string l.
 *
 * Firstout() returns a pointer to the first character s that is not in l. \0 is
 * always considered to be not in the string l.
 *
 * Note that unlike strpbrk() these never return NULL.  They always return a
 * valid pointer into string s, if only a pointer to it's terminating \0.
 * They are amazingly useful for simple tokenizing.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <string.h>
#include "bbsread.h"

char *
firstin(char *s, char *l)
{
	return s + strcspn(s, l);
}

char *
firstout(char *s, char *l)
{
	return s + strspn(s, l);
}
