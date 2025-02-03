#include "gate.h"

#ifndef NO_SPELL

#include <pwd.h>
#include <sys/file.h>

#ifdef ISPELL
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#endif

#if defined(IISPELL) || defined(OLDASPELL)

#if HAVE_DIRENT_H
#include <dirent.h>
typedef struct dirent dirent;
#define dnamlen(d) strlen(d->d_name)
#else
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
typedef struct direct dirent;
#define dnamlen(d) (d->d_namlen)
#endif

#endif /* IISPELL || OLDASPELL */


/* We support four different kinds of Unix spell checkers.  Define one of the
 * following options:
 *
 * SIMPLE_SPELL:  (eg: NeXT)
 *    check spelling with:                  spell [-b] [-d hlist]
 *    write new words through:              spellin oldhlist > newhlist
 *
 *    User dictionaries are rather large.  We print warning messages before
 *    creating them.
 *
 * PLUS_SPELL:  (eg: SunOS)
 *    check spelling with:                  spell [-b] [+mywords] [-d hlist]
 *    merge new words into sorted file mywords.
 *
 *    The + option lets us do much more compact user dictionaries.  Otherwise
 *    this is the same as simple spell.
 *
 * IISPELL: (International ispell, version 3.1)
 *    This uses Ispell's weird programmatic interface, and works completely
 *    differently from the above.  It ought to be faster, and it processes the
 *    misspelled words in context order instead of jumping around to do them in
 *    alphabetical order.  It is recommended that ispell be installed and used
 *    in preference to those other things.
 *
 *    Note that this doesn't work for ispell 4.0, which is *not* a newer
 *    version of ispell 3.1, but an entirely different program that happens
 *    to have the same name.
 *
 * GISPELL: (ispell, version 4.0)
 *    This is *not* a successor to version 3.1.  It's a sibling.  In most ways
 *    that we care about, it is actually more primitive.  It's the only one
 *    of the spell checkers that has no language options at all.
 *
 * OLDASPELL: (Gnu Aspell, version less than 0.50)
 *    This is a replacement for ispell.  It emulates ispell, and we use it
 *    the same way.  However listing dictionaries has to be a bit different.
 *
 * ASPELL: (Gnu Aspell, versions 0.50 or above)
 *    Later versions of aspell have a better way to list dictionaries.
 *
 * NO_SPELL:
 *    Gate can also be compiled with no spell checking.
 */

#ifndef GISPELL
char language[LANGSIZE+1]= LANG_DEFAULT;
#endif

void spell_help(int guesses)
{
    printf("   Type a replacement for the misspelled word or:\n");
    if (guesses > 0)
    {
	printf("   <number>  to replace the word with guess number nn\n");
	printf("   #         to reprint the list of guesses\n");
    }
    printf("   <return>  to ignore all occurrences\n");
    printf("   +         to add this word to your dictionary\n");
    printf("   \\         to quit spell checking.\n");
    junk_above= 0;
}


#ifdef ISPELL

	/*************** GNU ISPELL INTERFACE *****************/

#ifndef SPELL_PATH
#if defined(OLDASPELL) || defined(ASPELL)
#define SPELL_PATH "aspell"
#else
#define SPELL_PATH "ispell"
#endif
#endif

#define MAX_GUESS 80

FILE *to, *from;	/* Pipes to and from ispell process */
pid_t spell_pid;	/* Process id of ispell process */
int found_some;		/* Did we find errors? */

#ifndef GISPELL
int initial_offset= 1;
#endif


/* We want to be able to remember previous replacements, so we can repeat
 * them if the same misspelled word occurs again.  This also filters out any
 * "ignored" words that line-by-line versions of ispell may not have got
 * the message about yet.
 */

struct repl {
	char *old;		/* Word to replace */
	char *new;		/* Replacement, or NULL if we should ignore */
	struct repl *next;
	} *rephead= NULL;


/* EMPTY_REPL - destroy the replacement list.
 */

