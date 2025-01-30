#include "gate.h"
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

/* Termcap Library interface */

/* Gate's use of termcap stuff is strictly limited.  We don't really want to
 * go into any visual mode, and we certainly don't want to be clearing the
 * screen when we start up.  This means we avoid sending the "ti" string before
 * entering, or the "te" string afterwards.  This is a problem because some
 * terminals won't accept cursor movement commands until after you have done
 * some other stuff.  Currently, that's just tough luck.  We try to design
 * things so it will still work reasonably.
 */

short ospeed;		/* The output baud rate */
char PC= '\000';	/* The pad characters (default ^@) */
char area[1024];	/* Memory to store the following: */
char *BC;		/* The backspace string (default `\b`) */
char *BL;		/* The bell character */
char *CD;		/* The clear to bottom of screen string */
char *CE;		/* The clear to end of line string */
char *CR;		/* The carriage return string */
char *SE;		/* The end stand out mode string */
char *SO;		/* The start stand out mode string */
char *UE;		/* The end underline mode string */
char *US;		/* The start underline mode string */
char *UP;		/* The cursor up string */
int mycols= 0;		/* Screen size */
int mylines= 0;		/* Screen size */
char *tgetstr(), *tgoto(), *getenv();


/* INITTERM()
 *
 *   Read in terminal capabilities description (termcap).  Terminal type is
 *   obtained from $TERM environment variable.
 */

void initterm()
{
    register char *bp, *pad, *term;
    char *tp;
#ifdef GREX
    int t= 't';	/* workaround for ridiculous bug in Grex gcc */
#endif /*GREX*/
#ifdef TIOCGWINSZ
    struct winsize x;

    ioctl(2,TIOCGWINSZ,&x);
    mycols= x.ws_col;
    mylines= x.ws_row;
#endif /*TIOCGWINSZ*/

    /* Get the termcap entry */
    bp = (char *) malloc(1024);
    if ((term= getenv("TERM")) == NULL || tgetent(bp,term) != 1)
    {
	/* No termcap: */
	/* assume you can backspace with ^H, return with ^M */
	free(bp);
	area[0]= '\b'; area[1]= '\0'; area[2]= '\r'; area[3]= '\0';
	area[4]= '\007'; area[5]= '\0';
	BC= area;		/* Backspace with ^H */
	CR= area+2;		/* Carraige return with ^M */
	BL= area+4;		/* Ring with ^G */
	CE= CD= SE= SO= UE= US= UP= NULL;
	return;
    }

    /* Load the pad character for tputs.  Default to null. */
    tp= area;
    if(pad= tgetstr("pc",&tp))
	PC= *pad;

    /* Get number of lines and columns if we don't already have them */
    if (mycols == 0)
    {
	mycols= tgetnum("co");
	if (mycols < 0) mycols= 80;
    }
    if (mylines == 0)
    {
	mylines= tgetnum("li");
	if (mylines < 0) mylines= 24;
    }

    /* Get the backspace string - if "bc" undefined used control-H even
     * if "bs" isn't defined...usually if "bs" is not there it's an error */
    if ((BC= tgetstr("bc",&tp)) == NULL && (BC= tgetstr("le",&tp)) == NULL)
	{ BC= tp; *tp++= '\b'; *tp++= '\0'; }

    /* Get the bell string, prefering visual bell if it is there */
    if (!(BL= tgetstr("vb",&tp)) && !(BL= tgetstr("bl",&tp)))
	{ BL= tp; *tp++= '\007'; *tp++= '\0'; }

    /* Get the carriage return string */
    if (tgetflag("nc"))
	CR= (char *) 0;
    else if (!(CR= tgetstr("cr",&tp)))
	{ CR= tp; *tp++= '\r'; *tp++= '\0'; }

    UP= tgetstr("up",&tp);		/* Move cursor up */
    CD= tgetstr("cd",&tp);		/* Clear rest of screen */
    CE= tgetstr("ce",&tp);		/* Clear rest of line */

    SO= tgetstr("so",&tp);		/* Start standout mode */
    SE= tgetstr("se",&tp);		/* End standout mode */
    if(!SO || !SE) SO= SE= 0;

    US= tgetstr("us",&tp);		/* Start underline mode */
    UE= tgetstr("ue",&tp);		/* End underline mode */
    if(!SO || !SE) {US= SO; UE= SE;}

    /* Check if termcap buffer overflowed.  Free memory */
    if (tp-area > sizeof(area))
    {
	printf("Panic: Termcap too big.  %d bytes.\n",tp-area);
	exit(1);
    }
    free(bp);
}

/* XPUTC()
 *
 *   putchar() is a macro.  We need a real subroutine to use with tputs.
 *   This is it.
 */

void xputc(char ch)
{
    putchar(ch);
}

/* XPUTS()
 *
 *   send out a control sequence.
 */
void xputs(char *s)
{
    (void) tputs(s, 1, xputc);
}

/* NXPUTS()
 *
 *   Put out the same control sequence <s> n times.
 */

void nxputs(char *s,int n)
{
    for ( ; n > 0; n--)
	(void) tputs(s, 1, xputc);
}

/* MOVE_UP - Move the cursor up to the begining of the n-th previous line,
 * erasing everything from the current line up to, but not including, the
 * destination line.  If cursor movement doesn't work, this just moves you
 * to the begining of the current line and erases it.  Our current column
 * number on the current-line should be supplied.  You should call erase_eos()
 * on the final line when you have typed on it.
 */

void move_up(int n, int ccol)
{
    int i;

    cr();

    /* Dumb Terminals */
    if (!UP || (!CE && !CD))
    {
	for (i= 0; i < ccol; i++)
	    putchar(' ');
	cr();
    }

    for (i= 0; i < n; i++)
    {
	if (!CD) xputs(CE);
	xputs(UP);
    }
    if ((junk_above-= n) < 0) junk_above= 0;
}

void erase_eol()
{
    if (CE)
	xputs(CE);
    else if (CD)
	xputs(CD);
}

void erase_eos()
{
    if (CD)
	xputs(CD);
    else if (CE)
	xputs(CE);
}


/* BEGIN_STANDOUT(), END_STANDOUT()
 *
 *   Enter and exit standout mode.
 */

int in_so= 0;

void begin_standout()
{
    if(SO && !in_so) xputs(SO);
    in_so= 1;
}

void end_standout()
{
    if(SE && in_so) xputs(SE);
    in_so= 0;
}


/* BEGIN_UNDERLINE(), END_UNDERLINE()
 *
 *   Enter and exit standout mode.
 */

void begin_underline()
{
    if(US && !in_so) xputs(US);
    in_so= 1;
}

void end_underline()
{
    if(UE && in_so) xputs(UE);
    in_so= 0;
}


/* GET_PGRP -- Return our process group */
int get_pgrp()
{
    int pgrp;

    ioctl(0,TIOCGPGRP,&pgrp);
    return pgrp;
}
