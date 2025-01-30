#include "gate.h"

/* Various Globals local to this file, because I'm sick of passing these to
 * every darn helper routine in here.
 */

int colo;	/* current output column [1-79] - cursor is never in 0 */
int bfi;	/* current buffer index [0-78] (is colo-1 without tabs) */
char *bf;	/* primary text buffer */
char *pt;	/* prompt string */
int pcol;	/* column after prompt */


/* The following routines are for fooling with tabs.  Column numbers are from
 * 0 to 79.
 *
 * TABCOL(c) - If the cursor was in column c, this returns the column it would
 * be in after hitting a tab.
 *
 * OUTCOL(str,i) - What column would the cursor appear in after typing the
 * string from str[0] through str[n-1] with icol spaces prepended?
 *
 * MCURSOR(*col,*lin,ch) - given that we type character ch, update line and
 * column number.
 */

/* #define tabcol(c) ((((c)-base_col+8)&(-8))+base_col) --- moved to gate.h */

int outcol(char *str, int n, int icol)
{
    int i,c= icol;

    for (i= 0; i < n && str[i] != '\0'; i++)
    {
	if (str[i] == '\t')
	    c= tabcol(c);
	else if (str[i] == '\b')
	    c--;
	else
	    c++;
    }
    return c;
}

void mcursor(int *col, int *lin, char ch)
{
    if (ch == '\t')
    {
	*col= tabcol(*col);
	if (*col > mycols)
	{
	    *col= 0;
	    (*lin)++;
	}
    }
    else if (ch == '\r')
	*col= 0;
    else if (ch == '\b')
    {
	if (*col == 0)
	{
	    *col= mycols-1;
	    (*lin)--;
	}
	else
	    (*col)--;
    }
    else if (ch == '\n' || ++(*col) > mycols)
    {
	*col= 0;
	(*lin)++;
    }
}


/* GETLINE -- read in a line from the user.  <buf> and <wbuf> are buffers big
 * enough to hold a <n> character string.  If <wcol> is non-zero, it gives the
 * number of input characters already stored in <buf>.  Upon return, the input
 * line is in <buf>, and <wbuf> contains <wcol> characters typed on this line,
 * but wrapped onto the next.
 *
 * If <wbuf> is NULL, then this does a more ordinary, wrapless read.
 */

