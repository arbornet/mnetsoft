/*
 * This program toggles your backspace character.  If it is not control-H,
 * it sets it to control-H.  If it is control-H, it sets it to DEL.
 *
 * Jan Wolter - 3/9/97
 *
 * $Id: backspace.c 1485 2014-05-21 14:18:50Z cross $
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <termios.h>

#include "libgrex.h"

#ifndef TCSASOFT
#define	TCSASOFT 0
#endif

enum {
	BS = 0010,		/* Preferred backspace character */
	DEL = 0177		/* Alternative backspace character */
};

int erase[] = {BS, DEL};

int
main(int argc, char *argv[])
{
	struct termios tmode;

	progname(argv[0]);
	if (tcgetattr(0, &tmode) < 0)
		fatal("Input not a tty: %r");
	tmode.c_cc[VERASE] = erase[tmode.c_cc[VERASE] == BS];
	tcsetattr(0, TCSADRAIN | TCSASOFT, &tmode);

	return (EXIT_SUCCESS);
}
