#include <stdio.h>
#include "nu.h"
#include "wnu.h"

/* VERSION HISTORY:
 * 03/26/96 - Last version before I started collecting version history. [jdw]
 * 06/24/97 - Added version history. [jdw]
 * 12/29/03 - Revisions to avoid modifying fwrtpara(). [jdw]
 */

#ifndef MINREALNAME
#define MINREALNAME 3
#endif

char *check_eof(char *val);
char *check_fullname(char *val);
char *check_login(char *val);
char *check_passwd(char *val);
char *check_shell(char *val);
char *check_editor(char *val);
char *check_terminal(char *val);
char *check_backspace(char *val);
char *check_interupt(char *val);
char *check_kill(char *val);
char *clean_text(char *val);
char *set_paranoia(char *val);
char *set_sex(char *val);


/* Table of Query variables read by wnu:  for each, the named procedure is
 * called first, with the value as an argument, and then the value is stored
 * in the given variable.  Note that even erroneous and illegal values are
 * stored, but it is OK, because we never call add_user if errors are detected.
 */

#define VF_PARA 1	/* multi-line paragraph */

struct uinfo {
	char *name;
	char *(*process)();
	char **storage;
	} vartab[] = {
	 { "login",		check_login,	&loginid },
	 { "passwd1",		check_passwd,	&userpass },
	 { "passwd2",		check_passwd,	&userpass },
	 { "fname",		check_fullname,	&full_name },
	 { "shell",		check_shell,	&shell },
	 { "editor",		check_editor,	&editor },
	 { "term",		check_terminal,	NULL },
	 { "backspace",		check_backspace,NULL },
	 { "interupt",		check_interupt,	NULL },
	 { "kill",		check_kill,	NULL },
	 { "birth",		NULL,		&birthdate },
	 { "sex",		set_sex,	NULL },
	 { "email",		clean_text,	&email },
	 { "occupation",	clean_text,	&occupation },
	 { "interests",		clean_text,	&interests },
	 { "computers",		clean_text,	&computers },
	 { "how",		clean_text,	&how },
	 { "address",		clean_text,	&address },
	 { "telephone",		NULL,		&telephone },
	 { "paranoid",		set_paranoia,	NULL },

	 {NULL,			NULL,		NULL}};


/* GET_USER_INFO - Load the arguments from the cgi query into the various
 * appropriate global variables where the add_user program expects to find
 * them.  Do the first level of validity checking to assure that the values
 * describe a sensible account.  Output a page of error messages if anything
 * is improper, and return non-zero (actually, return the reject code).
 */

char nothing[1]= "";
int wnu_get_user_info(char *query)
{
    int i;
    char *val;

    reject= 0;

    load_query(query);

    loginid= NULL;
    userpass= NULL;

    for (i= 0; vartab[i].name != NULL; i++)
    {
	val= getvar(vartab[i].name);

	if (val == NULL) val= nothing;

	if (strlen(val) > 4096)
	    err(USER,"Your %s value (<KBD>%#.25S...</KBD>) is much too long.\n",
		vartab[i].name,val);

	if (vartab[i].process != NULL)
	    val= (*vartab[i].process)(val);

	if (vartab[i].storage != NULL && val != NULL)
	    *vartab[i].storage= val;
    }

    /* Check for duplicate keys, now that we have them all */
    final_key_check();

    /* Finish up the login check, with passwd file locked */
    final_login_check();

    /* Don't do "end_query()" -- we are still using that storage */

    return(reject);
}


/* CHECK_FULLNAME - Sanity checking for fullnames.  They should:
 *   - be between MINREALNAME and 40 characters long.
 *   - contain no colons (trying this is considered hostile).
 *   - contain no control-characters
 *   - contain no backslashes, quotes or exclamation points. (Marcus
 *     apparantly wants them to be quotable strings in the shell).
 * We follow Marcus in allowing commas.  Leading and trailing spaces are
 * deleted, and tabs are converted to spaces.
 */

