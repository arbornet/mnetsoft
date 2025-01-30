/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Selector manipulation routines
 *
 * Selector are made of various things separated by commas and dashes.  Like
 *
 *   dog,cat,mouse,lemon
 *   2,4,6,8
 *   12-30
 *   30-12
 *   2,13-15,10,7
 *   2,13-$
 *
 * Dashes can only be used between two numbers, or between a dollar-sign and
 * a number.
 */

#include "backtalk.h"

#include <ctype.h>

#include "sel.h"
#include "stack.h"
#include "free.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int atopi(char *b, char *e, int l);

#define DELIM	",-"
#define DIGIT	"0123456789"


/* REV_SEL -- Reverse a selector.  Thus "2,12,1003" becomes "1003,12,2"; and
 * "1,13-15,10,7" becomes "7,10,15-13,1"; and "cat,dog" becomes "dog,cat".
 */

void rev_sel(char *sel)
{
    char *a, *b;

    /* First reflect each word */
    for (a= firstout(sel,DELIM); *a != '\0'; a= firstout(b,DELIM))
    {
	b= firstin(a,DELIM);
	reflect(a,b-1);
    }

    /* Now reflect the whole string */
    reflect(sel,a-1);
}

/* FUNC_REV_SEL:  <list> rev_sel <list>
 * Reverse a selector.  Thus "2,12,1003" becomes "1003,12,2"; and
 * "1,13-15,10,7" becomes "7,10,15-13,1"; and "cat,dog" becomes "dog,cat".
 */

void func_rev_sel()
{
    rev_sel(peek_string());
}


/* FUNC_COUNT_SEL:  <sel> <last> count_sel <number>
* Count the number of things in a selector.  Thus "2,12,1003" contains 3
* objects and "1,13-15,10,7" contains 6.  If any dollar-sign appears in the
* selector, it is treated as 'infinity'.  Only numbers less than or
* equal to 'last' are counted, unless last is less than zero.
*/

void func_count_sel()
{
    int last= pop_integer();
    char *sel= pop_string();
    char *a,*b;
    int n,r1,r2;
    char *delim= (last < 0) ? "," : DELIM;

    n= 0;

    r1= -1;
    for (a= firstout(sel,delim); *a != '\0'; a= firstout(b,delim))
    {
	b= firstin(a,delim);
	if (last < 0)
	{
	    /* Negative last value - just count comma-separated terms */
	    n++;
	}
	else if (*b == '-')
	{
	    /* First number in an "n-n" pair */
	    r1= atopi(a,b,-2);
	}
	else if (r1 != -1 && (r2= atopi(a,b,-2)) != -1)
	{
	    if (r1 == -2)
		/* Second number in an "$-n" pair */
		n+= (r2 == -2 || r2 > last) ? 0 : last - r2 + 1;
	    else if (r2 == -2)
		/* Dollar sign in an "n-$" pair */
		n+= (r1 > last) ? 0 : last - r1 + 1;
	    else
		/* Second number in an "n-n" pair */
		n+= abs(r2-r1) + 1;
	    r1= -1;
	}
	else if (atoi(a) <= last)
	    /* Isolated number */
	    n++;
    }
    free(sel);
    push_integer(n);
}


/* ATOPI - convert the string starting at b, ending before e, to a positive
* (or zero) integer.  If it is $, return l.  If it is any other
* non-integer, return -1.
*/

int atopi(char *b, char *e, int l)
{
    int n;

    if (*b == '$' && b+1 == e)
	return l;
    n= 0;
    while (b < e)
    {
	if (!isascii(*b) || !isdigit(*b))
	    return -1;
	n= n*10 + *b - '0';
	b++;
    }
    return n;
}
    

/* FUNC_NEXT_INT:  <sel> <last> next_int <rest> <first>
* Pull the next integer off an integer selector.  <last> gives the value
* of a dollar sign.
*
* NEXT_INT - does the same thing, but returns the the <first> integer instead
* of pushing it.  Last_item gives the value of a '$'.  If it is negative,
* it is used only when $ is the first term in a range.  If it is the second
* term, it is treated as an infinity.
*/

