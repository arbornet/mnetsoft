/*
 * $Id: progname.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <stddef.h>
#include <string.h>

#include "libgrex.h"

char	*libgrex_program_name__;	/* Used by warning(), fatal(), etc. */

/**
 * @name progname
 *
 * For example, if called as:
 * progname("/path/to/a/file");
 * return a pointer to the substring
 * "file" in the input argument.
 *
 * If the input is NULL, return NULL.
 * If the input is a basename, return
 * only that.
 *
 * @memo Return the base file name from a pathname.
 */
char *
basename(char *filename)
{
	char	*slash;

	if (filename == NULL)
		return(NULL);
	slash = strrchr(filename, '/');
	if (slash == NULL)
		return(filename);
	return(slash + 1);
}

char *
progname(char *filename)
{
	libgrex_program_name__ = basename(filename);
	return(libgrex_program_name__);
}
