#include <stdio.h>
#include <ctype.h>

#include "common.h"
#include "str.h"
#include "qry.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

struct fun {
    char *name;		/* function name */
    int get;		/* True if this function returns values */
    char *args;		/* array of arg types:  'C' = char, 'I' = int */
    char *cond;		/* #if to enclose function */
    char *def;		/* define this symbol inside #if */
    } func[] = {

    {"auth_create",	   0, "",	"defined(SQL_CREATE)", "CREATE_AUTH"},
    {"auth_getpass",	   1, "C",	"defined(HAVEAUTH_GETPASS)",   NULL},
    {"auth_logins",	   1, "I",	"defined(HAVEAUTH_WALK)",      NULL},
    {"auth_logins_after",  1, "CI",	"defined(HAVEAUTH_SEEK)",      NULL},
    {"auth_newpass",	   0, "CC",	"defined(HAVEAUTH_NEWPASS)",   NULL},
    {"auth_add",	   0, "CCIICC", "defined(HAVEAUTH_ADD)",       NULL},
    {"auth_del",	   0, "C",	"defined(HAVEAUTH_DEL)",       NULL},

    {"ident_create",	   0, "",	"defined(SQL_CREATE)", "CREATE_IDENT"},
    {"ident_getpwnam",	   1, "C",	 NULL,			       NULL},
    {"ident_getpwuid",	   1, "I",	"defined(HAVEIDENT_GETPWUID)", NULL},
    {"ident_users",	   1, "I",	"defined(HAVEIDENT_WALKPW)",   NULL},
    {"ident_logins",	   1, "I",	"defined(HAVEIDENT_WALK)",     NULL},
    {"ident_logins_after", 1, "CI",	"defined(HAVEIDENT_SEEK)",     NULL},
    {"ident_newfname",	   0, "CC",	"defined(HAVEIDENT_NEWFNAME)", NULL},
    {"ident_newgid",	   0, "CI",	"defined(HAVEIDENT_NEWGID)",   NULL},
    {"ident_add",	   0, "CIICC",  "defined(HAVEIDENT_ADD)",      NULL},
    {"ident_del",	   0, "C",	"defined(HAVEIDENT_DEL)",      NULL},

    {"uid_create",	   0, "",	"defined(SQL_CREATE)",   "CREATE_UID"},
    {"uid_next",	   1, "",	NULL,			       NULL},
    {"uid_curr",	   1, "",	NULL,			       NULL},

    {"group_create",	   0, "",	"defined(SQL_CREATE)",  "CREATE_GROUP"},
    {"group_gid_to_name",  1, "I",	NULL,			       NULL},
    {"group_name_to_gid",  1, "C",	NULL,			       NULL},
    {"group_in_name",	   1, "CC",	NULL,			       NULL},
    {"group_in_gid",	   1, "CI",	NULL,			       NULL},
    {"group_all",	   1, "I",	NULL,			       NULL},
    {"group_mine",	   1, "CI",	NULL,			       NULL},
    {"group_add",	   0, "IC",	NULL,			       NULL},
    {"group_memadd",	   0, "IC",	NULL,			       NULL},
    {"group_del",	   0, "C",	NULL,			       NULL},
    {"group_memdel",	   0, "CC",	NULL,			       NULL},

    {"sess_create",	   0, "",	"defined(SQL_CREATE)",  "CREATE_SESS"},
    {"sess_get",	   1, "C",	NULL,			       NULL},
    {"sess_new",	   0, "CCIC",	NULL,			       NULL},
    {"sess_refresh",	   0, "CI",	NULL,			       NULL},
    {"sess_kill",	   0, "C",	NULL,			       NULL},
    {"sess_reap",	   0, "I",	NULL,			       NULL},

    {"baai_create",	   0, "",	"defined(SQL_CREATE)",  "CREATE_BAAI"},
    {"baai_get",	   1, "C",	NULL,			       NULL},
    {"baai_list",	   1, "",	NULL,			       NULL},
    {"baai_add",	   0, "CCICICIICI",NULL,		       NULL},
    {"baai_edit",	   0, "CCICICIICI",NULL,		       NULL},
    {"baai_link",	   0, "C",	NULL,			       NULL},
    {"baai_del",	   0, "C",	NULL,			       NULL},

    {NULL, 0, NULL, NULL, NULL}};

#define MAX_NAME 50

int line;
char *fn;
char *src= NULL;
int ungot= EOF;
char *progname;
FILE *fp;

struct mac {
    char *name;		/* macro name */
    char *value;	/* macro value */
    struct mac *next;	/* link */
} *macros= NULL;


/* Read one character from the current input source.  Return EOF on end of
 * file.
 */