int next_int(int last_item)
{
    char *sel= peek_string();
    char *a,*b,*c;
    int this,next,nextlen,final,dollar;
    int to_infinity= (last_item < 0);
    last_item= (last_item > 0) ? last_item : -last_item;

    for (;;)	/* rarely iterates */
    {
	/* Find start of first number (or dollar) */
	a= firstin(sel,DIGIT"$");
	if (*a == '\0') return -1;

	/* Find end of first number (or dollar) */
	if (dollar= (*a == '$'))
	    b= a+1;
	else
	b= firstout(a+1,DIGIT);/* end of 1st number */

	/* Convert 1st number */
	if ((this= atopi(a,b,last_item)) < 0)
	    return -2;

	/* Find next delimiter (if there is one) */
	b= firstin(b,DELIM"$"DIGIT);
	    
	/* Find start of next number */
	a= firstin(b,DIGIT"$");

	if (*b == '\0')
	{
	    /* Have used up selector */
	    *sel= '\0';	
	}
	else if (*b == ',' || (*b >= '0' && *b <= '9'))
	{
	    /* Have used up 1st field */
	    memmove(sel,a,strlen(a)+1);	
	}
	else
	{
	    /* Find next number in range */
	    if (*a == '$' || *a == '\0')
	    {
		next= this + 1;
		if (to_infinity)
		    final= next + 1;
		else
		    final= last_item;
	    }
	    else
	    {	
		c= firstout(a,DIGIT);
		if ((final= atopi(a,c,last_item)) < 0)
		    return -2;
		if (final < this)
		    next= this - 1;
		else if (dollar && final != this)
		    /* had $-number with number > last_item */
		    this= final= -1;  /* Throw it away and iterate */
		else
		    next= this + 1;
	    }
	    
	    if (final == this)
	    {
		/* Delete range entirely */
		a= firstin(firstout(a,DIGIT"$"),DIGIT"$");
		memmove(sel,a,strlen(a)+1);
	    }
	    else if (final == next && *a != '$')
		/* Collapse range to one number */
		memmove(sel,a,strlen(a)+1);
	    else
	    {
		/* Adjust first number in range */
		char bf[20];
		sprintf(bf,"%d",next);
		nextlen= strlen(bf);

		if (nextlen > b-sel)
		{
		    char *p= (char *)malloc(nextlen+strlen(b)+1);
		    strcpy(p,bf);
		    strcpy(p+nextlen,b);
		    free(sel);
		    repl_top(p);
		}
		else
		{
		    strncpy(sel,bf,nextlen);
		    if (nextlen < b-sel)
			memmove(sel+nextlen, b, strlen(b)+1);
		}
	    }
	}

	if (this >= 0) break;
    }
    return this;
}


void func_next_int()
{
    push_integer(next_int(pop_integer()));
}


/* IN_SEL - Return true if the given integer is in the given integer selector.
 * The selector is assumed to be in correct syntax, with no spaces and no
 * characters other than digits, commas, dollar signs and dashes.  It
 * treats '$' as infinity, so it understands "1-$" and "$-1", but no integer
 * will ever match "$".
 */

int in_sel(int i, char *sel)
{
    char *wrd, *end, *dash;

    for (wrd= sel; ; wrd= end+1)
    {
	/* Find the end and dash of the current word */
	dash= NULL;
	for (end= wrd; *end != '\0' && *end != ','; end++)
	    if (*end == '-') dash= end;

	if (dash != NULL)
	{
	    int a= (wrd[0] == '$') ? i+1 : atoi(wrd);
	    int b= (dash[1] == '$') ? i+1 : atoi(dash+1);
	    if ((a <= i && i <= b) || (b <= i && i <= a))
		return TRUE;
	}
	else if (atoi(wrd) == i)
	    return TRUE;

    	if (*end == '\0')
    	    return FALSE;
    }
}


/* FUNC_IN_SEL:  <value> <selector> in_sel <bool>
 * Pushes true if the given number or name is in the given selector.  The
 * value can be either an integer or numeric token.
 */

void func_in_sel()
{
    char *sel= pop_string();
    Token val;

    pop_any(&val);

    if (type(val) == TK_INTEGER)
    	push_integer(in_sel(ival(val),sel));

    else if (type(val) == TK_STRING)
    {
        int len= strlen(sval(val));
        int flag= 0;
        char *w= sel;
        for (;;)
        {
            if (!strncmp(w,sval(val),len) && (w[len] == '\0' || w[len] == ','))
            {
            	flag= 1;
            	break;
	    }
	    if ((w= strchr(w,',')) == NULL)
	    	break;
	    w++;
	}
	free_val(&val);
	push_integer(flag);
    }
    else
    	die("First argument must be string or integer");

    free(sel);
}
