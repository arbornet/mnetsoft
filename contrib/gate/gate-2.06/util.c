#include "gate.h"


/* OPENFILE - Open the named file.
 *   exists and writable:    open; print to the end; return 0.
 *   exists and !writable:   fail with error messages; return 1.
 *   !exists and creatable:  create; return 0.
 *   !exists and !creatable: fail with error messages; return 1.
 *
 * If resume is true, we are re-opening, and we do not reprint the file
 * contents, nor do we create the file.
 */

int openfile(char *filename, int resume)
{
    int oumask;

    /* Try opening an existing file */
    if ((tfp= fopen(filename,"r+")) != NULL)
    {
	/* Move to end of file */
	if (resume)
	    fseek(tfp,0L,2);
	else
	    typetoend(outcol(prompt,BUFSIZE,0));

	if (secure) fchmod(fileno(tfp),0600);
    }
    else
    {
	/* Try creating the file */

	oumask= umask(secure ? 077 : 022);

	if (resume || errno != ENOENT || (tfp= fopen(filename,"w+")) == NULL)
	{
	    char buf[BUFSIZE];

	    sprintf(buf,"%s: cannot %sopen %s",
		    progname, resume?"re-":"", filename);
	    perror(buf);
	    umask(oumask);
	    return(1);
	}
	umask(oumask);
    }

    if (resume)
    {
	puts("(Continue your text entry)");
	lines_above= 0;
    }
    else
    {
	if (ftell(tfp) == 0L)
	{
	    puts("Enter your text:");
	    lines_above= 0;
	}
    }

    return(0);
}


/* TYPETOEND - Print contents of file from current position to end, indenting
 * each line one space.  This assumes we are currently at the start of a line,
 * so it prints a space first.  Pcol gives the number of blank columns to type
 * at the front of each line.
 */

void typetoend(int pcol)
{
    int och= '\n';
    int ch;
    int col= 0;	/* keeping track of columns only matters if dont_tab is on */

    while ((ch= fgetc(tfp)) != EOF)
    {
	if (och == '\n')
	{
	    for (col= 0; col < pcol; col++)
		putchar(' ');
	    lines_above++;
	}
	och= ch;
	if (dont_tab)
	{
	    if (ch == '\n' || ch == '\r')
		col= 0;
	    else if (ch == '\b')
		col--;
	    else if (ch == '\t')
	    {
		int fcol= tabcol(col);
		for ( ; col < fcol; col++)
		    putchar(' ');
		continue;
	    }
	    else
		col++;
	}
	putchar(ch);
    }
    clearerr(tfp);
    if (lines_above > mylines-1) lines_above= mylines-1;
}


/* READYES - Read in a yes or no answer with the given prompt.  Default to
 * "yes" on end-of-file.
 */

int readyes(char *prompt)
{
    return(askquest(prompt,"yn") == 'y');
}


/* ASKQUEST - Read a one letter answer from the given list after printing
 * the given prompt.  One end-of-file, the first possible answer is returned.
 */

int askquest(char *prompt, char *answers)
{
    int an,ch;

    fputs(prompt,stdout);
    for (;;)
    {
	for (;;)
	{
       	    if (isupper(an= getchar())) an= tolower(an);
	    if (an == EOF) return(answers[1]);
	    if (strchr(answers,an) != NULL)
		break;
	    putchar('\007');
	}
	putchar(an);
        while ((ch= getchar()) != '\n' && ch != ERASE_CHAR &&
	       ch != KILL_CHAR && ch != WERASE_CHAR && ch != EOF)
	    putchar('\007');
	if (ch == '\n' || ch == EOF)
	{
	    putchar('\n');
	    break;
	}
	putchar('\b'); putchar(' '); putchar('\b');
    }
    return(an);
}


/* ISPREFIX(a,b) - Tests if a is a marcus-style prefix of b.
 */

int isprefix(char *a, char *b)
{
    int sawmark= 0;

    for (;;)
    {
	if (*b == '_') {sawmark= 1; b++;}
	if (*a == '\0') return(sawmark);
	if (isupper(*a)) tolower(*a);
	if (*a++ != *b++) return(0);
    }
}


/* Firstin() returns a pointer to the first character in s that is in l.
 * \0 is always considered to be in the string l.
 *
 * Firstout() returns a pointer to the first character s that is not in l.
 * \0 is always considered to be not in the string l.
 *
 * Note that unlike strpbrk() these never return NULL.  They always return
 * a valid pointer into string s, if only a pointer to it's terminating
 * \0.  They are amazingly useful for simple tokenizing.
 */

#ifdef HAVE_STRSPN

char *firstin(char *s, char *l)
{
    return s+strcspn(s,l);
}

char *firstout(char *s, char *l)
{
    return s+strspn(s,l);
}

#else

char *firstin(char *s, char *l)
{
    for (;*s;s++)
	if (strchr(l,*s)) break;
    return(s);
}

char *firstout(char *s, char *l)
{
    for (;*s;s++)
	if (!strchr(l,*s)) break;
    return(s);
}
#endif /* !HAVE_STRSPN */


/* STRPCPY - Just like strcpy, but returns the pointer to the null at the
 * end of the copied string.
 */
char *strpcpy(char *s1, char *s2)
{
    while (*s2 != '\0')
	*(s1++)= *(s2++);
    *s1= '\0';
    return(s1);
}


/* DO_EDIT - Launch the user's editor on the file.
 */

void do_edit()
{
    char *ed;
    char bf[BUFSIZE];

    fclose(tfp);
    if ((ed= getenv("VISUAL")) == NULL &&
	(ed= getenv("EDITOR")) == NULL) ed= DEFAULT_EDITOR;
    sprintf(bf,"%s %s",ed,filename);
    usystem(bf,1,1,-1,-1);
    if (openfile(filename,1))
	done(RET_ABORT);
    puts(HELP_MESG);
    lines_above= 0;
}


/* NOVICE_ERROR - Check an entered text string to see if it contains some
 * common novice error.  If it does, print a helpful message.  We don't want
 * to over-do this.
 */

void novice_error(char *str)
{
    if (!strcmp(str,"help\n"))
	printf("[To get help, type a : at the start of the line, "
		"followed by the word \"help\"]\n");
    else if (!strcmp(str,"exit\n") || !strcmp(str,"quit\n") ||
	     !strcmp(str,"q\n"))
	printf("[To cancel your text entry, "
		"type :quit at the start of a line]\n");
    else
	return;

    lines_above= 0;
    return;
}
