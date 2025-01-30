/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */


#include "backtalk.h"
#include "spell.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if SPELLCHECK == 1


#ifndef ASPELL_LINK

/* ============== ASPELL OR ISPELL RUN AS CHILD PROCESS  ============== */

#include "sysdict.h"
#include "waittype.h"
#include "stack.h"
#include "str.h"
#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* These are state information used by func_spellcheck.  If we wanted to
 * support nesting 'spellcheck' loops inside each other, these would have to
 * be on the execution stack, but I can't imagine wanting to do that,
 * so we don't bother.
 */

static FILE *spellin= NULL;	/* Pipe to spell checker */
static FILE *spellout= NULL;	/* Pipe from spell checker */
static pid_t spellcheck_pid;	/* Process ID of spell checker */
static int spell_start= 0;	/* Index of start of the current line */
static int spell_end= 0;	/* Index of end of the current line */
static int spell_intag= 0;	/* Are we currently in a tag? */
static int spell_inquote= 0;	/* Are we currently in quotes? */

static int ispell_offset;


/* OPEN_SPELLCHECK - start an ispell subprocess, setting up the spellin
 * and spellout global variables.
 */

void open_spellcheck()
{
    int pipe_in[2];
    int pipe_out[2];
    char bf[BFSZ+1], *p, *e, *env[2], **en;
    int ch;
    extern char **environ;

    if (spellout != NULL) return;

    if (pipe(pipe_in) || pipe(pipe_out))
    	return;

    switch (spellcheck_pid= fork())
    {
    case 0: /* Child - do the exec */

	/* Close parent's ends of pipes */
	close(pipe_in[1]);
	close(pipe_out[0]);
	/* Attach pipe_in[0] to child's stdin */
	if (pipe_in[0] != 0)
	{
	    dup2(pipe_in[0],0);
	    close(pipe_in[0]);
	}
	/* Attach pipe_out[1] to child's stdout */
	if (pipe_out[1] != 1)
	{
	    dup2(pipe_out[1],1);
	    close(pipe_out[1]);
	}

	/* If we have an authenticated user, set the HOME environment variable
	 * to his backtalk directory.  This way we might be able to access his
	 * private dictionary.  Or maybe not.
	 */
	if (sdict(VAR_HOMEDIR)[0] != '\0')
	{
	    env[0]= (char *)malloc(strlen(sdict(VAR_HOMEDIR))+6);
	    strcpy(env[0],"HOME=");
	    strcpy(env[0]+5,sdict(VAR_HOMEDIR));
	    env[1]= NULL;
	    en= env;
	}
	else
	    en= environ;

#ifdef SPELL_PATH
	if (sdict(VAR_SPELL_LANG)[0] == '\0')
	    execle(SPELL_PATH, SPELL_PATH, "-a", "-S", (char *)NULL, en);
	else
	    execle(SPELL_PATH, SPELL_PATH, "-a", "-S", "-d",
	    	sdict(VAR_SPELL_LANG),(char *)NULL, en);
#endif
	exit(127);

    case -1: /* Parent - Fork failed */
	close(pipe_in[0]);
	close(pipe_in[1]);
	close(pipe_out[0]);
	close(pipe_out[1]);
    	return;

    default: /* Parent - Fork succeeded */

	/* Close child's ends of the pipes */
    	close(pipe_in[0]);
    	close(pipe_out[1]);

	/* Start buffered I/O on useful ends of pipes */
	spellin= fdopen(pipe_in[1],"w");
	spellout= fdopen(pipe_out[0],"r");

	/* Put ispell in terse mode */
	fputs("!\n",spellin);
	fflush(spellin);

	/* Read & Discard Ispell's banner line */
	if ((ch= getc(spellout)) == EOF || ch != '@' ||
	    fgets(bf, BFSZ, spellout) == NULL)
	    die("Could not read ispell's banner line");

	/* Send words to ignore */
	for (p= sdict(VAR_SPELL_IGNORE); *p != '\0'; p= firstout(e,", \n\t"))
	{
	    e= firstin(p,", \n\t");
	    putc('@', spellin);
	    fwrite(p,1,e-p,spellin);
	    putc('\n', spellin);
	}

	/* Some versions of international ispell include the ^ when they report
	 * the offset of an error in a line, some don't.  We figure it out by
	 * trying it at run-time.
	 */
        fputs("^xfgzvwpqq\n",spellin);	/* some reasonably long non-word */
	fflush(spellin);
	if (fgets(bf,BFSZ,spellout) == NULL || getc(spellout) != '\n')
	    die("Could not communicate with ispell");
	ispell_offset= -atoi(firstin(bf,"0123456789"));
    }
}


/* CLOSE_SPELLCHECK - Close a spellcheck child process.  This must be the
 * most recently opened one.
 */

