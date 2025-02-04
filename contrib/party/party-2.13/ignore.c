/* PARTY PROGRAM -- ROUTINES TO IGNORE OBNOXIOUS USERS -- Jan Wolter */

#ifndef NOIGNORE

#include "party.h"
#include "opt.h"

/* Head of linked list of ignored users.  This list is kept sorted. */
struct ign *ignoring= NULL;


/* ADDIGNORE -- Add the named user to the ignore list.  Returns 1 if user is
 * already on the ignore list.  Returns 0 otherwise.
 */

int addignore(char *user)
{
    struct ign *new= (struct ign *)malloc(sizeof(struct ign));
    struct ign *curr, *prev;
    int rc;

    if (strlen(user) > UT_NAMESIZE) user[UT_NAMESIZE]= '\0';

    for (prev= NULL, curr= ignoring; curr != NULL; prev= curr, curr= curr->next)
    {
	rc= strcmp(user, curr->name);
	if (rc == 0) return(1);
	if (rc < 0) break;
    }

    strcpy(new->name, user);
    new->next= curr;
    if (prev == NULL)
    	ignoring= new;
    else
    	prev->next= new;

    return(0);
}


/* DELIGNORE -- Remove the named user from the ignore list.  Returns 1 if the
 * user is not on the ignore list, 0 otherwise.
 */

int delignore(char *user)
{
    struct ign *curr, *prev;
    int rc;

    for (prev= NULL, curr= ignoring; curr != NULL; prev= curr, curr= curr->next)
    {
	rc= strcmp(user, curr->name);
	if (rc == 0)
	{
	    if (prev == NULL)
		ignoring= curr->next;
	    else
	    	prev->next= curr->next;
	    free(curr);
	    return(0);
	}
	if (rc < 0) break; 
    }
    return(1);
}


/* NOIGNORE -- Remove all users from the ignore list.  */

void noignore()
{
    struct ign *next, *curr;

    for (curr= ignoring; curr != NULL; curr= next)
    {
	next= curr->next;
        free(curr);
    }
    ignoring= NULL;
}


/* LISTIGNORE -- List all the users currently being ignored. */

void listignore()
{
    struct ign *curr;
    int cols= convert(opt[OPT_COLS].str);
    int col= 0, len, n= 0;

    for (curr= ignoring; curr != NULL; curr= curr->next)
    {
	len= strlen(curr->name);
	if (n++ == 0)
	    fputs("Currently ignoring:\n",stdout);

	if (col != 0)
	{
	    if (col + len + 1 >= cols)
	    {
		putchar('\n');
		col= 0;
	    }
	    else
	    {
	    	putchar(' ');
	    	col++;
	    }
	}
	fputs(curr->name,stdout);
	col+= len;
    }
    if (col != 0)
	putchar('\n');
    if (n == 0)
	fputs("You are not ignoring any other users.\n",stdout);
}


/* AM_IGNORING -- Return true if we are ignoring the named user. */

int am_ignoring(char *user)
{
    struct ign *curr;
    int rc;

    for (curr= ignoring; curr != NULL; curr= curr->next)
    {
	rc= strcmp(user, curr->name);
	if (rc == 0) return(1);
	if (rc < 0) break;
    }
    return(0);
}


/* IGNORE_LINE -- Return true if we should ignore the given line (which is
 * terminated by a newline).
 */

int ignore_line(char *line)
{
    int ignored_last= 0;
    char *name, *nameend;
    int termchar;

    if (ignoring == NULL) return(0);

    switch(line[0])
    {
    case ' ':		/* Reads -- same as previous line */
    	return(ignored_last);

    case '~': 		/* Name Change -- track change if it is our guy */
	if (opt[OPT_UNIQUENAME].yes)
	{
	    char *newname, *newnameend;

	    name= line+5;
	    nameend= lineindex(name,' ');
	    if (nameend == NULL) return(ignored_last= 0);

	    newname= nameend+12;
	    newnameend= lineindex(newname,'\n');
	    if (newnameend == NULL) return(ignored_last= 0);

	    *nameend= '\0';
	    *newnameend= '\0';
	    if (ignored_last= !delignore(name))
	    	addignore(newname);
	    *nameend= ' ';
	    *newnameend= '\n';
	    return(ignored_last);
	}
	else
	    /* Never ignore name change lines in nouniquename channels */
	    return(ignored_last= 0);

    case '<':		/* Noises -- pick out the tag */
    	name= line+1;
    	nameend= lineindex(name,':');
	termchar= ':';
    	break;

    case '-':		/* Control Lines -- name is after "---- " */
    	name= line+5;
    	nameend= lineindex(name,' ');
    	termchar= ' ';
    	break;

    default:		/* Ordinary text -- name is start of line */
    	name= line;
    	nameend= lineindex(name,':');
    	termchar= ':';
    	break;
    }

    if (nameend == NULL) return(ignored_last= 0);
    *nameend= '\0';
    ignored_last= am_ignoring(name);
    *nameend= termchar;
    return(ignored_last);
}
#endif /*NOIGNORE*/
