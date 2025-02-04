/* PARTY PROGRAM -- OUTPUT FUNCTIONS - from file to screen - Jan Wolter */

/* This simulates a series of output filters in a pipeline.  Each of the
 * "internal filters" reads from one buffer and writes to another.  The
 * idea is to make it possible to write internal filter procedures like
 * expand(), eatline() and wrap() almost as if they were separate external
 * filter programs.  This makes it a lot simpler to add internal filter
 * procedures and simplifies the switching on and off of different
 * combinations of them.
 */

#include "party.h"
#include "opt.h"
#include <errno.h>
#include <term.h>

extern int errno;

/* This is a list of the internal filters numbered from 1 in the order in
 * which they will be applied if they are all turned on.
 */

#define BUF_FILE	0	/* Party file reader -- must be first */
#define BUF_EATLINE	1	/* Line eater */
#define BUF_EXPAND	2	/* Control character expander */
#define BUF_WRAP	3	/* Line wrapper */
#define MAXFIL		4

/* This is an array of output buffers that sit between internal filters.
 * Output buffer zero will always be used.  The rest may or may not be.
 */

struct outbuf {
	char *bf;	/* Pointer to buffer */
	int n;		/* Number of characters written to buffer */
	int i;		/* Number of characters read from buffer */
	} ob[MAXFIL];
#define OUTBUFSZ	1280

/* OUTPUT:  reads from the end of the party file.  If it finds anything there,
 * it prints it out through the filter chain.  It returns 1 if we are
 * at end of file.
 */

int output()
{
    int n;
    /* Read enough to fill the rest of the buffer */
    if ((n= read(rst, ob[BUF_FILE].bf + ob[BUF_FILE].n,
		      OUTBUFSZ - ob[BUF_FILE].n)) > 0)
    {
	ob[BUF_FILE].n += n;

	/* Write the Buffer to the next filter in the chain */
	bufwrite(BUF_FILE);
	return 0;
    }
    else
	return 1;
}


/* INITOUTPUT: should be called once by the main program during the
 * initialization stage.  It allocates memory for the first buffer in the
 * chain, and sets the rest too NULL.  These others will be allocated as
 * needed.
 */

void initoutput()
{
    int i;
    ob[BUF_FILE].bf= malloc(OUTBUFSZ+1);
    ob[BUF_FILE].bf[OUTBUFSZ]= '\0';
    ob[BUF_FILE].n= 0;
    for (i=1; i< MAXFIL; i++)
	ob[i].bf= NULL;
}

/* BUFWRITE: passes the contents of the buffer "in" to the next filter in the
 * chain, or prints it out if it hits the end.
 */

void bufwrite(int in)
{
    ob[in].i= 0;

    switch (in+1)	/* Cases must be coded in sequential order! */
    {
    case BUF_EATLINE:
	if (!opt[OPT_RAW].yes ||
	    !opt[OPT_SHOWNOISE].yes ||
	    ignoring != NULL ||
	    !opt[OPT_SHOWREAD].yes ||
	    !opt[OPT_SHOWEVENT].yes ||
	    !opt[OPT_REPEAT].yes)
	{
	    bufopen(BUF_EATLINE);
	    eatline(in);
	    bufflush(BUF_EATLINE);
	    break;
	}
    case BUF_EXPAND:
	if (opt[OPT_BS].yes != 1 || opt[OPT_CONTROL].yes != 1)
	{
	    bufopen(BUF_EXPAND);
	    expand(in);
	    bufflush(BUF_EXPAND);
	    break;
	}
    case BUF_WRAP:
	if (opt[OPT_WRAP].yes)
	{
	    bufopen(BUF_WRAP);
	    wrap(in);
	    bufflush(BUF_WRAP);
	    break;
	}
    default:
	if (write(out_fd, ob[in].bf, ob[in].n) < 0 &&
	    errno == EPIPE && out_fd != 1)
	{
	    /* Redo write if the filter died */
	    err("Filter Exited\n");
	    stop_filter();
	    write(out_fd, ob[in].bf, ob[in].n);
	}
	ob[in].n= 0;
    }
}


/* BPUTC:  Does the equivalent of a putchar() to the named buffer, passing
 * it to the next filter if it gets full.
 */

void bputc(int out,char ch)
{
    ob[out].bf[ob[out].n++]= ch;
    if (ob[out].n == OUTBUFSZ)
	bufwrite(out);
}