void close_spellcheck()
{
    pid_t wpid;

    if (spellout == NULL) return;
    fclose(spellout);
    if (spellin != NULL) fclose(spellin);
    spellin= NULL;
    spellout= NULL;
    while ((wpid= wait((waittype *)NULL)) != -1 && wpid != spellcheck_pid)
    	;
}


/* SPELL_LINE - Given text, texttype, and an index, send the next line of
 * text to the spellchecker doing whatever filtering is necessary.  Returns
 * the index of the first character after the '\n' ending the line, or 0 if
 * there is no next line.
 */

int spell_line(char *text, int type, int begin)
{
    char ch;
    int i= begin;

    /* Quote each line with an initial ^ */
    putc('^',spellin);

    if (text[i] == '\0') return 0;

    for (;;)
    {
    	switch (ch= text[i]) {
	case '\n':
	    i++;
	case '\0':
	    putc('\n',spellin);
	    fflush(spellin);
	    return i;
	}
	i++;
	if (type == 1)
	{
	    if (spell_intag)
	    {
	    	if (spell_inquote)
		{
		   if (ch == spell_inquote)
		   	spell_inquote= 0;
		}
		else
		{
		    if (ch == '"' || ch == '\'')
		    	spell_inquote= ch;
		    else if (ch == '>')
		    	spell_intag= 0;
		}
		/* Convert insides of tags into blanks */
		ch= ' ';
	    }
	    else if (ch == '<')
	    	spell_intag= 1;
	    else if (ch == '&')
	    {
		/* Handle &nbsp; type tags (which are always on one line) */
		char *p= strchr(text+i,';');
	    	if (p != NULL && p < firstin(text+i," \n\r\t"))
		{
		    while (text+i <= p)
		    {
			i++;
		    	putc(' ',spellin);
		    }
		    ch= ' ';
		}
	    }
	}
	putc(ch,spellin);
    }
}


/* SPELLCHECK_STEP - Do the work for func_spellcheck.  We are given the text,
 * an index that points to the first character after the terminating newline
 * of the last line that we sent to 'ispell' , the mode, and the * texttype
 * (0=plain, 1=html).  Do the processing that has to happen before the
 * execution of the procedure.  If no mispellings are found, we return
 * non-zero.  If a match is found we
 *  - if mode is 2 or 3, push the list of guesses for the word
 *  - if mode is 1 or 3, push the section of the string before the match
 *  - push the misspelled word.
 *  - update the index.
 *  - return 0.
 */

int spellcheck_step(char *text, int *i, int mode, int type)
{
    char bf[BFSZ+1];
    char *p;
    int wordlen, offset;
    int guesses;

    /* Check if we are starting a new spellcheck */
    if (*i == 0)
    {
    	open_spellcheck();
	spell_start= 0;
	spell_intag= 0;
	spell_inquote= 0;
	/* Send first line to ispell */
	if ((spell_end= spell_line(text,type,spell_start)) == 0) return 1;
    }

    while (fgets(bf,BFSZ,spellout))
    	switch(bf[0])
	{
	case '\n':	/* Line finished */
	    /* Feed next line of text to spellchecker */
	    spell_start= spell_end;
	    if ((spell_end= spell_line(text,type,spell_start)) == 0) return 1;
	    break;
	case '*':	/* Correctly spelled words */
	case '+':
	case '-':
	    break;

	case '&':	/* Incorrectly spelled words */
	case '?':
	case '#':

	    /* parse out misspelled word */
	    *(p= firstin(bf+2 ," \t\n"))= '\0';
	    wordlen= p - (bf + 2);
	    p++;

	    /* Parse out number of guesses */
	    if (bf[0] != '#')
	    {
	    	guesses= atoi(p);
		p= firstin(p," \t\n") + 1;
	    }
	    else
	    	guesses= 0;

	    /* Parse out starting offset of word */
	    offset= spell_start + atoi(p) + ispell_offset;

	    /* Push an array of spelling suggestions */
	    if (mode & 2)
	    {
		/* Push a [ mark */
		push_array_mark();

		if (guesses > 0)
		{
		    char *e= firstin(p,":") + 1;
		    char *w;

		    /* Push each suggestion onto the stack */
		    while (*(p= firstout(e,", \t\n")) != '\0')
		    {
			e= firstin(p,", \t\n");
			push_string(strndup(p, e-p), FALSE);
		    }
		}
		
		/* Make the array */
		make_array(0);
	    }

	    /* Push correct block fragment */
	    if (mode & 1)
	    	push_string(strndup(text+(*i), offset-(*i)), FALSE);

	    /* Push misspelled word */
	    push_string(strndup(text+offset, wordlen), FALSE);

	    /* Update pointer to end of last correct string */
	    *i= offset + wordlen;

	    return 0;
	}
}


#else /* !ASPELL_LINK */

/* =================== ASPELL VIA C LANGUAGE API ============== */