int getch()
{
    if (ungot != EOF)
    {
    	int ch= ungot;
	ungot= EOF;
	return ch;
    }

    if (src != NULL)
    {
        if (*src == '\0')
	{
	    free(src);
	    src= NULL;
	}
	else
	    return *(src++);
    }
    return getc(fp);
}


/* Return one character to the input stream for rereading.  Can only have one
 * ungot character at a time.  It doesn't have to be the same one you read.
 */

void ungetch(char ch)
{
    if (ungot != EOF)
    {
    	fprintf(stderr,"%s: Ungot more than one character!", progname);
	exit(1);
    }
    ungot= ch;
}


/* Discard characters until we find a newline */
void skip_line()
{
    int ch;

    while ((ch= getch()) != '\n' && ch != EOF)
    	;
}


/* Discard all whitespace characters up to the next non-white character.
 * Also skips comments.  If same_line is true, don't skip newline characters.
 */
void skip_white(int same_line)
{
    int ch;

    while ((ch= getch()) != EOF)
    {
    	if (!same_line && ch == '\n')
	    line++;
	else if (ch == '#')
	{
	    skip_line();
	    line++;
	}
	else if (ch != ' ' && ch != '\t' && ch != '\r')
	{
	    ungetch(ch);
	    return;
	}
    }
}


/* If the next character to read is 'ch' read it and return true.  Otherwise
 * don't read it and return false.
 */
int next_char(char goal_ch)
{
    int ch= getch();
    if (ch == goal_ch) return 1;
    if (ch != EOF) ungetch(ch);
    return 0;
}


/* Same as next_char(), except we skip white space first.
 */

int next_nonwhite(char goal_ch)
{
    skip_white(0);
    return next_char(goal_ch);
}


/* Read in a specific character, possibly preceeded by spaces or comments.
 */
void read_char(char goal_ch)
{
    int ch;
    skip_white(0);
    if ((ch= getch()) != goal_ch)
    {
    	fprintf(stderr,"%s: %s Line %d: expected '%c' but found '%c'\n",
	    progname, fn != NULL ? fn : "stdin", line, goal_ch, ch);
	exit(1);
    }
}


/* Read in a name, ungetting the terminating character and saving the name in
 * the given buffer.  If the line is longer than the maximum length, read all
 * of it but save only what fits.
 */

void read_name(char *bf)
{
    int i= 0;
    int storing= 1;
    int ch;

    while ((ch= getch()) != EOF)
    {
    	if ((ch >= 'a' && ch <= 'z') || ch == '_' ||
    	    (ch >= 'A' && ch <= 'Z') ||
    	    (i > 0 && ch >= '0' && ch <= '9'))
	{
	    if (storing)
	    {
		bf[i++]= ch;
		if (i >= MAX_NAME) storing= 0;
	    }
	}
	else
	{
	    ungetch(ch);
	    break;
	}
    }
    if (i == 0)
    {
    	fprintf(stderr,"%s: %s Line %d: bad name\n",
	     fn != NULL ? fn : "stdin", progname,line);
	exit(1);
    }
    bf[i]= '\0';
}


/* Given a name, return the value of that macro by that name.  Return NULL
 * if not defined.
 */
char *get_macro(char *name)
{
    struct mac *m;

    for (m= macros; m != NULL; m= m->next)
    {
    	if (!strcmp(name, m->name))
	    return m->value;
    }
    return NULL;
}


/* Given a malloced buffer of size *sz, containing *n characters, append the
 * given string, enlarging it if necesary.  n and sz and bf are updated, if
 * necessary.
 */
void addstr(char **bf, int *n, int *sz, char *add)
{
    int l= strlen(add);

    if (*n + l >= *sz)
    {
    	*sz= *n + l + MAX_NAME;
	*bf= (char *)realloc(*bf, *sz);
    }
    strcpy(*bf+*n,add);
    *n+= l;
}


/* Given a malloced buffer of size *sz, containing *n characters, append the
 * given character, enlarging the buffer if necesary.  n and sz and bf are
 * updated, if necessary.
 */
void addch(char **bf, int *n, int *sz, char add)
{
    if (*n + 1 >= *sz)
    {
    	*sz= *n + MAX_NAME;
	*bf= (char *)realloc(*bf, *sz);
    }
    (*bf)[(*n)++]= add;
}


/* Read in a macro definition (initial & has already been read) and save it.
 */