char *check_fullname(char *val)
{
    int len, ebc= 0;
    char *p;
    static char *badchars= ":\\!\'\"";

    /* Delete Leading spaces */
    val= firstout(val," \t");

    /* Check for bad characters (and map tabs to spaces) */
    for (p= val; *p != '\0'; p++)
    {
	if (*p == '\t') *p= ' ';

	if (!ebc && (!isascii(*p) || !isprint(*p) ||
		     index(badchars,*p) != NULL))
	{
	    err((*p == ':')?HOSTILE|USER:USER,
		"No <KBD>%#C</KBD> characters are permitted "
		"in your <STRONG>Full Name</STRONG> field.\n",*p);
	    ebc= 1;
	}
    }

    /* Delete Trailing spaces */
    for ( ; p[-1] == ' '; p--)
	;
    *p= '\0';

    /* Check name length */
    len= p - val;
    if (len == 0)
	err(USER,"You did not fill in the <STRONG>Full Name</STRONG> field.\n");
    else if (len < MINREALNAME)
	err(USER,"The <STRONG>Full Name</STRONG> you gave "
	    "(<KBD>%#S</KBD>) is too short.\n",val);
    else if (len > 40)
	err(USER,"The <STRONG>Full Name</STRONG> you gave "
	    "(<KBD>%.30S...</KBD>) is too long.\n",val);

    return val;
}


/* CHECK_FLIST - Check stuff in flist structures - shells and editors. Return
 * the index of the matching entry if successful, or -1 otherwise.
 */

int check_flist(char *val, char *what, struct flist *fl)
{
    int i;

    if ((i= kwscan(val, fl->fl_keys)) == 0)
    {
	err(USER|HOSTILE,"Illegal value selected for %s.\n",what);
	printf("Possible values are<KBD>");
	for (i= 1; i <= fl->fl_many ; i++)
	{
	    printf(" %s",fl->fl_name[i]);
	    if (i != fl->fl_many)
		putchar(',');
	    if (i == fl->fl_many - 1)
		fputs("</KBD> or<KBD>",stdout);
	}
	printf(".</KBD>\n");
	return(-1);
    }
    return(i-1);
}


/* CHECK_SHELL - Sanity checking for login shells.  The only legal shells are
 * the ones defined in nuconfig.  If the user submits anything else, the
 * query can't have been generated off our form, so the user is considered
 * hostile.
 */

char *check_shell(char *val)
{
    int i;

    if ((i= check_flist(val, "shell", &sh)) >= 0)
    {
	shellpath= sh.fl_towhat[i];
	bang= sh.fl_bang[i];
    }
    return val;
}

/* CHECK_EDITOR - Just like check_shell */

char *check_editor(char *val)
{
    int i;

    if ((i= check_flist(val, "editor", &ed)) >= 0)
	edpath= ed.fl_towhat[i];
    return val;
}


/* CHECK_TERMINAL - Fix up terminal value, and check for sanity.  Sane, in
 * this case, means it is either on the short-list or in /etc/termcap.  If
 * he got an illegal answer off a check-button, he is considered hostile.
 */

char *check_terminal(char *val)
{
    char bp[1024];
    static char *short_list[]= {"ansi","vt100",NULL};
    int is_other;
    char **t, *e0;

    /* Handle the "other" options on the checklist */
    if (is_other= !strcmp(val,"other"))
	val= getvar("otherterm");
    
    if (val == NULL || *val == '\0')
    {
	if (is_other)
	    err(USER,"You selected the 'other' option for "
		"<STRONG>Default Terminal Type</STRONG>,\nbut didn't give a "
		"name for your terminal.\n");
	else
	    err(USER,"No <STRONG>Default Terminal Type</STRONG> was "
		"specified.\n");
	return val;
    }

    /* Check the short list */
    for (t= short_list; *t != NULL; t++)
	if (!strcmp(val,*t))
	    break;

    /* Check the /etc/termcap file */
    
    if (*t == NULL && tgetent(bp,val) <= 0)
    {
	err(is_other?USER:HOSTILE|USER,"The <STRONG>Terminal Type</STRONG> "
	   "you gave (%#S) is unknown to %s.\n",val,system_name);
	return val;
    }

    /* We do the assignment here to handle the "other" case right */
    return terminal= val;
}