empty_repl()
{
    struct repl *r, *n;

    for (r= rephead; r != NULL; r= n)
    {
	n= r->next;
	if (r->old != NULL) free(r->old);
	if (r->new != NULL) free(r->new);
	free(r);
    }
    rephead= NULL;
}


/* ADD_REPL - Add the given replacement to the list of replacements.
 * "replace" may be NULL to indicate that the word is to be ignored.
 */

void
add_repl(char *word, int word_len, char *replace)
{
    struct repl *r;

    for (r= rephead; r != NULL; r= r->next)
    {
	if (!strncmp(word,r->old,word_len) && r->old[word_len] == '\0')
	{
	    if (r->new == NULL && replace == NULL)
		return;

	    if (r->new != NULL && replace != NULL && !strcmp(replace,r->new))
		return;

	    if (r->new != NULL) free(r->new);

	    break;
	}
    }

    if (r == NULL)
    {
	r= (struct repl *)malloc(sizeof(struct repl));
	r->old= (char *)malloc(word_len+1);
	strncpy(r->old, word,word_len);
	r->old[word_len]= '\0';
	r->next= rephead;
	rephead= r;
    }

    if (replace == NULL)
	r->new= NULL;
    else
    {
	r->new= (char *)malloc(strlen(replace)+1);
	strcpy(r->new, replace);
    }
}


/* PREV_REPL - Find the previous replacement used for word and return a
 * pointer to it.  Returns the length of the replacement if one is on
 * file; 0 if no replacement is on file; and -1 if the word should be
 * ignored.
 */

int prev_repl(char *word, int word_len, char *replace)
{
    struct repl *r;

    for (r= rephead; r != NULL; r= r->next)
    {
	if (!strncmp(word,r->old,word_len) && r->old[word_len] == '\0')
	{
	    if (r->new == NULL) return -1;

	    strcpy(replace,r->new);
	    return strlen(r->new);
	}
    }
    return 0;
}



/* SHOW_GUESSES - Print out the guesses stored in guesslist in nice neat
 * columns.
 */

void show_guesses(char **guesslist, int guesses)
{
    int cls,lns;
    int len,longest,i,j;

    longest= strlen(guesslist[0]);
    for (i= 1; i < guesses; i++)
    {
	len= strlen(guesslist[i]);
	if (len > longest) longest= len;
    }
	
    cls= mycols/(longest+7);
    lns= (guesses + cls - 1) / cls;

    for (i= 0; i < lns; i++)
    {
	printf("%2d: %-*.*s", i+1, longest, longest, guesslist[i]);
	for (j= lns; j+i < guesses; j+= lns)
	    printf("   %2d: %-*.*s", i+j+1, longest, longest, guesslist[i+j]);
	putchar('\n');
	junk_above++;
    }
}


/* SHOW_LINE - print the current line with the misspelled word highlighted.
 */

void show_line(char *line, int offset, int word_len)
{
    int cl= 1;
    int i;

    putchar(' ');		/* space at head of line */

    for (i= 0; i < offset; i++)
    {
	putchar(line[i]);
	mcursor(&cl,&junk_above,line[i]);
    }

    begin_standout();

    for ( ; i < offset+word_len; i++)
    {
	putchar(line[i]);
	mcursor(&cl,&junk_above,line[i]);
    }

    end_standout();

    for( ; line[i] != '\0'; i++)
    {
	putchar(line[i]);
	mcursor(&cl,&junk_above,line[i]);
    }
}


/* DO_REPLACE - fix up the line buffer to reflect changing word to replace
 * at offset.  This may change the length of the line, which means future
 * replaces need to have their offsets adjusted.  Adjust oldlen to newlen
 * after the call.
 */