int ggetline(char *buf, char *wbuf, int *wcol, char *prompt)
{
    int linemode= (wbuf == NULL);	/* reading single line, not file */
    int wrapon= !linemode;	        /* is line wrap enabled? */
    int ch,backwrapped,rc;
#define RESUME 256
    int zero= 0;
    static int space_wrapped= 0;

    bf= buf;
    pt= prompt;
    pcol= outcol(prompt,BUFSIZE,0);
    if (wcol == NULL) wcol= &zero;

    /* Echo the wrapped characters */
    print_to_index(bfi= *wcol);
    colo= outcol(bf,bfi,pcol);

    /* Input loop */
    use_jump= 1;
    for(;;)
    {
	if (resume || (rc= setjmp(jenv)) != 0)
	{
	    if (linemode && rc == 1)
	    {
		use_jump= 0;
		return 1;
	    }
	    resume= 0;
	    ch= RESUME;
	}
	else
	    ch= getchar();

	if (ch == '\n')
	{
	    putchar('\n');
	    bf[bfi++]= '\n';
	    bf[bfi]= '\0';
	    *wcol= 0;
	    space_wrapped= 0;
	    break;
	}
	else if (ch == EOF_CHAR && !linemode)
	{
	    if (bfi == 0)
	    {
		use_jump= 0;
		return 1;
	    }
	    putchar('\007');
	}
	else if (ch == KILL_CHAR)
	{
	    bfi= 0;
	    back_to_col(pcol);
	    wrapon= !linemode;
	}
	else if (ch == REPRINT_CHAR || ch == RESUME || ch == '\014')
	{
	    if (ch != RESUME) putchar('\n');
	    if (ch == '\014' && !linemode)
	    {
		fseek(tfp,0L,0);
		printf("---\n");
		typetoend(pcol);
	    }
	    print_to_index(bfi);
	    if (backwrapped)
		{ erase_eos(); backwrapped= 0;}
	}
	else if (bfi == 0 && (ch == WERASE_CHAR || ch == ERASE_CHAR ||
		              ch == '\b' || ch == '\177'))
	{
	    /* Wrap back to previous line */
	    int rc;

	    if (linemode || (!backwrap || (rc= cliplast(bf,BUFSIZE)) == 1))
	    {
		/* In first line of file -- can't backwrap */
		space_wrapped= 0;
		continue;
	    }
	    
	    if (rc == 0)
	    {
		 bfi= strlen(bf);
		 colo= outcol(bf,bfi,pcol);
		 if (colo > maxcol)
		 {
		    fputs(bf,tfp);
		    putc('\n',tfp);
		    rc= 2;
		 }
	    }
	    if (rc == 2)
	    {
		printf("\n[sorry, previous line too long to backwrap to]\n");
		bfi= 0;
		colo= pcol;
		lines_above= 0;
	    }
	    else if (lines_above == 0)
		cr();
	    else
		{ move_up(1,pcol); backwrapped= 1; lines_above--;}
	    resume= 1;
	}
	else if (ch == WERASE_CHAR)
	{
	    while (bfi > 0 && (bf[bfi-1] == ' ' || bf[bfi-1] == '\t'))
		bfi--;

	    if (bfi > 0)
	    {
		bfi--;
		while (bfi > 0 && bf[bfi-1] != ' ' && bf[bfi-1] != '\t')
		    bfi--;
	    }

	    back_to_col(outcol(bf,bfi,pcol));
	    if (colo == 1) wrapon= !linemode;
	}
	else if (ch == ERASE_CHAR || ch == '\b' || ch == '\177')
	{
	    if (bf[--bfi] == '\t')
		back_to_col(outcol(bf,bfi,pcol));
	    else
	    {
		bs(); putchar(' '); bs();
		colo--;
	    }
	    if (colo == 1) wrapon= !linemode;
	}
	else if (ch == '\t')
	{
	    int c;
	    /* Handle wrapping on a tab */
	    if ((wrapon && (colo >= (hotcol&(-8)) || colo >= (maxcol&(-8)))) ||
		bfi >= BUFSIZE)
	    {
		bf[bfi++]='\n';
		bf[bfi]= '\0';
		*wcol= 0;
		wbuf[0]= '\0';
		putchar('\n');
		space_wrapped= 0;
		break;
	    }
	    /* Just store and display the tab */
	    bf[bfi++]= '\t';
	    colo= tabcol(c= colo);
	    if (dont_tab)
		for ( ; c < colo; c++) putchar(' ');
	    else
		putchar('\t');
	}
	else if (!(isascii(ch) && isprint(ch)) || bfi > BUFSIZE)
	    putchar('\007');
	else if (wrapon && (colo >= maxcol || (ch == ' ' && colo >= hotcol)))
	{
	    int flg, i;

	    /* When we wrap on a space, it just vanishes */
	    if (ch == ' ')
	    {
		bf[bfi++]='\n';
		bf[bfi]= '\0';
		*wcol= 0;
		wbuf[0]= '\0';
		putchar('\n');
		space_wrapped= 1;
		break;
	    }

	    /* Scan back to find the last space or tab */
	    for(i= bfi; i >= 0 && bf[i-1] != ' ' && bf[i-1] != '\t'; i--)
		;
	    if (i == -1)
		flg= i= bfi;     /* no spaces -- break at last col */
	    else
		flg= 0;
	    *wcol= bfi-i;

	    /* Backspace back and copy into next line buffer - no tabs here */
	    while (bfi > i)
	    {
		bs(); putchar(' '); bs();
		bfi--;
		wbuf[bfi-i]= bf[bfi];
	    }
	    if (!flg)
	    {
		bs(); putchar(' '); bs();
		bfi--;
	    }
	    putchar('\n');
	    bf[bfi++]='\n';
	    bf[bfi]= '\0';
	    wbuf[(*wcol)++]= ch;
	    wbuf[*wcol]= '\0';
	    space_wrapped= 0;
	    break;
	}
	else if (!(space_wrapped && ch == ' '))
	{
	    if (colo == 1 && (ch == ':' || ch == '!')) wrapon= 0;
	    bf[bfi++]= ch;
	    putchar(ch);
	    colo++;
	}
	space_wrapped= 0;
    }
    use_jump= 0;
    return 0;
}


/* BACK_TO_COL - Erase the line from the current position back to the given
 * column number and index.
 */

void back_to_col(int ncol)
{
    int i;

    if (3*(colo-ncol) < bfi+4+pcol)
    {
	while(colo > ncol)
	{
	    bs(); putchar(' '); bs();
	    colo--;
	}
    }
    else
    {
	cr();
	print_to_index(bfi);
	erase_eol();
	colo= ncol;
    }
}


/* PRINT_TO_INDEX - print the line up to the named index */

void print_to_index(int newi)
{
    int i;
    int col= pcol;

    fputs(pt,stdout);
    for (i= 0; i < newi; i++)
    {
	if (dont_tab)
	{
		if (bf[i] == '\b')
			col--;
		else if (bf[i] == '\t')
		{
			int fcol= tabcol(col);
			for ( ; col < fcol; col++)
				putchar(' ');
			continue;
		}
	}
	putchar(bf[i]);
    }
}
