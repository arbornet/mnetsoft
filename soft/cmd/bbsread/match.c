/*
 * MATCH -- Check if a name matchs a pattern string.  Letters after the _ in
 * the pattern are optional to a successful match.  The pattern may be
 * terminated either by a null or a colon.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <stdio.h>

int
match(char *name, char *pat)
{
	int passed_dash = 0;

	for (; (*pat != ':' || *pat == '\0') && *name != '\0'; name++, pat++) {
		if (*pat == '_') {
			passed_dash = 1;
			name--;
			continue;
		}
		if (*name != *pat)
			return (0);
	}

	if (*pat == ':' || *pat == '\0')
		return (*name == '\0');
	else
		return (passed_dash || *pat == '_');
}