void do_replace(char *line, int offset, int *oldlen, char *replace)
{
    int newlen= strlen(replace);
    int delta= newlen - *oldlen;
    int i;

    add_repl(line+offset,*oldlen,replace);

    /* Do the replacement in the line */

    if (delta > 0)
    {
	for (i= strlen(line); i >= offset+*oldlen; i--)
	    line[i+delta]= line[i];
    }
    else if (delta < 0)
    {
	for (i= offset+*oldlen; line[i] != '\0'; i++)
	    line[i+delta]= line[i];
	line[i+delta]= '\0';
    }
    strncpy(line+offset,replace,newlen);

    *oldlen= newlen;
}


#if defined(IISPELL) || defined(OLDASPELL) || defined(ASPELL)

#ifdef IISPELL
#define DICT_SUFFIX "hash"
#define DICT_SUFFIX_LEN 4
#else
#define DICT_SUFFIX "dat"
#define DICT_SUFFIX_LEN 3
#endif

#ifdef ASPELL
/* We keep a linked list of languages around with modern aspell */
struct langl {char *lang; struct langl *next;} *langlist= NULL;
#endif


/* SET_LANGUAGE - Check if we have a dictionary for the given language.  If so,
 * return 1.  If not, list legal languages and return 0.
 */

int set_language(char *str, char *source)
{
#ifdef ASPELL
    struct langl *l,*ll;
    char buf[BUFSIZE+1];
    int n= 0;
    int fd_from;
    pid_t pid,child;
    FILE *fp_from;

    if (langlist == NULL)
    {
	/* Get the language list */
	sprintf(buf,"%.200s dump dicts", SPELL_PATH);

	if (child= cmd_pipe(buf,1, NULL, &fd_from) < 0)
	{
	    printf("Error: could not execute %s\n",SPELL_PATH);
	    return 1;
	}
	fp_from= fdopen(fd_from,"r");
	while (fgets(buf,BUFSIZE, fp_from))
	{
	    l= (struct langl *)malloc(sizeof(struct langl));
	    *firstin(buf,"\n")= '\0';
	    l->lang= strdup(buf);
	    l->next= NULL;
	    if (langlist == NULL)
		langlist= l;
	    else
		ll->next= l;
	    ll= l;
	}

	/* Close the pipe and wait for child to terminate */
	fclose(fp_from);
	while (((pid=wait((int *)NULL)) != -1 || errno == EINTR) && pid != child )
	    ;
    }

    /* Check if the selected language is in it */
    for (l= langlist; l != NULL; l= l->next)
	if (!strcmp(l->lang, str)) return 1;

    if (*source != ':' || langlist == NULL)
	return 0;

    /* List the alternatives */
    printf("Dictionaries available are:\n");
    for (l= langlist; l != NULL; l= l->next)
	printf("%s\n", l->lang);
    return 0;
#else
#ifdef DICT_DIR
    char buf[BUFSIZE+1];
    DIR *ispd;
    dirent *de;
    int n= 0;
    int namlen;

    sprintf(buf,"%s/%s."DICT_SUFFIX, DICT_DIR, str);
    if (!access(buf, 4)) return 1;

    printf("%s error: No dictionary available for language '%s'.\n",
	source, str);

    if (*source != ':' || (ispd= opendir(DICT_DIR)) == NULL)
	return 0;

    /* List the alternatives */
    while ((de= readdir(ispd)) != NULL)
    {
	namlen= dnamlen(de);
	if (namlen > DICT_SUFFIX_LEN+1 &&
	    !strncmp(de->d_name + namlen - DICT_SUFFIX_LEN - 1,
		"."DICT_SUFFIX, DICT_SUFFIX_LEN+1))
	{
	    if (n++ == 0)
		printf("Dictionaries available are:\n");
	    printf("%.*s\n", namlen - DICT_SUFFIX_LEN - 1, de->d_name);
	}
    }

    closedir(ispd);
    return 0;
#else /* !ASPELL && !DICT_DIR */
    return 1;
#endif
#endif
}


/* CHECK_LINE - Do the spell check for one line of text.  Return one if
 * no further checking should be done, zero otherwise.
 */

