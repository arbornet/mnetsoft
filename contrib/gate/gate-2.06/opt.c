#include "gate.h"

/*	OPTION TABLE
 *
 * Options are stored in individual global variables.  The option table
 * provides an index to those variables, giving their name, type, address,
 * range, and possibly a function to call before setting them.  This supports
 * uniform parsing and printing of options.
 *
 * If no function is given, the value is simply stored in the given variable.
 * If a function is given, it is called with the value to be assigned and the
 * source, but the value is only assigned if it returns a non-zero value.  The
 * function can be used either to do further checks on the value assigned, or
 * to perform side-effects for the assignment.
 */

/* Option Variables - Default values are set here */

int askok=0;		/* Should we always ask if OK to enter? */
int backwrap=1;		/* Allow backspacing onto previous lines? */
char cmdchar[2]= ":";	/* Prefix for commands */
int hotcol=75;		/* Last column (one based) a space may appear in */
/* char language[];	/* Defined in spel.c */
int maxcol=79;		/* Last column (one based) anything may appear in */
int novice=1;		/* Is the user a novice? */
int outdent= 1;		/* Amount output will be offset when displayed */
char prompt[71]=">";	/* Input prompt */
int secure=1;		/* Should we keep buffer file depermitted? */
int spellcheck=0;	/* Should we check spelling on exit? */

/* Option types */

#define OT_NOYES 0
#define OT_NOYESASK 1
#define OT_INTEGER 2
#define OT_STRING 3

struct {
	char *name;	/* Name of option - NULL indicates end of table */
	char type;	/* Type of option: 0=boolean, 1=integer */
	void *store;	/* Where to store the value of the option */
	int (*func)();	/* Function to call before saving */
	int min,max;	/* Range of values (for integer type)... */
			/* ...of string length (for string type) */
	} opttab[] = {

	{"askok",    OT_NOYES,   (void *)&askok,     NULL,         0,1},
	{"backwrap", OT_NOYES,   (void *)&backwrap,  NULL,         0,1},
	{"cmdchar",  OT_STRING,  (void *)cmdchar,    NULL,         1,1},
	{"hotcol",   OT_INTEGER, (void *)&hotcol,    NULL,         0,BUFSIZE},
#if !defined(GISPELL) && !defined(NO_SPELL)
	{"language", OT_STRING,  (void *)language,   set_language, 1,LANGSIZE},
#endif
	{"maxcol",   OT_INTEGER, (void *)&maxcol,    NULL,         2,BUFSIZE},
	{"novice",   OT_NOYES,   (void *)&novice,    NULL,         0,1},
	{"outdent",  OT_INTEGER, (void *)&outdent,   set_outdent,  0,BUFSIZE},
	{"prompt",   OT_STRING,  (void *)prompt,     set_prompt,   0,70},
	{"secure",   OT_NOYES,   (void *)&secure,    NULL,         0,1},
#ifndef NO_SPELL
	{"spell",    OT_NOYESASK,(void *)&spellcheck,NULL,         0,2},
#endif
	{NULL,       0,          (void *)NULL,       NULL,         0,0}};



/* PARSE_OPTS - parse an option string and store the values of the flags */

