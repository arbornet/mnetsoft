/*
 * RANGE MANIPULATION -- Handle ranges of items or responses.  Not much here
 * yet, but there should be more if we ever really get fancy.
 *
 * Dec  4, 1995 - Jan Wolter:  Broken out from arg.c
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "bbsread.h"

struct range item = {1, -1};	/* item range to read */
struct range resp = {0, -1};	/* response range to read */


void usage();

/*
 * PARSE_RANGE - Parse an item or response range.  Right now we only savy
 * single numbers or pairs of dash-separated numbers.
 */

void
parse_range(struct range * r, char *arg)
{

	r->first = atoi(arg);

	arg = firstout(arg, "0123456789");

	if (arg[0] == '\0') {
		r->last = r->first;
		if (r == &item)
			read_forgotten = 2;
	} else if (arg[0] != '-')
		usage();
	else if (arg[1] == '$' || arg[1] == '\0')
		r->last = -1;
	else if (isascii(arg[1]) && isdigit(arg[1]))
		r->last = atoi(arg + 1);
	else
		usage();

	if (r->last != -1 && r->first > r->last) {
		int temp = r->first;
		r->first = r->last;
		r->last = temp;
	}
	return;
}