int check_line(char *line)
{
    char buf[BUFSIZE+1];
    int offset,word_len,offset_adjust;
    char *guesslist[MAX_GUESS+1];
    int guesses;
    int checkdone= 0;

    fprintf(to,"^%s",line);
    fflush(to);

    offset_adjust= initial_offset;

    while(fgets(buf,BUFSIZE,from) != NULL)
	switch(buf[0])
	{
	case '\n':	/* Line finished */
	    return checkdone;

	case '*':	/* Words spelled right */
	case '+':
	case '-':
	    break;

	case '&':
	case '?':
	case '#':
	    if (!checkdone)
	    {
		parse_ispell(buf, &offset, &word_len, &guesses, guesslist);
		offset+= offset_adjust;
		offset_adjust-= word_len;
		checkdone= fix_word(line,offset,&word_len, guesses,guesslist);
		offset_adjust+= word_len;
	    }
	    break;
	default:
	    printf("weird ISPELL line: %s",buf);
	}
    return checkdone;
}
#endif


#ifdef GISPELL

/* NEXT_WORD - given the length and offset of a word in a line, find the
 * length and offset of the next word.  If there is no next word, a length
 * of zero is returned.
 */

void next_word(char *line, int *offset, int *word_len)
{
    int i;
    char *nw,*ew;
#define isletter(c) (isascii(c) && ( isalpha(c) || c == '\'' ))
#define is1stletter(c) (isascii(c) &&  isalpha(c))

    for (i= *offset + *word_len + 1;
	 line[i] != '\0' && !is1stletter(line[i]);
	 i++)
	;
    *offset= i;

    if (line[i] == '\0')
	*word_len= 0;
    else
    {
	for ( ; isletter(line[i]); i++)
	    ;
	*word_len= i - *offset;
    }
}
 

/* MAKE_GUESSLIST - Parse a GISPELL style guess list into words and store
 * them in the guesslist structure.
 */

void make_guesslist(char **guesslist, char *p, int *guesses)
{
    int off= -1, len= 0;

    *guesses= 0;
    for (;;)
    {
	next_word(p,&off,&len);
	if (len == 0 || *guesses >= MAX_GUESS) break;
	guesslist[(*guesses)++]= p + off;
	*(p + off + len)= '\0';

    }
}


/* SKIP_TO_EQUAL - discard all ispell output up to the next = sign.  Return
 * 1 on end-of-file.
 */

int skip_to_equal()
{
    int ch;

    while ((ch= getc(from)) != '=')
	if (ch == EOF) return 1;
    return 0;
}


/* CHECK_LINE - Do the spell check for one line of text.  Return one if
 * no further checking should be done, zero otherwise.
 */

int check_line(char *line)
{
    char buf[BUFSIZE+1];
    char *guesslist[MAX_GUESS+1];
    int guesses;
    int offset= -1, word_len= 0;
    int checkdone= 0;

    for(;;)
    {
	next_word(line,&offset,&word_len);

	if (word_len == 0) break;

	if (word_len == 1) continue;	/* all one-letter words are correct */

	/* Send word to ispell */

	fprintf(to,"%.*s\n", word_len, line+offset);
	fflush(to);

	/* Read response */

	if (fgets(buf,BUFSIZE,from) == NULL || skip_to_equal())
	{
	    printf("Error: Spell checker exited prematurely\n");
	    return 1;
	}

	/* Words spelled right */
	if (buf[0] == 't' && buf[1] == '\n')
	    continue;

	/* Word spelled wrong */
	if (!strcmp(buf,"nil\n"))
	    guesses= 0;
	else
	    make_guesslist(guesslist,buf,&guesses);

	checkdone= fix_word(line, offset, &word_len, guesses, guesslist);
    }
    return checkdone;
}
#endif


#if defined(IISPELL) || defined(ASPELL) || defined(OLDASPELL)

