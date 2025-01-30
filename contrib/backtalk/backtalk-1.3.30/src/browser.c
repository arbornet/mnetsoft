/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include <ctype.h>
#include "backtalk.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* SEARCH_BROWSER - search the list of browsers for a match.
 * Returns -2 if not found.  (-1 is a valid browser class.)
 */

#ifdef BROWSER_FILE
int search_browser(char *ident)
{
    FILE *fp;
    char bf[BFSZ+1];
    char *p;
    int class;

    if ((fp= fopen(BROWSER_FILE,"r")) == NULL)
    	return -2;

    while (fgets(bf,BFSZ,fp) != NULL)
    {
    	if (bf[0] == '#' || (p= strchr(bf, '\t')) == NULL) continue;

	if (!strncmp(ident,bf,p-bf))
	{
	    p= firstin(p,"-0123456789");
	    if (*p == '\0') continue;
	    class= atoi(p);
	    fclose(fp);
	    return class;
	}
    }
    fclose(fp);
    return -2;
}
#endif /* BROWSER_FILE */


/* BROWSER_CLASS - Return the browser class for the current user's browser.
 */

int browser_class()
{
    static int b_class= -1;
    char *p,*ident;
    int v;

    /* If we've already identified it, just return same value */
    if (b_class >= 0)
    	return b_class;

    /* Fetch Identification string */
    if ((ident= getenv("HTTP_USER_AGENT")) == NULL)
	return (b_class= 3);	/* Assume unidentifiable browsers are great */

    /* Check for Netscape or some other browser spoofing Netscape */
    if ((p= strstr(ident,"Mozilla/")) != NULL)
    {
	/* Get major version number */
	if (isascii(p[8]) && isdigit(p[8]))
	    v= atoi(p+8);
	else
	    return (b_class= 2);

	switch(v)
	{
	case 0:  return (b_class= 1);	/* Or 2?  I don't know. */
	case 1:  return (b_class= 2);
	case 2:
	case 3:
	default: return (b_class= 3);
	}
    }

    /* Check for Microsoft */
    if (!strncmp(ident,"Microsoft Internet Explorer",27))
	return (b_class= 3);

    /* Check for Lynx */
    if (!strncmp(ident,"Lynx/",5))
    {
	/* Lynx 2.5 does lousy tables, lynx 2.7 is good enough */
	/* We'll assume the worst about 2.6 */
	if (ident[5] > '2' || ident[7] > '6')
	    return (b_class= 1);
	else
	    return (b_class= 0);
    }

    /* Search the file, if we have one */
#ifdef BROWSER_FILE
    if ((b_class= search_browser(ident)) != -2)
    	return b_class;
#endif

    /* Default */
    return (b_class= 1);
}


/* FUNC_BROWSER:  - browser <code>
 *
 * Classify the user's browser.  Current classes are:
 *   -1 - something annoying - refuse to run for it.
 *    0 - non-graphical, dumb browser.  Basically lynx.
 *    1 - basic HTML 2.0 browser.  No tables.
 *    2 - HTML 3.0 browser or Netscape version 1.  Tables, but no frames.
 *    3 - Netscape 2+ or Internet Explorer, frames.
 *
 * Higher numbers may be added in the future.  A few of the more common
 * Browsers are hard coded into this routine.  For the others, we seach
 * the browser database, which contains lines each containing a string, a
 * tab, and a number.  The first string in the file which matches an initial
 * substring of HTTP_USER_AGENT is used.  If no match is found, we default
 * to class 1.
 */

void func_browser()
{
    push_integer(browser_class());
}
