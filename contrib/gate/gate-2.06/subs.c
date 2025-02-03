#include "gate.h"


/* DECODE_PAT - decode any backslashed escapes in the given string.  Since
 * the new pattern is always the same length or short, it can just be over-
 * written in the old buffer.
 */

void decode_pat(char *pattern)
{
    char *old,*new;

    for (old= new= pattern; *old != '\0'; old++,new++)
    {
	if (*old == '\\')
	{
	    switch(*(++old))
	    {
	    case 'n':  *new= '\n'; break;
	    case '/':  *new= '/';  break;
	    case '\\': *new= '\\'; break;
	    case 't':  *new= '\t'; break;
	    default:   old--; *new= *old; break;
	    }
	}
	else
	    *new= *old;
    }
    *new= '\0';
}


/* MAKE_COPY - Create and instantly unlink a temporary file containing an
 * exact copy of the current text buffer.  Return the still open file
 * descriptor.  The immediate unlink is to make sure it isn't left around if
 * our process gets killed.
 */

FILE *make_copy()
{
    char tmpname[18];
    FILE *fp;

    strcpy(tmpname, "/tmp/gateXXXXXX");
#if HAVE_MKSTEMP
    fp= fdopen(mkstemp(tmpname),"w+");
    unlink(tmpname);
    copy_fp(fp);
#else
    mktemp(tmpname);
    fp= copy_file(tmpname);
    unlink(tmpname);
#endif
    return fp;
}

char do_subs;


/* ASK_SPELL_SUBS - Ask for a replacement, also giving options to stop or
 * add the word to the dictionary.
 */

#ifdef UNIX_SPELL
int ask_spell_subs(char **replace, int *wcol,  char *match)
{
    int rc= 0;
    static char bf[BUFSIZE];

    while (do_subs == 'y' || do_subs == 'n')
    {
	char st[BUFSIZE+40];

	subtask= 1;
	sprintf(st,"Replacement for \"%.*s\" (? for help): ",
		strlen(match+1)-1, match+1);
	if (ggetline(*replace= bf,NULL,wcol,st))
	{
	    printf("\nSpell Check Interupted.\n");
	    do_subs= 'q';
	    rc= 1;
	    break;
	}
	subtask= 0;

	*firstin(bf,"\n")= '\0';
    
	if (bf[0] == '?' && bf[1] == '\0')
	{
	    spell_help(0);
	    *wcol= 0;
	    continue;
	}
	*wcol= strlen(bf);
	if (bf[0] == '\0')
	    do_subs= 'q';
	else if (bf[0] == '+' && bf[1] == '\0')
	{
	    int i;

	    for (i= 1; match[i+1] != '\0'; i++)
		st[i-1]= match[i];
	    st[i-1]= '\0';
	    addword(st);
	    do_subs= 'q';
	}
	else if ((bf[0] == '\\' || bf[0] == '>') && bf[1] == '\0')
	{
	    do_subs= 'q';
	    rc= 1;
	}
	else
	    do_subs= 'y';
	break;
    }
    return rc;
}
#endif


/* ASK_SUBS - Ask if he really wants to make the named substitution */

ask_subs()
{
    if (do_subs == 'y' || do_subs == 'n')
    {
	subtask= use_jump= 1;
	switch (setjmp(jenv))
	{
	case 0:
	case 2:
	    do_subs= askquest("Confirm substitution [ynaq]? ","qyna");
	    break;

	case 1:
	    printf("\nSubstitution Interupted.\n");
	    do_subs= 'q';
	    break;
	}
	subtask= use_jump= 0;
    }
}


/* MAKE_FAIL: Totally textbook KMP setup routine - the fail array should
 * always have at least two elements.
 */

void make_fail(char *pattern,int m,int *fail)
{
    int i,j,k;

    j= 1;
    k= 0;
    fail[1]= 0;
    while (j < m-1)
	if (pattern[j] == pattern[k])
	    fail[++j]= ++k;
	else if (k == 0)
	    fail[++j]= 0;
	else
	    k= fail[k];
}