/* MAKE_GUESSLIST - Compile all the guesses into an array, so we can print
 * them pretty and fetch them back if a user selects one.
 */

void make_guesslist(char **guesslist, char *p, int *guesses)
{
    int i;

    if (*guesses > MAX_GUESS) *guesses= MAX_GUESS;

    for (i= 0; i < *guesses && p[0] != '\0' && p[1] != '\0'; i++)
    {
	guesslist[i]= (++p);
	*(p= firstin(p,",\n"))= '\0';
	p++;
    }
    *guesses= i;
}
#endif


/* PARSE_ISPELL - parse an incorrect word response from Int'l Ispell, picking
 * out the offset (not yet adjusted), word length, and guesslist.
 */

parse_ispell(char *ispell, int *offset, int *word_len,
	   int *guesses, char **guesslist)
{
    char *p;

    *(p= firstin(ispell+2 ," \t\n"))= '\0';
    *word_len= p - (ispell + 2);
    p++;

    if (ispell[0] != '#')
    {
	*guesses= atoi(p);
	p= firstin(p," \t\n") + 1;
    }
    else
	*guesses= 0;

    *offset= atoi(p);

    if (*guesses > 0)
    {
	p= firstin(p,":") + 1;
	make_guesslist(guesslist, p, guesses);
    }
}


/* FIX_WORD - Display the context around the misspelled word, print a list
 * of guesses, if any, and ask the user for a response.  Returns 1 if
 * we should end the spell check process.  Word_len is adjusted to the
 * replacement word length after the call.
 */

int fix_word(char *line, int offset, int *word_len,
	     int guesses, char **guesslist)
{
    char st[BUFSIZE+40];
    char bf[BUFSIZE];
    char *word= line + offset;
    int i,wcol;

    /* Check if we have seen this before */
    if ((wcol= prev_repl(word, *word_len, bf)) < 0) return 0;

    found_some++;

    /* Display the old text line */

    if (junk_above > 0)
    {
	move_up(junk_above,0);
	erase_eos();
    }
    show_line(line, offset, *word_len);

    /* Display the list of guesses */

    if (guesses > 0)
	show_guesses(guesslist, guesses);

    /* Read in a user command */

    sprintf(st,"Replacement for \"%.*s\" (? for help): ", *word_len, word);

    for(;;)
    {
	subtask= 1;
	if (ggetline(bf,NULL,&wcol,st))
	{
	    printf("\nSpell Check Interupted.\n");
	    return 1;
	}
	junk_above++;
	subtask= 0;
	*firstin(bf,"\n")= '\0';

	/* Carry out the command */

	if (bf[0] == '?' && bf[1] == '\0')
	{
	    spell_help(guesses);
	    continue;
	}

	if (bf[0] == '\0')
	{
#ifdef IISPELL
	    fprintf(to,"@%.*s\n", *word_len, word);
	    fflush(to);
	    add_repl(word, *word_len, NULL);	/* ignore this word */
#endif
#ifdef GISPELL
	    fprintf(to,":accept %.*s\n", *word_len, word);
	    fflush(to);
	    if (skip_to_equal()) return 1;
#endif
	    return 0;
	}

	if (bf[0] == '\\' && bf[1] == '\0')
	    return 1;

	if (bf[0] == '+' && bf[1] == '\0')
	{
#ifdef IISPELL
	char *oldword= NULL;

	    /* See if we should convert this to lower case */
	    for (i= 0; i < *word_len; i++)
		if (isupper(word[i]))
		{

		    sprintf(bf,
		      "Should \"%.*s\" always be capitalized this way [yn]? ",
		      *word_len, word);
		    junk_above++;
		    if (!readyes(bf))
		    {
			oldword= word;
			word= (char *)malloc(*word_len+1);
			for (i= 0; oldword[i] != '\0'; i++)
			    word[i]= isupper(oldword[i]) ?
				tolower(oldword[i]) : oldword[i];
		    }
		    break;
		}
	    fprintf(to,"*%.*s\n", *word_len, word);
	    fflush(to);
	    add_repl(word, *word_len, NULL);	/* ignore this word */
	    if (oldword != NULL) free(word);
#endif
#ifdef GISPELL
	    fprintf(to,":insert %.*s\n", *word_len, word);
	    fflush(to);
	    if (skip_to_equal()) return 1;
#endif
	    return 0;
	}

	if (bf[0] == '#' || (bf[0] >= '0' && bf[0] <= '9'))
	{
	    if ((i= atoi(bf+(bf[0] == '#'))) <= 0 || i > guesses)
	    {
		printf("Possible guesses are:\n");
		junk_above++;
		show_guesses(guesslist,guesses);
		continue;
	    }

	    do_replace(line, offset, word_len, guesslist[i-1]);
	}
	else
	    do_replace(line, offset, word_len, bf);

	return 0;
    }
}


