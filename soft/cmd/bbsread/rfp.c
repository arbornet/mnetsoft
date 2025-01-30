/*
 * RFP PROMPT COMMANDS -- This interprets and executes such RFP prompt
 * commands as this program manages to cope with.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Dec  2, 1995 - Jan Wolter:
 * Ansification
 */

#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "bbsread.h"

int curitem;			/* Name of current item */
int curresp;			/* Current response number */
int max_resp;			/* Max response number in current item */
int max_read;			/* Max resp number read previously */

jmp_buf pipejmp;

int againfirst, againlast, againhead;	/* Last display_it done */


int cant_respond(), cant_act(), cant_find(), cant_since();
int do_forget(), do_pass(), do_remember(), do_header(), do_text(), do_again(),
 do_first(), do_current(), do_last(), do_new(), do_postpone(), do_only(),
 do_stop();


struct cmd_list {
	char *cmd;		/* Name of a command */
	int (*func) ();		/* Routine to call to execute it */
} rfp_cmds[] = {

	{
		"r_espond", cant_respond
	},
	{
		"ps_eudonym", cant_respond
	},
	{
		"forget", do_forget
	},
	{
		"p_ass", do_pass
	},
	{
		"k_ill", cant_act
	},
	{
		"rem_ember", do_remember
	},
	{
		"unf_orget", do_remember
	},
	{
		"h_eader", do_header
	},
	{
		"text", do_text
	},
	{
		"a_gain", do_again
	},
	{
		"first", do_first
	},
	{
		"cu_rrent", do_current
	},
	{
		"l_ast", do_last
	},
	{
		"n_ew", do_new
	},
	{
		"po_stpone", do_postpone
	},
	{
		"freeze", cant_act
	},
	{
		"thaw", cant_act
	},
	{
		"sync_hronous", cant_act
	},
	{
		"async_hronous", cant_act
	},
	{
		"retire", cant_act
	},
	{
		"unretire", cant_act
	},
	{
		"e_nter", cant_respond
	},
	{
		"f_ind", cant_find
	},
	{
		"lo_cate", cant_find
	},
	{
		"si_nce", cant_since
	},
	{
		"cen_sor", cant_act
	},
	{
		"scr_ibble", cant_act
	},
	{
		"onl_y", do_only
	},
	{
		"stop", do_stop
	},
	{
		"q_uit", do_stop
	},
	{
		"^", do_first
	},
	{
		"$_$", do_last
	},

	{
		NULL, NULL
	}
};

int do_badcmd();
void display_it();


/*
 * RFP_COMMAND -- execute the rfp command string given by cmd.  This returns
 * 1 if we should execute another command afterwards, 0 otherwise.
 */

int
rfp_command(char *cmd)
{
	struct cmd_list *c;
	char *cmd_end, *args;
	int resp;

	cmd = firstout(cmd, " \t");	/* Skip leading spaces and tabs */
	cmd_end = firstin(cmd, " \t\n");	/* End of first keyword */
	args = firstout(cmd_end, " \t\n");	/* Skip spaces before first
						 * arg */
	*cmd_end = '\0';

	/* Blank line -- Just pass */
	if (*cmd == '\0')
		return (do_pass());

	/* gobblty gook */
	if (!isascii(cmd[0])) {
		do_badcmd(cmd);
		return (1);
	}
	/* numeric commands -- read from absolute response number */
	if (isdigit(cmd[0])) {
		display_it(atoi(cmd), -1, 0);
		return (1);
	}
	/* signed numeric commands -- read from relative response number */
	if (cmd[0] == '-' || cmd[0] == '+') {
		if (cmd[1] == '\0')
			resp = 1;
		else if (!isascii(cmd[1]) || !isdigit(cmd[1])) {
			do_badcmd(cmd);
			return (1);
		} else
			resp = atoi(cmd + 1);
		if (cmd[0] == '-')
			resp = -resp;
		display_it(curresp + resp, -1, 0);
		return (1);
	}
	/* shell escapes */
	if (cmd[0] == '!') {
		if (args != cmd_end) {
			*cmd_end = ' ';
			*firstin(cmd_end, "\n") = '\0';
		}
		usystem(cmd + 1);
		return (1);
	}
	/* Slide through command table and execute the first that matches */
	for (c = rfp_cmds; c->cmd != NULL; c++)
		if (match(cmd, c->cmd))
			return ((*c->func) (args));

	/* No command matches */
	do_badcmd(cmd);
	return (1);
}


/*
 * MAYBE_DISP -- Decide if we want to display the current item.  If so,
 * select the appropriate response range, display it and return 1.  Otherwise
 * return 0.
 */