/* SHOW_MATCH - Print what the line(s) would look like before or after the
 * substitution.  Print contents of tfp from offset newline to current offset,
 * followed by highlighted string "text", followed by contents of cfp through
 * next newline.  Both tfp and cfp are restored to original offsets before
 * returning.  If "spell" is 1, the first and last characters of the text
 * will not be highlighted.  "spell" should always be zero or one.
 */

void show_match(FILE *tfp, long newline, char *text, FILE *cfp, int spell)
{
    int i, tch, len;
    int hlcontext= 0;	/* should we highlight characters around text? */
    long oldt, oldc;
    int cl= 1;

    junk_above++;
    oldt= ftell(tfp);

    if (spell && text[0] == '\n')
    {
	/* Line begins inside text string */
	putchar(' ');	/* space at head of line */
    }
    else
    {
	fseek(tfp,newline,0);

	len= oldt-newline;

	if (hlcontext= (text[0] == '\0'))
	{
	    if (len > 0)
		len--;
	    else
		begin_standout();
	}

	putchar(' ');	/* space at head of line */

	for (i= 0; i < len; i++)
	{
	    putchar(tch= getc(tfp));
	    mcursor(&cl,&junk_above,tch);
	}

	if (spell)
	{
	    putchar(text[0]);
	    mcursor(&cl,&junk_above,text[0]);
	}
    }

    begin_standout();
    if (hlcontext && len > 0)
    {
	putchar(tch= getc(tfp));
	mcursor(&cl,&junk_above,tch);
    }

    fseek(tfp,oldt,0);  /* Need to seek between reading and writing */
    for (i= spell; text[i+spell] != '\0'; i++)
    {
	putchar(text[i]);
	mcursor(&cl,&junk_above,text[i]);
	if (text[i] == '\n')
	{
	    putchar(' ');	/* yes, highlight this */
	    cl= 1;
	}
    }

    oldc= ftell(cfp);
    if (spell) fseek(cfp,-1L,1);	/* Backup one character */
    tch= getc(cfp);
    if (hlcontext && tch != '\n' && tch != EOF)
    {
	putchar(tch);
	mcursor(&cl,&junk_above,text[i]);
	tch= getc(cfp);
    }
    end_standout();

    while (tch != '\n' && tch != EOF)
    {
	putchar(tch);
	mcursor(&cl,&junk_above,text[i]);
	tch= getc(cfp);
    }
    fseek(cfp,oldc,0);
}


/* DO_SUBSTITUTE - A crummy implementation using KMP algorithm.  If replace
 * is NULL, we are checking a spelling word instead.  Returns 1 if quit during
 * spell check and 2 if for any other reasons no further substitutes should be
 * tried.
 */