/* BPUTN:  Writes n characters from str to the named buffer, passing it to
 * the next filter if it gets full.
 */

void bputn(int out, char *str, int n)
{
    int space= OUTBUFSZ - ob[out].n;

    while (n >= space)
    {
	strncpy(ob[out].bf + ob[out].n, str, space);
	ob[out].n= OUTBUFSZ;
	bufwrite(out);
	str += space;
	n -= space;
	space= OUTBUFSZ - ob[out].n;
    }

    if (n > 0)
    {
	strncpy(ob[out].bf + ob[out].n, str, n);
	ob[out].n += n;
    }
}

/* BUFOPEN:  makes sure the named buffer is ready to write to.
 */

void bufopen(int out)
{
    if (ob[out].bf == NULL)
    {
	ob[out].bf= malloc(OUTBUFSZ+1);
	ob[out].bf[OUTBUFSZ]= '\0';
	ob[out].n= 0;
    }
}

/* BUFFLUSH: flushes out anything that may be in the named buffer.
 */

void bufflush(int out)
{
    if (ob[out].n > 0)
	bufwrite(out);
}

/* BGETC: Return the next character in the named buffer.  Return EOF if there
 * isn't anything left in it.
 */

int bgetc(int in)
{
    if (ob[in].i == ob[in].n)
    {
	ob[in].n= 0;
	return EOF;
    }
    return (unsigned char)ob[in].bf[ob[in].i++];
}

/* BGETS: Return the pointer to the begining of a line terminated by a newline,
 * or to a full buffer containing no newlines.  If there isn't a newline
 * in the buffer, and the buffer is not full, return NULL, making space in
 * the buffer so we can read in the rest of the line later.  The net effect
 * of this is kind of like gets().
 */

char *bgets(int in)
{
    register int j;
    int k;

    /* Check if input pointer is already at end */
    if (ob[in].i == ob[in].n)
    {
	    ob[in].n= 0;
	    return NULL;
    }

    /* Scan for end of line */
    for (j= ob[in].i; j < ob[in].n; j++)
    {
	    if (ob[in].bf[j] == '\n')
	    {
		    k= ob[in].i;
		    ob[in].i= j+1;
		    return ob[in].bf + k;
	    }
    }

    if (ob[in].i > 0)
    {
	    /* Move the useless remnants to the begining */
	    strncpy(ob[in].bf,
		    ob[in].bf + ob[in].i,
		    (k= ob[in].n - ob[in].i));
	    ob[in].n= k;
	    ob[in].i= 0;
	    return NULL;
    }

    if (ob[in].n == OUTBUFSZ)
    {
	    ob[in].i= OUTBUFSZ;
	    return(ob[in].bf);
    }
    else
	    return NULL;
}


/* EXPAND: Expand or eat control characters.
 */

void expand(int in)
{
    int out= BUF_EXPAND;
    int ch;
    int wasmeta;

    while((ch= bgetc(in)) != EOF)
    {
	if (ch == '\b')
	{
	    if (opt[OPT_BS].yes==1)
		bputc(out,ch);
	    else if (opt[OPT_BS].yes==2)
	    {
		bputc(out,'^');
		bputc(out,'H');
	    }
	    continue;
	}

	if (!isascii(ch))
	{
	    if (opt[OPT_CONTROL].yes==0)
		continue;
	    if (opt[OPT_CONTROL].yes==1)
		bputc(out,ch);
	    else
	    {
		bputc(out,'M');
		bputc(out,'-');
		ch= toascii(ch);
	    }
	    wasmeta= 1;
	}
	else
	    wasmeta= 0;

	if (iscntrl(ch) && (wasmeta || (ch != '\t' && ch != '\n')))
	{
	    if (opt[OPT_CONTROL].yes==1)
		bputc(out,ch);
	    else if (opt[OPT_CONTROL].yes==2)
	    {
		bputc(out, '^');
		bputc(out, (ch==0177) ? '?' : (ch + '@') );
	    }
	}
	else
	    bputc(out,ch);
    }
}


/* WRAP:  Do line wrapping */