/* START_ISPELL - launch ispell and do whatever initialization is necessary.
 * Return the open file descriptors "to" and "from" and a copy of the startup
 * banner in "banner".  Return 1 on failure.
 */

#if defined(IISPELL) || defined(ASPELL) || defined(OLDASPELL)
int start_ispell(char *banner)
{
    int fd_to, fd_from;
    char buf[BUFSIZE+1];
    int ch;

    sprintf(buf,"%s -a -S -d %s", SPELL_PATH, language);

    if (spell_pid= cmd_pipe(buf,1, &fd_to, &fd_from) < 0)
    {
	printf("Error: could not execute %s\n",SPELL_PATH);
	return 1;
    }
    to= fdopen(fd_to,"w");
    from= fdopen(fd_from,"r");

    fputs("!\n",to);	/* Put ispell in terse mode */
    fflush(to);

    /* Read Ispell's banner line */

    if ((ch= getc(from)) == EOF || ch != '@' ||
	fgets(banner, BUFSIZE, from) == NULL)
    {
	printf("Error: could not get ispell's banner\n");
	if (ch == '*')
	    printf("  You might have ispell 4.0 (recompile with GISPELL)\n");
	fclose(to);
	fclose(from);
	return 1;
    }
    *firstin(banner,"\n")= '\0';

    /* Some versions of international ispell include the ^ when they report
     * the offset of an error in a line, some don't.  We figure it out by
     * trying it.
     */

    if (initial_offset == 1)
    {
	fputs("^xfgzepqq\n",to);	/* some reasonably long non-word */
	fflush(to);
	if (fgets(buf, BUFSIZE, from) == NULL || getc(from) != '\n')
	{
	    printf("Error: premature termination of ispell\n");
	    return 1;
	}
	initial_offset= -atoi(firstin(buf,"0123456789"));
    }
    return 0;
}
#endif

#ifdef GISPELL
start_ispell(char *buf)
{
    int fd_to, fd_from;
    int ch,i;

    sprintf(buf,"%s -S", SPELL_PATH);

    if (spell_pid= cmd_pipe(buf,1, &fd_to, &fd_from) < 0)
    {
	printf("Error: could not execute %s\n",SPELL_PATH);
	return 1;
    }
    to= fdopen(fd_to,"w");
    from= fdopen(fd_from,"r");

    if (getc(from) != '(' /*)*/ )
    {
	printf("Error: could not get ispell's banner\n");
	goto fail;
    }
    while ((ch= getc(from)) != '"')
	if (ch == EOF || ch == '\n')
	    goto fail;

    i= 0;
    while ((ch= getc(from)) != '"')
	if (ch == EOF || ch == '\n')
	    goto fail;
	else
	    buf[i++]= ch;
    buf[i]= '\0';

    while ((ch= getc(from)) != '=')
	if (ch == EOF || ch == '\n')
	    goto fail;

    return 0;

fail:;
    fclose(to);
    fclose(from);
    return 1;
}
#endif


/* CMD_SPELL - Entry Point for the Spell checker.  */

