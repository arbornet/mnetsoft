/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#include <ctype.h>

#include "udb_check.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* CSTR - Expand a character into a string, by expanding it if it is a
 * control character.
 */

char *cstr(int ch)
{
    static char bf[10];
    int i;

    if (ch > 255)
    {
	ch= ch%256;
	strcpy(bf,"M-");
	i= 2;
    }
    else
	i= 0;
    
    if (ch == '\177')
	strcpy(bf+i,"^?");
    else if (ch < ' ')
    {
	bf[i++]= '^';
	bf[i++]= ch+'@';
	bf[i]= '\0';
    }
    else
    {
	bf[i++]= ch;
	bf[i]= '\0';
    }
    return bf;
}


/* FULLNAME_BAD - Sanity checking for fullnames.  They should:
 *   - be between 2 and 40 characters long.
 *   - contain no colons
 *   - contain no control-characters
 *   - contain no backslashes, quotes or exclamation points.
 * We allowing commas.  Leading and trailing spaces are deleted, and tabs
 * are converted to spaces.
 */

int fullname_bad(char *fname, char *msg)
{
    int len, ebc= 0;
    char *p;
    static char *badchars= ":\\\"";

    if (fname == NULL)
    {
	strcpy(msg,"No full name given.\n");
	return 1;
    }

    /* Delete Leading spaces */
    while (*fname == ' ' || *fname == '\t')
	fname++;

    /* Check for bad characters (and map tabs to spaces) */
    for (p= fname; *p != '\0'; p++)
    {
	if (*p == '\t') *p= ' ';

	if (!ebc && (!isascii(*p) || !isprint(*p) ||
		     strchr(badchars,*p) != NULL))
	{
	    sprintf(msg,"Illegal '%s' character in full name.\n",cstr(*p));
	    return 1;
	}
    }

    /* Delete Trailing spaces */
    for ( ; p[-1] == ' '; p--)
	;
    *p= '\0';

    /* Check name length */
    len= p - fname;
    if (len == 0)
    {
	strcpy(msg,"No full name given.\n");
	return 1;
    }
    else if (len < 2)
    {
	strcpy(msg,"Full name too short.\n");
	return 1;
    }
    else if (len > 40)
    {
	strcpy(msg,"Full name too long.\n");
	return 1;
    }
    return 0;
}


/* PASSWD_BAD - Sanity checking for passwords.  Passwords must:
 *   - be between 4 and 40 characters long
 *   - be different from the login name
 *   - not be "password"
 *
 * These are very mild rules, and should arguably be tightened up further.
 */

int passwd_bad(char *pw, char *lid, char *msg)
{
    int len;

    if (pw == NULL || (len= strlen(pw)) == 0)
    {
	strcpy(msg,"No password given.\n");
	return 1;
    }
    else if (len < 4)
    {
	strcpy(msg,"The password you selected is too short.\n");
	return 1;
    }
    else if (len > 40)
    {
	strcpy(msg,"The password you selected is too long.\n");
	return 1;
    }

    if ((lid != NULL && !strcmp(pw,lid)) || !strcmp(pw,"password"))
    {
	strcpy(msg,"The password you selected is too obvious.\n");
	return 1;
    }
    return 0;
}
