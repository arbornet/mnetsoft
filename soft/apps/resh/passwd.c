/*
 * run ylc with the given arguments.
 */

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	argv[0] = "/suid/bin/passwd";
	return(spawn(argc, argv));
}