void cmd_spell(char *arg)
{
    char buf[2*BUFSIZE+1];
    FILE *cfp;
    int checkdone= 0;
    int ch;
    pid_t pid;

    junk_above= 0;

    if (start_ispell(buf)) return;

    if (buf[0] != '\0' && UP)	/* only display banner if we can erase it */
    {
	printf("%s\n",buf);
	junk_above++;
    }

    /* Get the files all set up */

    if ((cfp= make_copy()) == NULL)
    {
	printf("Error: could create copy of text file\n");
	return;
    }
    fflush(cfp);
    fseek(cfp,0L,0);
    emptyfile();

    if (*arg == '!') empty_repl();

    found_some= 0;

    /* Main Spell Check Loop - check each line of file */

    while(fgets(buf,BUFSIZE,cfp) != NULL)
    {
	if (!checkdone)
	    checkdone= check_line(buf);
	fputs(buf,tfp);
    }

    if (junk_above > 0)
    {
	move_up(junk_above,0);
	erase_eos();
    }
    if (!found_some)
	printf("No spelling errors detected.\n");

    fflush(tfp);
    fclose(cfp);

    /* Close the pipe and wait for child to terminate */
    fclose(to);
    fclose(from);
    while ((pid=wait((int *)NULL)) != -1 && pid != spell_pid)
	;

    return;
}

#endif /* ISPELL */

#ifdef UNIX_SPELL

	/********** GENERIC UNIX SPELL INTERFACE *************/

#ifndef SPELL_PATH
#define SPELL_PATH "spell"
#endif
#ifndef SPELLIN_PATH
#define SPELLIN_PATH "spellin"
#endif


/* SET_LANGUAGE - Check if we have a dictionary for the given language.  If so,
 * return 1.  If not, list legal languages and return 0.
 */
int set_language(char *str, char *source)
{
    static char *lang[]= {"english","american","british",NULL};
    int i,f;

    if (!strcmp(str,LANG_DEFAULT)) return 1;

    for (i= 0; lang[i] != NULL; i++)
	if (!strcmp(str,lang[i])) return 1;
    
    printf("%s error: No dictionary available for language '%s'.\n",
	source, str);

    if (*source != ':') return 0)

    printf("Dictionaries available are:\n");
    f= 1;
    for (i= 0; lang[i] != NULL; i++)
    {
	printf("%s\n",lang[i]);
	if (f && !strcmp(lang[i],LANG_DEFAULT)) f= 0;
    }
    if (f)
	printf("%s\n",LANG_DEFAULT);
}


/* HOMEDIR - Return the user's home directory name.  We try to get it from
 * the environment variable first, and only go into the password file if
 * the environment variable isn't around. */

char *homedir()
{
    char *r;
    struct passwd *pwd;

    /* Try the environment variable first */
    if ((r= getenv("HOME")) != NULL) return r;
    
    /* Try the password file */
    setpwent();
    pwd= getpwuid(getuid());
    endpwent();
    if (pwd != NULL) return pwd->pw_dir;

    /* Give up */
    return NULL;
}


/* MYDICT - Return the pathname of the user's personal dictionary file */

char *mydict()
{
    static char buf[60];
    char *home;

    if ((home= homedir()) == NULL)
	return NULL;

#ifdef SIMPLE_SPELL
    sprintf(buf,"%s/.dict%c", home, strcmp(language,"british") ? 'a' : 'b');
#else
    sprintf(buf,"%s/.word%c", home, strcmp(language,"british") ? 'a' : 'b');
#endif

    return buf;
}


/* MKSPELLCMD -- Return the proper shell command to use to invoke the
 * spelling program to check stdin for spelling and write a list of
 * incorrect words to stdout.
 */

