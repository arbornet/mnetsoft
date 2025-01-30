/*
 */

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char *
lower(char *str)
{
	char	*p;

	for (p = str; *p != '\0'; p++) {
		if (isupper(*p)) {
			*p = tolower(*p);
		}
	}

	return(str);
}

char *
unquote(char *str)
{
	char	*s, *d;

	for (s = d = str; *s != '\0'; *d++ = *s++) {
		if (*s == '\'' || *s == '\\') {
			if (*(s + 1) == '\0') {
				*s = '\0';
			}
			s += (*(s + 1) != '\0');
		}
	}
	*d = '\0';

	return(str);
}

static char *
quotedtoken(char *str)
{
	char	*p;

	for (p = str; *p != '\0'; p++) {
		if (*p == '\'') {
			if (*(p + 1) == '\'')
				p++;
			else {
				*p++ = '\0';
				break;
			}
		} else if (*p == '\\' && *(p + 1) != '\0')
			p += 2;
	}

	return(p);
}

static char *
simpletoken(char *str)
{
	char	*p;

	for (p = str; *p != '\0' && *p != '\'' && !isspace(*p); p++)
		;

	return(p);
}

int
tokenizeq(char *str, char *toks[], int maxtoks)
{
	char	*p;
	int	ntoks;

	if (str == NULL || toks == NULL || maxtoks <= 0)
		return(-1);
	p = str;
	ntoks = 0;
	while (*p != '\0') {
		while (isspace(*p))
			*p++ = '\0';
		switch (*p) {
		case '\0':
			break;
		case '\'':
			*p++ = '\0';
			toks[ntoks] = p;
			p = quotedtoken(p + 1);
			unquote(toks[ntoks]);
			ntoks++;
			break;
		default:
			toks[ntoks++] = p;
			p = simpletoken(p);
			break;
		}
		if (ntoks >= maxtoks)
			break;
	}

	return(ntoks);
}