void read_macro()
{
    char name[MAX_NAME+1];
    struct mac *m;
    int ch;
    int n= 0, s= MAX_NAME;

    read_name(name);
    read_char('=');
    skip_white(1);

    /* Make the structure */
    m= (struct mac *)malloc(sizeof(struct mac));
    m->name= strdup(name);
    m->value= (char *)malloc(s);

    /* Read in rest of line as the value */
    while ((ch= getch()) != EOF && ch != '\n')
    {
    	if (ch == '&' && !next_char('&'))
	{
	    char cn[MAX_NAME+1];
	    char *cv;
	    int cl,ch2;

	    read_name(cn);
	    cv= get_macro(cn);
	    if (cv == NULL)
	    {
		fprintf(stderr,"%s: %s Line %d: undefined macro &%s\n",
		    progname, fn != NULL ? fn : "stdin", line, cn);
		exit(1);
	    }
	    addstr(&m->value,&n,&s,cv);
	}
	else
	    addch(&m->value,&n,&s,ch);
    }
    m->value[n]= '\0';
    line++;

    /* Link it into linked list */
    m->next= macros;
    macros= m;
}


/* Read in a function definition, and write the C version out to stdout.
 */
void read_func()
{
    char name[MAX_NAME+1];
    char *arg[50];
    struct fun *f;
    char ch;
    int i, n, l, was_white, depth, cmd_no, ignore_error;
    int sz_fmt= MAX_NAME, sz_par= MAX_NAME;
    char *fmt= (char *)malloc(sz_fmt);
    char *par= (char *)malloc(sz_par);
    int n_fmt, n_par;

    /* Read function name and convert to lower case */
    read_name(name);
    for (i= 0; name[i] != '\0'; i++)
    	if (isupper(name[i])) name[i]= tolower(name[i]);

    /* Look up the name in our table of functions */
    for (f= func; f->name != NULL && strcmp(f->name,name); f++)
    	;
    if (f->name == NULL)
    {
    	fprintf(stderr,"%s: %s Line %d: unknown command %s\n", progname,
	    fn != NULL ? fn : "stdin", line, name);
	exit(1);
    }

    /* Print start of function definition */
    if (f->cond) printf("#if %s\n", f->cond);
    if (f->def) printf("#define %s\n",f->def);
    if (f->get)
	printf("SQLhandle qry_%s(", f->name);
    else
    	printf("void qry_%s(", f->name);

    /* Read argument names and print them out */
    read_char('(');
    skip_white(0);
    n= 0;
    l= strlen(f->args);
    while ((ch= getch()) != ')')
    {
	if (n >= l)
	{
	    fprintf(stderr,"%s: %s Line %d: expected only %d arguments\n",
	    	progname, fn != NULL ? fn : "stdin", line, l);
	    exit(1);
	}
    	if (ch != '$')
	{
	    fprintf(stderr,
	    	"%s: %s Line %d: expected argument starting with $\n",
	    	progname, fn != NULL ? fn : "stdin", line);
	    exit(1);
	}
	read_name(name);
	printf("%s%s", (f->args[n] == 'C') ? "char *" : "int ", name);
	arg[n++]= strdup(name);
	if (n < l) {
	    read_char(',');
	    printf(", ");
	}
	skip_white(0);
    }
    if (n != l)
    {
	fprintf(stderr,"%s: %s Line %d: Got %d arguments instead of %d\n",
	    progname, fn != NULL ? fn : "stdin", line, n, l);
	exit(1);
    } 
    if (!next_nonwhite('{'))
    {
	fprintf(stderr,"%s: %s Line %d: Expected { after function arguments\n",
	    progname, fn != NULL ? fn : "stdin", line);
	exit(1);
    }
    printf(")\n{\n");

    cmd_no= 0;
    while (!next_nonwhite('}'))
    {
	/* Prepare to process an sql command */
	n_fmt= 0;  n_par= 0;
	depth= 0;

	if (cmd_no++ == 0)
	{
	    printf("    struct sqlbuf b;\n    ");
	    if (f->get) printf("SQLhandle s;\n    ");
	    printf("makeqry(&b);\n    ");
	}
	else
	    printf("sql_do(finalqry(&b),%d);\n    resetqry(&b);\n    ",
	    	ignore_error);
		

	ignore_error= 0;

	while ((ch= getch()) != ';')
	{
	    if (ch == '&' && !next_char('&'))
	    {
		char cn[MAX_NAME+1];
		char *cv;
		int cl,ch2;

		read_name(cn);
		cv= get_macro(cn);
		if (cv == NULL)
		{
		    fprintf(stderr,"%s: %s Line %d: undefined macro &%s\n",
		    	progname, fn != NULL ? fn : "stdin", line, cn);
		    exit(1);
		}
		addstr(&fmt,&n_fmt,&sz_fmt,cv);
		was_white= 0;
	    }
	    else if (ch == '$' && !next_char('$'))
	    {
		int flag,i;
		char cn[MAX_NAME+1];

		flag= getch();
		if (flag != '\'' && flag != '[')
		{
		    ungetch(flag);
		    flag= 0;
		}

	        read_name(cn);
		for (i= 0; i < n; i++)
		    if (!strcmp(cn,arg[i])) break;
		if (i >= n)
		{
		    fprintf(stderr,"%s: %s Line %d: undefined parameter $%s\n",
		    	progname, fn != NULL ? fn : "stdin", line, cn);
		    exit(1);
		}

		if (flag == '[')
		{
		    fmt[n_fmt]= '\0'; par[n_par]= '\0';
		    printf("addqry(&b,\"%s\"%s);\n    ", fmt, par);
		    n_fmt= 0;  n_par= 0;
		    if (f->args[i] == 'C')
			printf("if (%s != NULL) ", cn);
		    else
			printf("if (%s > 0) ", cn);
		    depth++;
		}
		else if (flag == '\'' && f->args[i] == 'C')
		{
		    fmt[n_fmt]= '\0'; par[n_par]= '\0';
		    printf("addqry(&b,\"%s\"%s);\n    ", fmt, par);
		    printf("addquoteqry(&b,%s);\n    ", cn);
		    n_fmt= 0;  n_par= 0;
		}
		else
		{
		    /* Add %s or %d to format string */
		    addstr(&fmt,&n_fmt,&sz_fmt,
			(f->args[i] == 'C') ? "%s" : "%d");

		    /* Add variable name to parameter string */
		    addstr(&par,&n_par,&sz_par,", ");
		    addstr(&par,&n_par,&sz_par,cn);
		}
		was_white= 0;
	    }
	    else if (ch == ']' && depth > 0)
	    {
		fmt[n_fmt]= '\0'; par[n_par]= '\0';
		printf("addqry(&b,\"%s\"%s);\n    ", fmt, par);
		n_fmt= 0;  n_par= 0;
		depth--;
	    }
	    else if (ch == '-' && n_fmt == 0)
	    {
	    	ignore_error= 1;
		was_white= 1;
	    }
	    else
	    {
		/* Ordinary characters */
		int is_white= (ch==' ' || ch=='\n' || ch=='\t' || ch=='\r');
		if (ch == '\n') line++;

		if (is_white)
		{
		    if (!was_white) addch(&fmt,&n_fmt,&sz_fmt,' ');
		}
		else
		{
		    if (ch == '%')
			addch(&fmt,&n_fmt,&sz_fmt,'%');
		    else if (ch == '"' || ch == '\\')
			addch(&fmt,&n_fmt,&sz_fmt,'\\');
		    addch(&fmt,&n_fmt,&sz_fmt,ch);
		}

		was_white= is_white;
	    }
	}

	/* At end of SQL command (;) */
	if (n_fmt > 0)
	{
	    fmt[n_fmt]= '\0'; par[n_par]= '\0';
	    printf("addqry(&b,\"%s\"%s);\n    ", fmt, par);
	}
    }

    if (cmd_no > 0)
    {
	/* At end of function */
	if (f->get)
	{
	    printf("s= sql_get(finalqry(&b));\n    ");
	    printf("freeqry(&b);\n    ");
	    printf("return s;\n");
	}
	else
	{
	    printf("sql_do(finalqry(&b),%d);\n    ",ignore_error);
	    printf("freeqry(&b);\n");
	}
    }
    printf("}\n");

    if (f->cond) printf("#endif\n");
    printf("\n");
}