#include "sysdict.h"
#include "stack.h"
#include "str.h"
#include <ctype.h>
#include <aspell.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* These are state information used by func_spellcheck.  If we wanted to
 * support nesting 'spellcheck' loops inside each other, these would have to
 * be on the execution stack, but I can't imagine wanting to do that,
 * so we don't bother.
 */


static AspellConfig *spell_config;	      /* Configuration */
static AspellManager *spell_checker= NULL;    /* Document descriptor */

/* These describe the current misspelled word */
static int guesses= 0;
char *guesslist= NULL;


/* INIT_SPELLCHECK - set up spell checking.
 */

void init_spellcheck()
{
    char *p, *e;

    AspellCanHaveError *possible_err;

    if (spell_checker != NULL) return;

    spell_config= new_aspell_config();

    /* Set language */
    if (sdict(VAR_SPELL_LANG)[0] != '\0')
	aspell_config_replace(spell_config, "lang", sdict(VAR_SPELL_LANG));

    spell_checker= NULL;
    if (aspell_error_number(possible_err) != 0)
	die("aspell error: %s",aspell_error_message(possible_err));
    else
	spell_checker= to_aspell_speller(possible_err); 

    /* Send words to ignore */
    for (p= sdict(VAR_SPELL_IGNORE); *p != '\0'; p= firstout(e,", \n\t"))
    {
	e= firstin(p,", \n\t");
	aspell_speller_add_to_session(spell_checker, p, e-p);
    }
}


/* CLOSE_SPELLCHECK - Close a spellcheck child process.  This must be the
 * most recently opened one.
 */

void close_spellcheck()
{
    delete_aspell_speller(spell_checker);
    spell_checker= NULL;
}


/* SPELLCHECK_STEP - Do the work for func_spellcheck.  We are given the text,
 * an index that points to the first character after the last word that
 * we checked, the mode, and the texttype (0=plain, 1=html).  Do the
 * processing that has to happen before the execution of the procedure.  If
 * no mispellings are found, we return non-zero.  If a match is found we
 *  - if mode is 1, push the section of the string before the match
 *  - push the misspelled word.
 *  - update the index.
 *  - return 0.
 */

int spellcheck_step(char *text, int *i, int mode, int type)
{
    int intag= 0, inquote= 0, inword= 0;
    int wordstart;
    int ch, oldch= 1;
    int j;

    /* Check if we are starting a new spellcheck */
    if (*i == 0) init_spellcheck();

    for (j= *i; j == 0 || text[j-1] != 0; j++)
    {
	if (spell_intag)
	{
	    if (spell_inquote)
	    {
	       if (text[j] == spell_inquote)
		    spell_inquote= 0;
	    }
	    else
	    {
		if (text[j] == '"' || text[j] == '\'')
		    spell_inquote= text[j];
		else if (text[j] == '>')
		    spell_intag= 0;
	    }
	}
	else if (isalpha(text[j]))
	{
	    if (!inword) {wordstart= j - 1; inword= 1;}
	    continue;	/* since we are still in a word */
	}
	else if (text[j] == '\'' || text[j] == '-')
	{
	    /* dashs or apostrophes are part of a word if they are preceded
	     * and followed by letters */
	    if (inword && isalpha(text[j-1]) && isalpha(text[j+1]))
		continue;  /* we are still in a word */
	}
	else if (text[j] == '<' && type == 1)
	    spell_intag= 1;
	else if (text[j] == '&' && type == 1)
	{
	    /* Handle &nbsp; type tags (which are always on one line) */
	    char *p= strchr(text+j,';');
	    if (p != NULL && p < firstin(text+j," \n\r\t"))
		j= p - text;
	}

	if (inword)
	{
	    /* If we got here, then we have just read the first non-word
	     * character after a word.  Check its spelling.
	     */
	    if (!aspell_speller_check(spell_checker,
				      text+wordstart, j-wordstart))
	    {
		/* Word is misspelled */

		/* Push an array of spelling suggestions */
		if (mode & 2)
		{
		    /* Get the suggestion list */
		    AspellWordList *suggs= aspell_speller_suggest(spell_checker,
			text+wordstart, j-wordstart);
		    AspellStringEnumeration *elem=
			aspell_word_list_elements(suggs); 
		    char *w;

		    /* Push a [ mark */
		    push_array_mark();

		    /* Push each suggestion onto the stack */
		    while ((w= aspell_string_enumeration_next(elem)) != NULL )
			push_string(w, TRUE);
		    
		    /* Make the array */
		    make_array(0);

		    delete_aspell_string_manag(elem); 
		}

	        /* Push correct block fragment */
	        if (mode & 1)
	    	    push_string(strndup(text + *i, wordstart - *i), FALSE);

	        /* Push misspelled word */
	        push_string(strndup(text+wordstart, j - wordstart), FALSE);

	        /* Update pointer to first character after bad word */
	        *i= j;

	        return 0;
	    }
	    else
		inword= 0;
	}
    }
    return 1;
}

#endif /* ASPELL_LINK */

#endif /* SPELLCHECK */
