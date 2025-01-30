/*
 * $Id: background.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <sys/types.h>

#include <err.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "libgrex.h"

/**
 * @name background
 *
 * Background the current process by calling fork(2) and
 * exiting in the parent.  Note, this will also exit if
 * fork() fails if, e.g., the process table is full, the
 * system is out of memory, etc.
 *
 * @see fork(2)
 * @memo Put the current process into the `background'.
 */
void
background(void)
{
	pid_t pid;

	pid = fork();
	if (pid > 0)
		_exit(EXIT_SUCCESS);
	if (pid < 0) {
		fatal("couldn't fork a process: %r");
	}
}
