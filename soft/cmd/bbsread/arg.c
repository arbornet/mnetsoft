/*
 * COMMAND LINE ARGS -- This interprets "read" command-like command line args
 * and sets the appropriate global flag variables.
 *
 * Mar 16, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification Dec  3, 1995 - Jan Wolter:  Response ranges, better "usage".
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bbsread.h"

bool read_brandnew = true;	/* Completely new items */
int read_newresp = 1;		/* New response items (1 = newresp, 2= whole
				 * item) */
bool read_old = false;		/* Old items */
int read_forgotten = 0;		/* Forgotten items (1 = newresp, 2= whole
				 * item) */

bool reverse = false;		/* Read backwards */
bool prompt_rfp = true;		/* Stop to prompt for command after each item */
bool noresponse = false;	/* Print only headers on pass */

bool extracting = false;	/* Has user named specific responses? */


void set_new(), set_reverse(), set_all(), set_brandnew(), set_nwresponse(),
 set_old(), set_forgotten(), set_noforget(), set_noresponse(), set_pass(),
 set_nopass();

struct arg_list {
	char *arg;		/* Name of an argument */
	void (*func) ();	/* Routine to call to execute it */
} args[] = {

	{
		"n_ew", set_new
	},
	{
		"r_everse", set_reverse
	},
	{
		"a_ll", set_all
	},
	{
		"bra_ndnew", set_brandnew
	},
	{
		"newr_response", set_nwresponse
	},
	{
		"o_ld", set_old
	},
	{
		"for_gotten", set_forgotten
	},
	{
		"nof_orget", set_noforget
	},
	{
		"nor_esponse", set_noresponse
	},
	{
		"nop_ass", set_nopass
	},
	{
		"p_ass", set_pass
	},

	{
		NULL, NULL
	}
};


/* USAGE - Print out our usage message and exit the program. */

void
usage()
{
	int col, i;
	char *c;

	fprintf(stderr, "usage: %s [-pjm] <confname> "
	"[<options>] [<item>-<item>] [<item> [<resp>[-<resp>]]\n", progname);
	fprintf(stderr, "options are:");
	col = 12;
	for (i = 0; args[i].arg != NULL; i++) {
		putc((col + strlen(args[i].arg) >= 80) ? '\n' : ' ', stderr);
		for (c = args[i].arg; *c != '\0'; c++)
			if (*c != '_')
				putc(*c, stderr);
	}
	putc('\n', stderr);
	exit(1);
}


/* PARSE_ARG -- Parse a read option */

void
parse_arg(char *arg)
{
	static int which_range = 0;	/* Which range to set (0= item, 1=
					 * resp) */
	struct arg_list *a;

	/* gobblty gook */
	if (!isascii(arg[0])) {
		usage();
		return;
	}
	/* numeric arguments -- read item range or response range */
	if (isdigit(arg[0])) {
		switch (which_range) {
		case 0:
			parse_range(&item, arg);
			read_brandnew = true;
			read_newresp = 2;
			read_old = true;
			which_range++;
			break;
		case 1:
			if (item.first != item.last)
				usage();
			extracting = true;
			parse_range(&resp, arg);
			which_range++;
			break;
		default:
			usage();
		}
	}
	for (a = args; a->arg != NULL; a++) {
		if (match(arg, a->arg)) {
			(*a->func) ();
			return;
		}
	}
	usage();
	return;
}


void
set_new()
{
	read_brandnew = true;
	read_newresp = 1;
	read_old = false;
}


void
set_all()
{
	read_brandnew = true;
	read_newresp = 2;
	read_old = true;
}

void
set_old()
{
	read_brandnew = false;
	read_newresp = 0;
	read_old = true;
}

void
set_brandnew()
{
	read_brandnew = true;
	read_newresp = 0;
	read_old = false;
}

void
set_nwresponse()
{
	read_brandnew = false;
	read_newresp = 1;
	read_old = false;
}

void
set_forgotten()
{
	read_brandnew = false;
	read_newresp = 0;
	read_old = false;
	read_forgotten = 1;
}

void
set_reverse()
{
	reverse = true;
}

void
set_noforget()
{
	read_forgotten = 1;
}

void
set_nopass()
{
	prompt_rfp = true;
}

void
set_pass()
{
	prompt_rfp = false;
}

void
set_noresponse()
{
	noresponse = true;
}