int
maybe_display()
{
	bool forgotten;
	int read_from;

	/* Check participation file */
	last_read(curitem, &max_read, &forgotten);

	if (forgotten) {
		/* Forgotten items */
		if (!read_forgotten)
			return (0);
		if (read_forgotten == 2)
			read_from = 0;
		else
			read_from = max_read + 1;
	} else if (max_read >= max_resp) {
		/* Old Item */
		if (max_read > max_resp) {
			/* Missing response */
			fprintf(stderr,
			      "Whoops...item %d is missing %d responses.\n",
				curitem, max_read - max_resp);
			update_item(curitem, max_resp);
			max_read = max_resp;
		} else if (!read_old)
			return (0);
		read_from = 0;
	} else if (max_read == -1) {
		/* Brand new item */
		if (!read_brandnew)
			return (0);
		read_from = 0;
	} else {
		/* Newresponse item */
		if (!read_newresp)
			return (0);
		if (read_newresp == 2)
			read_from = 0;
		else
			read_from = max_read + 1;
	}

	display_it(read_from, -1, noresponse ? 2 : 1);
	return (1);
}


/*
 * DISPLAY_IT -- Go ahead and display an item (or a part of an item) and mark
 * that section of the file as read in the participation file.
 */

void
display_it(int first, int last, int showhead)
{
	FILE *ofp;

	againfirst = first;
	againlast = last;
	againhead = showhead;

	if (dont_page) {
		ofp = stdout;
		signal(SIGINT, pipeintr);
	} else
		ofp = upopen(PAGER, "w");

	/* Set up to recover from interupts during execution */
	if (setjmp(pipejmp)) {
		/* This is executed if the pager or the pipe terminates */
		fputc('\n', stderr);
		if (dont_page) {
			signal(SIGINT, SIG_IGN);
			fflush(stdout);
		} else
			upclose();
		return;
	}
	fputc('\n', ofp);
	last = dispitem(ofp, confdir, curitem, first, last, max_resp, showhead, false);

	if (dont_page)
		signal(SIGINT, SIG_IGN);
	else
		upclose();

	if (last > max_read && (!extracting || first <= max_read)) {
		update_item(curitem, last);
		max_read = last;
	}
	curresp = last + 1;
}

void
pipeintr(int sig)
{
	longjmp(pipejmp, 1);
}


/*
 * DO_EXTRACT -- item and responses have been designated by user.  Just show
 * that, not worrying about if it is new or not.
 */

int
do_extract()
{
	bool forgotten;

	if (next_item(&curitem, &max_resp) || curitem != item.first)
		return (0);
	curresp = max_resp;
	max_resp--;
	last_read(curitem, &max_read, &forgotten);
	display_it(resp.first, resp.last, resp.first == 0);
	return (1);
}


/* ======================== COMMAND ROUTINES ======================== */


int
do_pass()
{
	do {
		if (reverse) {
			if (prev_item(&curitem, &max_resp) ||
			    curitem < item.first)
				return (0);
		} else {
			if (next_item(&curitem, &max_resp) ||
			    (curitem > item.last && item.last != -1))
				return (0);
		}

		curresp = max_resp;
		max_resp = max_resp - 1;

	} while (!maybe_display());

	return (1);
}


int
do_badcmd(char *cmd)
{
	fprintf(stderr, "Unknown command: %s\n", cmd);
	return (1);
}


int
do_first()
{
	display_it(1, -1, 0);
	return (1);
}


int
do_last()
{
	display_it(max_resp, -1, 0);
	return (1);
}


int
do_text()
{
	display_it(0, -1, 0);
	return (1);
}


int
do_current()
{
	display_it(curresp, -1, 0);
	return (1);
}


int
do_again()
{
	display_it(againfirst, againlast, againhead);
	return (1);
}


int
do_only(char *arg)
{
	int n;
	if (!isascii(arg[0]) || !isdigit(arg[0])) {
		fprintf(stderr,
			"The 'only' command requires a numeric argument.\n");
		return (1);
	}
	n = atoi(arg);
	display_it(n, n, 0);
	return (1);
}


int
do_forget()
{
	update_item(curitem, -1);
	fprintf(stderr, "item %d forgotten\n", curitem);
	return (1);
}


int
do_remember()
{
	update_item(curitem, -1);
	fprintf(stderr, "item %d remembered\n", curitem);
	return (1);
}


int
do_header()
{
	display_it(0, 0, 2);
	return (1);
}


int
do_postpone()
{
	reset_item(curitem);
	return (do_pass());
}


int
do_new()
{
	reset_item(curitem);
	return (0);
}


int
do_stop()
{
	return (0);
}


int
cant_act()
{
	fprintf(stderr, "Cannot change items -- use \042bbs\042 for that.\n");
	return (1);
}


int
cant_respond()
{
	fprintf(stderr,
		"Cannot respond to items -- use \042bbs\042 for that.\n");
	return (1);
}


int
cant_find()
{
	fprintf(stderr, "Text searching not supported.\n");
	return (1);
}


int
cant_since()
{
	fprintf(stderr, "Date searching not supported.\n");
	return (1);
}
