/*
 * Bare-bones ps(1).
 */

#include <stddef.h>
#include "resh.h"

int
main(int argc, char *argv[])
{
	char *args[] = { "/usr/local/bin/fronttalk", NULL };
	return(spawn(NELEM(args), args));
}