void wrap(int in)
{
    int out= BUF_WRAP;
    int cols= convert(opt[OPT_COLS].str);
    int wrapindent= convert(opt[OPT_WRAP].str);
    char *c;		/* Pointer to current character */
    static char *line;	/* Pointer to beginning of current line */
    static char *space;	/* Pointer to last space character scanned */
    int col;		/* Number of columns in current line */
    int ind;		/* Number of unwrappable chars at head of line */
    int n;		/* Number of characters read from current line */
    int i;

    if (cols < 12) cols= 12;

    while((c= bgets(in)) != NULL)
    {
	n= col= 0;
	ind= wrapindent;
	line= space= c;
	for (; *c != '\n' && n < OUTBUFSZ; c++,n++)
	{
	    if (isprint(*c))
	    {
		col++;
		if (*c == ' ')
		    space= c;
	    }
	    else if (*c == '\t')
	    {
		col= col + 8 - (col % 8);
		space= c;
	    }
	    else if (*c == '\b')
		if (col > 0) col--;

	    if (col >= cols)
	    {
		if (space - line > ind)
		{
		    /* Break between words */
		    bputn(out,line,space-line);
		    c= space++;
		    line= space;
		}
		else
		{
		    /* Break inside word */
		    bputn(out,line,c-line);
		    line= space= c--;
		}
		bputc(out,'\n');
		for (i=0;i<wrapindent;i++)
		    bputc(out,' ');
		col= wrapindent;
		ind= 0;
	    }
	}
	bputn(out,line,c-line+1);
    }
}


/* EATLINE:  Strip out lines the user doesn't want to see */

void eatline(int in)
{
    int out= BUF_EATLINE;
    static char *line;
    static char oldline[BFSZ+INDENT+3]= "\n";
    char *colon;

    while((line= bgets(in)) != NULL)
	if ((opt[OPT_SHOWNOISE].yes || line[0] != '<') &&
	    (opt[OPT_SHOWREAD].yes || line[0] != ' ') &&
	    (opt[OPT_SHOWEVENT].yes || line[0] != '-') &&
	    (opt[OPT_REPEAT].yes || linecmp(line,oldline)) &&
	    (ignoring == NULL || !ignore_line(line)))
	{
	    if (line[0] != '<' || opt[OPT_RAW].yes ||
		(colon= lineindex(line+1,':')) == NULL)

		/* Pass line through unchanged */
		bputn(out,line,linelen(line));
	    else
	    {
		/* Delete name tag from noise */
		colon++;
		bputc(out,'<');
		bputn(out,colon,linelen(colon));
	    }
	    bputc(out,'\n');
	    if (!opt[OPT_REPEAT].yes) linecpy(oldline,line);
	}
}

/* LINECMP, LINECPY, LINELEN, LINEINDEX - just like strcmp, strcpy, strlen, and
 * index except lines are terminated by newlines instead of nulls.
 */

int linecmp(char *l1, char *l2)
{
    for (;;)
    {
	if (*l1 == '\n' || *l1 == '\0')
	    return (*l2 == *l1 ? 0 : -1);
	else if (*l1 != *l2)
	    return ((*l2=='\n' || *l2=='\0' || *l1 > *l2)? 1 : -1);
	l1++;
	l2++;
    }
}

char *linecpy(char *to, char *from)
{
    register char *t1= to;

    for (;;)
    {
	*to= *from;
	if (*to == '\n' || *to == '\0') return t1;
	to++;
	from++;
    }
}

int linelen(char *l)
{
    return firstin(l,"\n")-l;
}

char *lineindex(char *ln, char ch)
{
    for ( ; ; )
    {
	if (*ln == ch) return ln;
	if (*ln == '\n' || *ln == '\0') return NULL;
	ln++;
    }
}

/* GETCOLS: This sets the number of columns from the termcap, if possible.
 * If not it leaves it unchanged.  If the windows variable is set, it asks
 * the kernel instead.  This is only used if we don't have the termcap
 * library.
 */

#ifdef WINDOWS

RETSIGTYPE setcols()
{
    struct winsize x;

   ioctl(2,TIOCGWINSZ,&x);
   if (x.ws_col > 11)
       setnum(OPT_COLS,x.ws_col);
}

#else

RETSIGTYPE setcols()
{
    char *term;
    int cols;
    char bf[1024];

    if ((term= getenv("TERM")) == NULL || tgetent(bf,term) < 1)
	return;
	 
    if ((cols= tgetnum("co")) != -1 && cols > 11)
	setnum(OPT_COLS,cols);
}
#endif /*WINDOWS*/