char *mkspellcmd()
{
    static char buf[BUFSIZE];
    char *p, *q, *dict;

    p= strpcpy(buf,SPELL_PATH);

    /* Check if the user has his own .dict? or .word? file. */
    if ((dict= mydict()) != NULL && !access(dict,R_OK))
    {
#ifdef SIMPLE_SPELL
	q= strpcpy(p," -d ");
#else
	q= strpcpy(p," +");
#endif
	q= strpcpy(q,dict);
	return buf;
    }

    if (!strcmp(language,"british")) p= strpcpy(p," -b");

    return buf;
}

/* ADDWORD -- Add a word to the user's spelling dictionary.
 * 
 */

FILE *addfp= NULL;
char tempfile[BUFSIZE];

void addword(char *word)
{
    static char buf[BUFSIZE];
    char *dict;

    if (addfp == NULL)
    {
	tempfile[0]= '\0';

	if ((dict= mydict()) == NULL)
	{
	    printf("cannot find your home directory!\n");
	    return;
	}

#ifdef SIMPLE_SPELL
	if (access(dict,R_OK))
	{
	    /* Doesn't already have a dictionary */

	    junk_above= 0;
	    printf("Creating %c%s English dictionary %s...\n",
		    islower(language[0]) ? toupper(language[0]):language[0],
		    language+1, dict);
	    printf("This will require about 50K of disk space.\n");
	    if (askquest("Are you sure you want to do this? ","ny") == 'n')
		    return;

	    sprintf(buf,"%s %s%c > %s",
		    SPELLIN_PATH,
		    SYSDICT_PATH, (strcmp(language,"british") ? 'a' : 'b'),
		    dict);
	}
	else
	{
	    /* Move our dictionary to tmp location */
	    sprintf(tempfile,"%s.tmp",dict);
	    if (mv_file(dict,tempfile)) return;

	    sprintf(buf,"%s %s > %s",
		    SPELLIN_PATH,
		    tempfile,
		    dict);
	}
	if ((addfp= upopen(buf,0,"w")) == NULL)
	{
	    printf("could not start child process\n");
	    if (tempfile[0] != '\0')
		(void) mv_file(tempfile,dict);
	    return;
	}
#else
	if (access(dict,R_OK))
	{
	    junk_above= 0;
	    printf("Creating %c%s English dictionary %s...\n",
		    islower(language[0]) ? toupper(language[0]):language[0],
		    language+1, dict);
	    sprintf(buf,"%s -u -o %s", SORT_PATH, dict);
	}
	else
	    sprintf(buf,"%s -u %s - -o %s", SORT_PATH, dict, dict);

	if ((addfp= upopen(buf,1,"w")) == NULL)
	{
	    printf("could not start child process\n");
	    return;
	}
#endif
    }

    fputs(word,addfp);
    putc('\n',addfp);
}


/* END_ADDWORD -- terminate the addword process.
 */

void end_addword()
{
    if (addfp == NULL) return;

    printf("writing dictionary...\n");
    upclose();
    addfp= NULL;

    if (tempfile[0] != '\0')
	(void) unlink(tempfile);
}


/* CMD_SPELL -- Run a spell checker on the file, and then go through a
 * substitution process for each misspelled word.  This returns TRUE if
 * we stopped with a "continue text entry" request.  Otherwise it returns
 * FALSE.  This version doesn't use the argument.
 */

void cmd_spell(char *arg)
{
    FILE *sfp;
    char word[BUFSIZE+1];
    int nwords= 0;
    char *p;

    if ((sfp= pipe_thru(mkspellcmd())) == NULL) return;

    junk_above= 0;
    while (fgets(word+1,BUFSIZE-2,sfp) != NULL)
    {
	word[0]= CSEP;
	nwords++;
	p= firstin(word+1,"\n");
	p[0]= CSEP; p[1]= '\0';
	if (do_substitute(word,NULL)) break;
    }
    if (nwords == 0)
	printf("No spelling errors detected.\n");

    fclose(sfp);

    end_addword();
}
#endif /* UNIX_SPELL */

#endif /* !NO_SPELL */