void parse_opts(char *string, char *source)
{
char *nxt, *beg, *end, *arg;
int saw_no,i,val,saw_ask,saw_eq;
char *space=" \t\n\r";		/* stuff that can be between keywords */
char *c;

    if (string == NULL) return;

    nxt= firstout(string,space);

    while (*nxt != '\0')
    {
	beg= nxt;
	end= firstin(beg,(*source == 'o') ? "" : space);
	arg= firstin(beg,"=");
	if (saw_eq= (arg < end))
	{
	    *(arg++)= '\0';
	    if (*source != 'o' &&
	        (*arg == '\'' || *arg == '"') &&
	        (c= strchr(arg+1,*arg)) != NULL)
	    {
		arg++;
		*c= '\0';
		end= c+1;
	    }
	}
	nxt= firstout(end,space);
	*end= '\0';

	/* Search table of options for the name */
	for (i= 0; ; i++)
	{
	    if (opttab[i].name == NULL)
	    {
		printf("%s error: Unknown option %s%s\n",
			source, saw_no?"no":"", beg);
		break;
	    }
	    if (!(saw_ask= saw_no= strcmp(beg,opttab[i].name)) ||
	        !(saw_ask= !(saw_no= (beg[0] == 'n' && beg[1] == 'o' &&
		 !strcmp(beg+2,opttab[i].name)))) ||
	        !(saw_no= !(saw_ask=
			(beg[0] == 'a' && beg[1] == 's' && beg[2] == 'k' &&
		 !strcmp(beg+3,opttab[i].name)))))
	    {
		switch (opttab[i].type)
		{
		case OT_NOYES:
		    if (saw_ask)
		    {
			printf("%s error: boolean option %s given with "
			    "ask prefix\n",source,beg+2);
			goto next;
		    }
		    /* drop through */
		case OT_NOYESASK:
		    if (saw_eq)
		    {
			printf("%s error: Illegal assignment of value "
			    "to boolean option %s\n", source, beg);
			goto next;
		    }
		    val= (saw_no ? 0 : (saw_ask ? 2 : 1));
		    if (!opttab[i].func || (*opttab[i].func)(val,source))
			*(int *)opttab[i].store= val;
		    break;

		case OT_INTEGER:
		case OT_STRING:
		    if (saw_no || saw_ask)
		    {
			printf("%s error: integer option %s given with "
			    "boolean prefix\n",source,beg+2);
			goto next;
		    }
		    if (!saw_eq ||
			(*arg == '\0' && opttab[i].type == OT_INTEGER))
		    {
			printf("%s error: No value given for option %s\n",
			    source,beg);
			goto next;
		    }
		    if (opttab[i].type == OT_INTEGER)
		    {
			if (*firstout(arg,"0123456789-+") != '\0')
			{
			    printf("%s error: Non-numeric value given for "
				"integer option %s\n",source,beg);
			    goto next;
			}
			val= atoi(arg);
			if (val < opttab[i].min)
			{
			    printf("%s warning: "
				"setting %s to minimum legal value of %d\n",
				source,beg,opttab[i].min);
			    val= opttab[i].min;
			}
			else if (val > opttab[i].max)
			{
			    printf("%s warning: "
				"setting %s to maximum legal value of %d\n",
				source,beg,opttab[i].max);
			    val= opttab[i].max;
			}
			if (!opttab[i].func || (*opttab[i].func)(val,source))
			    *(int *)opttab[i].store= val;
		    }
		    else
		    {
			val= strlen(arg);
			if (val < opttab[i].min)
			{
			    printf("%s error: "
				"%s must be at least %d character%s long.\n",
				source,beg,opttab[i].min,
				opttab[i].min == 1 ? "" : "s");
			    goto next;
			}
			else if (val > opttab[i].max)
			{
			    printf("%s error: "
				"%s may be at most %d character%s long.\n",
				source,beg, opttab[i].max,
				opttab[i].max == 1 ? "" : "s");
			    goto next;
			}
			if (!opttab[i].func || (*opttab[i].func)(arg,source))
			    strcpy((char *)opttab[i].store,arg);
		    }
		    break;
		}
		break;
	    }
	}

    next:;
    }
}


/* PRINT_OPTS - Print out current option settings */

void print_opts()
{
int i,col= 0;
char buf[BUFSIZE];

    for (i= 0; opttab[i].name != NULL; i++)
    {
	switch (opttab[i].type)
	{
	case OT_NOYESASK:
	    if (*(int *)opttab[i].store == 2)
	    {
		printf("ask%s ",opttab[i].name);
		break;
	    }
	case OT_NOYES:
	    sprintf(buf,"%s%s ", (*(int *)opttab[i].store)?"":"no",
		opttab[i].name);
	    break;
	case OT_INTEGER:
	    sprintf(buf,"%s=%d ", opttab[i].name, *(int *)opttab[i].store);
	    break;
	case OT_STRING:
	    if (*firstin(opttab[i].store," /t'\"") == '\0')
		sprintf(buf,"%s=%s ", opttab[i].name, (char *)opttab[i].store);
	    else if (strchr(opttab[i].store,'\042') == NULL)
		sprintf(buf,"%s=\042%s\042 ",
			opttab[i].name,(char *)opttab[i].store);
	    else
		sprintf(buf,"%s='%s' ",
			opttab[i].name,(char *)opttab[i].store);
	    break;
	}
	col+= strlen(buf);
	if (col >= mycols - 1)
	{
	    col= 0;
	    putchar('\n');
	}
	fputs(buf,stdout);
    }
    putchar('\n');
}


/* SET_PROMPT - do sanity checking and side effects for setting prompt */

int set_prompt(char *arg, char *source)
{
char *c;

    for (c= arg; *c != '\0'; c++)
	if (!isascii(*c) || !(isprint(*c) || *c == '\t'))
	{
	    printf("%s error: bad characters in prompt string\n",source);
	    return 0;
	}
    base_col= (strlen(arg) - outdent) % 8;
    dont_tab= cant_tab || (base_col != 0);
    return 1;
}


/* SET_OUTDENT - do side effects for setting outdent */

int set_outdent(int val, char *source)
{
    base_col= (strlen(prompt) - val) % 8;
    dont_tab= cant_tab || (base_col != 0);
    return 1;
}