main(int argc, char **argv)
{
    char ch;
    int i,n;

    progname= argv[0];

    printf("/* DO NOT EDIT THIS FILE!!! */\n\n");
    printf("/* This file was automatically generated from %stemplate\n",
    	(argc > 1) ? "the following " : "");
    printf(" * files by mksql and will be overwritten on the next build.\n");
    for (i= 1; i < argc; i++)
    	printf(" *  %s\n",argv[i]);
    printf(" */\n\n");

    printf("#include \"common.h\"\n");
    printf("#include \"authident.h\"\n");
    printf("#include \"sql.h\"\n");
    printf("#include \"qry.h\"\n\n");

    n= (argc > 1) ? argc : 2;

    for (i= 1; i < n; i++)
    {
	/* Close previous file */
	if (i > 1) fclose(fp);

	if (i >= argc)
	{
	    /* If no files given, read from stdin */
	    fn= NULL;
	    fp= stdin;
	}
	else
	{
	    /* Open new file */
	    fn= argv[i];
	    if ((fp= fopen(fn,"r")) == NULL)
	    {
		fprintf(stderr,"%s: Cannot open file %s\n", progname, fn);
		exit(1);
	    }
	}
	line= 1;

	/* Read through file, ignore comments, do macro and function defs */
	while ((ch= getch()) != EOF)
	{
	    switch (ch)
	    {
	    case '\n':
		line++;
	    case ' ': case '\t': case '\r':
		/* Ignore white space */
		break;

	    case '#':
		/* Ignore comments */
		skip_line();
		line++;
		break;

	    case '&':
		/* Macro definition */
		read_macro();
		break;

	    default:
		/* Function definition */
		ungetch(ch);
		read_func();
	    }
	}
    }
    exit(0);
}