/* SET_PARANOIA - set the paranoid variable.
 */

char *set_paranoia(char *val)
{
	paranoid= (*val == 'y' || *val == 'Y');
	return val;
}

/* SET_SEX - set the sex variable to a very limited range of options.  We
 * don't complain about other things.  We just map it to Male, Female, or
 * unspecified.
 */

char *set_sex(char *val)
{
	val= firstout(val," ");
	if (*val == 'm' || *val == 'M')
		sex= "Male";
	else if (*val == 'f' || *val == 'F')
		sex= "Female";
	else
		sex= "";
	return val;
}


/* WRAP_TEXT - this goes through a long string of text and changes spaces or
 * tabs to newlines often enough to fit it on an 80 column screen (if
 * possible).  This is not terribly intelligent, but shouldn't make too
 * big a hash of things.  It neither lengthens nor shortens the array.
 */

#define MAXCOL 71	/* Break lines longer than this */
#define MINCOL 48	/* If breaking a line makes a line shorter than this
			   join it to the next one */

void wrap_text(char *c)
{
    int col= 0;
    char *last_space= NULL;
    int have_wrapped= 0;

    for ( ; *c != '\0'; c++)
    {
	/* His line too long and we have seen a space - turn it into newline */
	if (col > MAXCOL && *c != '\n' && last_space != NULL)
	{
	     *last_space= '\n';
	     col= c - last_space;
	     last_space= NULL;
	     have_wrapped= 1;
	}
	switch (*c)
	{
	case '\n':
	    if (have_wrapped && col < MINCOL &&
		c[1] != '\n' && c[1] != ' ' && c[1] != '\0')
	    {
		/* Join the line to the next one */
		*c= ' ';
		col++;
		last_space= c;
	    }
	    else
	    {
		/* Accept his newline */
		last_space= NULL;
		col= 0;
		have_wrapped= 0;
	    }
	    break;
	case '\t':
	    *c= ' ';
	case ' ':
	    if (col > MAXCOL)
	    {
		*c= '\n';
		last_space= NULL;
		col= 0;
	    }
	    else
	    {
		last_space= c;
		col++;
	    }
	    break;
	default:
	    col++;
	    break;
	}
    }
}

/* CLEAN_TEXT - This cleans up paragraphs.  Portions adapted from mdw's
 * fix_para() routine, but that can't be used because it doesn't realloc()
 * when it lengthens the string.  It breaks long lines, deletes leading
 * and trailing empty lines, deletes \r's and makes sure we have a terminal
 * newline.
 */


char *clean_text(char *val)
{
    char *cp1, *cp2;
    char c;
    int i, start;
    char *end, *bufend;

    /* Pass 1 - strip carriage returns */
    for (cp1= cp2= val; (c = *cp1) != '\0'; ++cp1)
	if (c != '\r')
		    *(cp2++)= c;
    *cp2= '\0';
    bufend= cp1;
    end= cp2;

    /* Pass 2 - delete leading blank lines */
    cp1= val;
    start= 0;
    while ((c= *cp1++) != '\0' && isspace(c))
	if (c == '\n') start= cp1-val; /* was blank line, try next one */

    if (c == '\0') return cp1;    /* if nothing left, return empty string. */

    /* Pass 3 - delete trailing blank lines and check for trailing newline */

    /* find last non-space */
    for (cp1= end-1; isspace(*cp1);  cp1--)
	;
    /* Place \n \0 right after last non-space */
    if (cp1 < bufend-1)
    {
	/* can append a newline in place */
	cp1[1]= '\n';
	cp1[2]= '\0';
    }
    else
    {
	/* Need to enlarge buffer before appending newlines */
	i= cp1-val;
	cp2= (char *)malloc(i+3);
	bcopy(val, cp2, i+1);
	cp2[i+1]= '\n';
	cp2[i+2]= '\0';
	val= cp2;
    }

    /* Pass 4: wrap */
    wrap_text(val+start);

    return val+start;
}