int do_substitute(char *pattern, char *replace)
{
    FILE *cfp;
    int *fail;		/* Fail array for KMP scan */
    int m;		/* length of pattern */
    long newline= 0L;	/* one plus offset of last \n written to text file */
    char *match;	/* Whatever matched pattern */
    int i,k,rc=0;
    int fch;
    int nmatch= 0, wcol= 0;
    int spell= (replace == NULL);

    do_subs= 'y';

    /* Make a copy of the file and rewind it */
    if ((cfp= make_copy()) == NULL) return 2;
    fflush(cfp);

    /* Decode patterns */
    decode_pat(pattern);
    if (!spell) decode_pat(replace);

    /* Set up fail array */
    m= strlen(pattern);
    fail= (int *)malloc((m+1)*sizeof(int));	/* oversize for m==1 case */
    match= (char *)malloc((m+1)*sizeof(char));

    make_fail(pattern,m,fail);

    /* Start search from beginning of file */
    fseek(cfp,0L,0);
    emptyfile();

    /* Nearly textbook KMP match routine - the text is read from the copy
     * file instead of an array.  As soon as they are known not to be part
     * of a match, characters are written out to the text file.
     */

    fch= getc(cfp);
    if (pattern[0] == CSEP)
    {
	match[0]= CSEP;
	k= 1;
    }
    else
	k= 0;

    for (;;)
    {
	if (fch == EOF)
	{
	    for (i= 0; i < k; i++)
		putc(match[i],tfp);
	    fclose(cfp);
	    if (nmatch == 0 && !spell)
		printf("Pattern not found.\n");
	    free(fail);
	    free(match);
	    return rc;
	}
	else if (fch == pattern[k] || (pattern[k] == CSEP && !isalpha(fch)))
	{
	    match[k]= fch;
	    if (++k == m)
	    {
		/* Match found - all characters before the match have been
		 * written out and last character of match is in fch variable
		 */

		nmatch++;

		if (do_subs == 'y' || do_subs == 'n')
		{
		    match[k]= '\0';

		    if (junk_above > 0)
		    {
			move_up(junk_above,0);
			erase_eos();
		    }
		    if (spell)
			show_match(tfp,newline,match,cfp,1);
		    else
			show_match(tfp,newline,replace,cfp,0);
		    putchar('\n');

		    junk_above++;
#ifdef UNIX_SPELL
		    if (spell)
			rc= ask_spell_subs(&replace,&wcol,match);
		    else
#endif
			ask_subs();
		}

		if (do_subs == 'y' || do_subs == 'a')
		{
		    /* Do the substitution */
		    if (spell) putc(match[0],tfp);
		    for (i= 0; replace[i] != '\0'; i++)
		    {
			putc(replace[i],tfp);
			if (replace[i] == '\n') newline= ftell(tfp);
		    }
		    k= 0;

		    /* Continue Search by advancing text pointer */
		    if (!spell) fch= getc(cfp);
		    continue;
		}
		else
		    /* Reject match - by pretending (fch != pattern[k]) */
		    k--;
	    }
	    else
	    {
		/* Match with non-terminal pattern character */
		fch= getc(cfp);
		continue;
	    }
	}

	if (k == 0)
	{
	    /* Mis-match with first pattern character */
	    putc(fch,tfp);
	    if (fch == '\n') newline= ftell(tfp);
	    fch= getc(cfp);
	}
	else
	{
	    /* Mis-match with later pattern character */
	    for(i= 0; i < k-fail[k]; i++)
	    {
		if (match[i] != CSEP) putc(match[i],tfp);
		if (match[i] == '\n') newline= ftell(tfp);
	    }
	    k= fail[k];
	}
    }
}


int interupted_clip= 0;
void clip_intr()
{
	interupted_clip= 1;
}

/* CLIPLAST - Delete the last line from the file.  Load it into the
 * buffer given, making sure it isn't longer than the length. If
 * it is longer return 2.  If the file is empty, return 1.  If all
 * is fine, return 0.
 */

int cliplast(char *lastbuf, int bufsize)
{
    FILE *cfp;
    int i, ch, linedone, empty;

    interupted_clip= 0;
#ifdef HAVE_SIGACTION
    sigact(SIGINT, clip_intr, NULL);
#else
    signal(SIGINT,(void (*)())clip_intr);
#endif

    /* Make a copy of the file */
    if ((cfp= make_copy()) == NULL) return 0;
    fflush(cfp);
    fseek(cfp,0L,0);

    /* Empty our file */
    emptyfile();

    i= 0;
    empty= 1;
    linedone= 0;
    bufsize--;
    for (;;)
    {
	if ((ch= getc(cfp)) == EOF)
	{
	    fclose(cfp);
#ifdef HAVE_SIGACTION
	    sigact(SIGINT, intr, NULL);
#else
	    signal(SIGINT,(void (*)())intr);
#endif
	    if (interupted_clip) intr();
	    if (empty) return 1;
	    if (i >= bufsize)
	    {
		if (linedone) putc('\n',tfp);
		return 2;
	    }
	    if (!linedone) lastbuf[i]= '\0';
	    return 0;
	}
	else if (linedone)
	{
	    if (i < bufsize) fputs(lastbuf,tfp);
	    putc('\n',tfp);
	    linedone= 0;
	    i= 0;
	}

	empty= 0;

	if (ch == '\n')
	{
	    if (i <= bufsize) lastbuf[i]= '\0';
	    linedone= 1;
	}
	else if (i == bufsize)
	{
	    lastbuf[i++]= '\0';
	    fputs(lastbuf,tfp);
	    putc(ch,tfp);
	}
	else if (i > bufsize)
	    putc(ch,tfp);
	else
	    lastbuf[i++]= ch;
    }

    return 0;
}
