/*
 * Print working directory.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	char cwd[MAXPATHLEN + 1];

	printf("%s\n", getcwd(cwd, sizeof(cwd)));

	return(EXIT_SUCCESS);
}
