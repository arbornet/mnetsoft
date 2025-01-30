#include <stdio.h>

/* PRINT-TERMS
 *
 * This is Marcus Watt's pterm() routine out of his newuser program, hacked
 * up to be a stand-alone cgi tool.
 *
 */

int tables= 1;	/* Set this true if the browser understands tables */

#define COLS 5
int col;


#ifndef TERMCAP
#define TERMCAP "/etc/termcap"
#endif


/* PRINT_A_TERM */

print_a_term(char *term)
{
	cgiprintf(tables ? "<TD>%S</TD>" : "%-12.12S ",term);
	if (++col == COLS)
	{
		printf(tables ? "</TR>\n<TR>" : "\n");
		col= 0;
	}
}


/* GETLONGEST
 * find the longest terminal name not longer than len
 * that is pure alphanumeric
 * (helper function for pterms)
 */

char *getlongest(char *cp, int len)
{
	char *bst, *ben;
	char *st, *en;
	int f;
	bst = 0;
	for (st = cp; ; st = en+1)
	{
		f = 1;
		for (en = st; *en; ++en)
		{
			if (*en == '|' || *en == ':')
				break;
			if (!isalnum(*en) && *en != '-') f = 0;
		}
		if (f && (en - st) <= len)
		{
			if (!bst || (en-st) >= (ben-bst))
			{
				bst = st;
				ben = en;
			}
		}
		if (*en != '|')
			break;
		st = en+1;
	}
	if (bst)
		*ben = 0;
	return bst;
}


/* PTERMS
 * 	print out terminals from termcap.
 *	uses a pretty N-across format, and quits early
 *	if it gets an interrupt.  Prints the longest
 *	reasonable alphanumeric ID found, for each terminal.
 *	(hopefully it'll mean something to the user.)
 */

pterms()
{
	FILE *fd;
	char inputline[300];
	char *name;

	fd = fopen(TERMCAP, "r");
	if (!fd)
	{
		fprintf (stderr, "Could't open termcap file\n");
		return;
	}


	for (;;)
	{
		fgets(inputline, 300, fd);
		if (feof(fd))
			break;
		if (*inputline == '#')
			continue;
		if (*inputline == '\t' || *inputline == ' ')
			continue;
		name = getlongest(inputline, 10);
		if (!name)
			continue;
		print_a_term(name);
	}
	fclose(fd);
}


main(int argc, char **argv)
{
	print_cgi_header("Valid " SYSNAME " Terminal Types");
	printf(tables ? "<TABLE>\n<TR>" : "<PRE>\n");

	col= 0;
	pterms();

	if (tables)
		printf(tables ? "</TR>\n</TABLE>\n" : "</PRE>\n");
	print_cgi_trailer();
}
