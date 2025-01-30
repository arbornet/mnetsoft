/*
 * help.c
 */

#include <stddef.h>
#include "resh.h"

int
main(int argc, char *argv[])
{
	char *args[] = { "pg", "/cyberspace/grex/resh/lib/resh-help", NULL };
	return(spawn(NELEM(args), args));
}
