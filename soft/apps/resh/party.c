/*
 * Bare-bones ps(1).
 */

#include <stddef.h>
#include "resh.h"

int
main(int argc, char *argv[])
{
	char *args[] = { "/suid/bin/party", NULL };
	return(spawn(NELEM(args), args));
}
