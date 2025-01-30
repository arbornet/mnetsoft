#include <ctype.h>

/*
 * ATOLH -- For systems without a builtin atoh() function, this interprets a
 * character string as a hex string and returns an integer equivalent.
 *
 * Mar 16, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

char *index();

unsigned long
atolh(char *str)
{
	unsigned long n = 0;

	for (; isascii(*str) && isxdigit(*str); str++)
		n = n * 16 + *str - (isdigit(*str) ? '0'
				     : (isupper(*str) ? 'A'
					: 'a') - 10);
	return (n);
}
