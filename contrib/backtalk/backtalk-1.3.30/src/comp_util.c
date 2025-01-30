/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include "sysdict.h"
#include "comp.h"
#include "comp_util.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

HashTable constdict;	/* Dictionary of constants defined in config.bt */
int cd_n;		/* Number of variables stored in constdict */
int cd_size= 0;		/* Size of constdict */

extern int curr_file;

/* Substitutions done on strings and regular expressions */
static struct ch_sub {
	char ch;
	char sub;
	} ch_subs[] = {
	{'a',	'\a'},
	{'b',	'\b'},
	{'f',	'\f'},
	{'n',	'\n'},
	{'r',	'\r'},
	{'t',	'\t'},
	{'v',	'\v'},
	{'\0',	'\0'}};

/* INIT_DICT - routine to initialize system dictionary and constant dictionary
 */

void init_dict()
{
    InitHashTable(&constdict);
    init_sysdict();
}


/* ADD_CONST - Add a symbol to the constant dictionary.  Don't set a value,
 * just destroy any old value and return a pointer to the token.
 */

Token *add_const(char *label)
{
    HashEntry *h;
    int new;

    h= CreateHashEntry(&constdict, label, &new);
    if (!new) free_val(&h->t);
    return &(h->t);
}


/* DEF_CONSTANT - Add a constant to the constant table.  This will use all
 * new memory, so the token can be safely deallocated later.  If sysonly is
 * true, then we only actually store the constant if it would be saved to
 * a constant entry in the system table.  Otherwise we silently ignore the
 * whole thing.  Returns 0 if we didn't save it because it wasn't in the
 * system dictionary and sysonly was set, returns 1 if we saved it to the
 * constant table, returns 2 if we saved it to the system dictionary.
 */

int def_constant(Token *var, Token *val, int sysonly)
{
    int i= -1;
    int rc= 1;
    Token st, *va= NULL;
    HashEntry *h;

    /* Look for it in the system dictionary first */
    if (type(*var) == TK_BOUND_LITERAL)
    	i= ival(*var);
    else if ((h= FindHashEntry(&syshash,sval(*var))) != NULL)
    	i= h - sysdict;

    if (i >= 0)
    {
	/* Was in system dictionary - make sure it is a constant */
	if (!(sysdict[i].flag & DT_CONSTANT))
	{
	    if (sysonly) return 0;
	    die("System variable %s is not a constant.", sysdict[i].key);
	}
	if (!initializing)
	    die("Cannot modify system constant %s.", sysdict[i].key);
	va= &sysdict[i].t;
	rc= 2;
    }
    else if (sysonly)
        return 0;
    else if ((h= FindHashEntry(&constdict,sval(*var))) != NULL)
    {
	/* Was in constant dictionary */
	va= &(h->t);
    }

    if (va != NULL)
    {
	/* Free any previous value if we are resetting an existing var */
	if (class(*va) == TKC_STRING && (va->flag & TKF_FREE))
	    free(sval(*va));
    }
    else
    {
	/* Find space for a new variable */
	va= add_const(sval(*var));
    }

    /* Save new value in dictionary */
    *va= *val;

    /* Copy string constants into fresh storage */
    if (class(*va) == TKC_STRING && (va->flag & TKF_FREE))
	    va->val= (void *)strdup(sval(*va));

    return rc;
}


/* UNDEF_CONSTANT - Remove a constant from the constant table.  You cannot
 * undefine system constants (the ones in the system dictionary).
 */

void undef_constant(Token *var)
{
    int i= -1;
    int rc= 1;
    Token st, *va= NULL;
    HashEntry *h;

    /* Make sure it isn't in the system dictionary first */
    if (type(*var) == TK_BOUND_LITERAL ||
        FindHashEntry(&syshash,sval(*var)) != NULL)
    	die("Cannot undefined system constant");

    if ((h= FindHashEntry(&constdict,sval(*var))) == NULL) return;
    DeleteHashEntry(h);
}


/* DUMP_DICT - printout all variables in a dictionary.  Debugging only.
 */

void dump_dict(HashTable *dict, FILE *fp)
{
    int col= 0;
    int len;
    char bf[BFSZ+1];
    HashEntry *h;
    HashSearch hs;

    for (h= FirstHashEntry(dict, &hs); h != NULL; h= NextHashEntry(&hs))
    {
	switch (type(h->t))
	{
	case TK_FUNCTION:
	    continue;
	case TK_INTEGER:
	    sprintf(bf,"%s=%ld",h->key, ival(h->t));
	    break;
	case TK_STRING:
	    sprintf(bf,"%s=(%s)",h->key, sval(h->t));
	    break;
	case TK_PROCEDURE:
	    sprintf(bf,"%s={...}",h->key);
	    break;
	case TK_UNBOUND_SYMBOL:
	    sprintf(bf,"%s=%s",h->key, sval(h->t));
	    break;
	case TK_UNBOUND_LITERAL:
	    sprintf(bf,"%s=/%s",h->key, sval(h->t));
	    break;
	case TK_BOUND_SYMBOL:
	    sprintf(bf,"%s=%s(bound %d)",h->key,
		sysdict[ival(h->t)].key, ival(h->t));
	    break;
	case TK_BOUND_LITERAL:
	    sprintf(bf,"%s=/%s(bound %d)",h->key,
		sysdict[ival(h->t)].key, ival(h->t));
	    break;
	case TK_MARK:
	    sprintf(bf,"%s=mark(%ld)",h->key, ival(h->t));
	    break;
	case TK_ARRAY:
	    sprintf(bf,"%s=[...]",h->key);
	    break;
	default:
	    sprintf(bf,"%s=<weird type>",h->key);
	    break;
	}
	len= strlen(bf);
	if (col+len+1 > 78)
	    { col= 0; putc('\n',fp);}
	else if (col > 0)
	    { col++; putc(' ',fp);}
	fputs(bf,fp);
	col+= len;
    }
    if (col != 0)
	putc('\n',fp);
}


/* GETINCH - get the next input character from given file.
 */

int getinch(FILE *fp)
{
    int ch;

    ch= getc(fp);
    if (ch == '\n') curr_line++;	/* Increment line number */
    return ch;
}

/* UNGETINCH - unget the given input character to either the current file
 * or string.  The character ungot must be the same one just read.
 */

int ungetinch(int ch, FILE *fp)
{
    if (ch == '\n') curr_line--;	/* Decrement line number */
    return ungetc(ch,fp);
}


/* READ_STRING - This reads a string delimited by matching parenthesis.
 * It assumes the opening ( has already been read.  It scans up till a
 * balanced closing ) not counting any parens that are preceeded by an odd
 * number of \ characters.  A pointer to the string in malloc() storage will
 * be returned.  There is no upper limit to the length of the string, as
 * storage will be expanded as needed.  Standard escape sequences are expanded
 * out.
 */

char *read_string(FILE *fp)
{
    char bf[BFSZ], *string= NULL;
    int ch, i=0, j, nest, len=0, esc= FALSE;
    int old_line= curr_line;

    /* Initialize nesting level of parens (we've already read one) */
    nest= 1;

    for (;;)
    {
	/* Get the next input character */
	if ((ch= getinch(fp)) == EOF)
	{
	    curr_line= old_line;
	    die("Unterminated String");
        }

	/* Count opening and closing braces */
	if (!esc)
	{
	    if (ch == '(')
		nest++;
	    else if (ch == ')')
		nest--;
	}
	else
	{
	    for (j= 0; ch_subs[j].ch != '\0'; j++)
	    {
	    	if (ch == ch_subs[j].ch)
		{
			ch= ch_subs[j].sub;
			break;
		}
	    }
	}

	esc= (!esc && ch == '\\');

	/* If buffer is full, or we're done, copy buffer to malloced space */
	if (i == BFSZ || nest == 0)
	{
	    if (string != NULL)
		string= (char *)realloc(string, len+i+1);
	    else
		string= (char *)malloc(i+1);
	    strncpy(string+len,bf,i);
	    len+= i;
	    string[len]= '\0';
	    if (nest == 0)
		return string;
	    i= 0;
	}

	/* Save the newly read character */
	if (!esc) bf[i++]= ch;
    }
}


/* READ_REGEX - This reads in and compiles a regular expression.  It assumes
 * the opening '<<' has already been read.  It scans up till  a '>>' string is
 * found, then checks immediately after that for 'i' and 'n' flags.  Sequences
 * like '\n' and '\s' are expanded as in the table below.  The regular
 * expression is compiled, bombing on any error.  The compiled regulare
 * expression is returned.
 */


Regex *read_regex(FILE *fp)
{
    char bf[BFSZ], *string= NULL;
    int ch, j, flags, i= 0, len= 0, done= 0, esc= FALSE;
    int old_line= curr_line;
    Regex *re;

    /* Read regular expression string */
    for (;;)
    {
	/* Get the next input character */
	if ((ch= getinch(fp)) == EOF)
	{
	    curr_line= old_line;
	    die("Unterminated Regular Expression");
        }

	if (!esc)
	{
	    if (ch == '>')
	    {
	    	if ((ch= getinch(fp)) == '>')
		    done= 1;
		else
	    	{
		    ungetinch(ch,fp);
		    ch= '>';
		}
	    }
	}
	else
	{
	    /* Find appropriate /x substitution */
	    for (j= 0; ch_subs[j].ch != '\0'; j++)
	    {
	    	if (ch == ch_subs[j].ch)
		{
		    ch= ch_subs[j].sub;
		    break;
		}
	    }
	}

	/* Is current character a backslash escape character? */
	esc= (!esc && ch == '\\');
	if (esc) continue;

	/* If buffer is full, or we're done, copy buffer to malloced space */
	if (done || i+1 > BFSZ)
	{
	    if (string != NULL)
		string= (char *)realloc(string, len+i+1);
	    else
		string= (char *)malloc(i+1);
	    strncpy(string+len,bf,i);
	    len+= i;
	    string[len]= '\0';
	    if (done) break;
	    i= 0;
	}

	/* Save the newly read character */
	bf[i++]= ch;
    }

    /* Look for flags */
    flags= REG_EXTENDED;
    for (;;)
    {
	if ((ch= getinch(fp)) == 'i')
	    flags|= REG_ICASE;
	else if (ch == 'm')
	    flags|= REG_NEWLINE;
	else
	{
	    ungetinch(ch,fp);
	    break;
	}
    }

    /* Compile regular expression string */

    re= (Regex *)malloc(sizeof(Regex));
    re->lk= 1;
    if (j= regcomp(&re->re, string, flags))
    {
	regerror(j, &re->re, bf, BFSZ);
    	die("Regular Expression Error: %s",bf);
    }
    
    return re;
}


/* GET_TOKEN - read in the next token from given file, and return a
 * pointer to a static Token structure describing it.  Return NULL on
 * end-of-file.  We don't return procedures as TK_PROCEDURE types.  Instead,
 * the { and the } are each returned as separate tokens in a flat stream of
 * tokens.  They have type TK_UNBOUND_SYMBOL and with string value "{" or "}".
 */

Token *get_token(FILE *fp)
{
    static Token t;		/* storage for returned token */
    char sym[MAXSYMLEN+1];	/* temp storage for symbol names */
    int ch;

    /* Skip white space and comments */
    for (;;)
    {
	while ((ch= getinch(fp)) == ' ' || ch == '\t' || ch == '\n')
	    ;
	if (ch != '%')
	    break;
	while ((ch= getinch(fp)) != EOF && ch != '\n')
	    ;
    }
    
    if (ch == EOF)
	return NULL;

    t.line= curr_line;
    t.sfid= curr_file;

    /* Handle string tokens */
    if (ch == '(')
    {
	t.flag= TK_STRING | TKF_FREE;
	t.val= (void *)read_string(fp);
	return &t;
    }

    /* Handle integer tokens */
    if (isdigit(ch) || ch == '-')
    {
	/* Read an integer - could use fscanf, but I hate it */
	long val;
	int sgn= 1;
	if (ch == '-')
	{
	    sgn= -1;
	    if ((ch= getinch(fp)) == EOF || !isdigit(ch))
	    {
		/* Isolated - sign */
		ungetinch(ch,fp);
		ch= '-';
		t.flag= TK_UNBOUND_SYMBOL;
		goto symbol;
	    }
	}
        val= ch - '0';
	
	while ((ch= getinch(fp)) != EOF && isdigit(ch))
	    val= 10*val + ch - '0';
	ungetinch(ch,fp);

	t.flag= TK_INTEGER;
	t.val= (void *)(sgn * val);
	return &t;
    }
    
    /* Handle regular expression tokens */
    if (ch == '<')
    {
    	if ((ch= getinch(fp)) == EOF || ch != '<')
	{
	    /* Isolated < sign */
	    ungetinch(ch,fp);
	    ch= '<';
	    t.flag= TK_UNBOUND_SYMBOL;
	    goto symbol;
	}
	t.flag= TK_REGEX | TKF_FREE;
	t.val= (void *)read_regex(fp);
	return &t;
    }

    /* Everything else is a symbol or literal */
    t.flag= TK_UNBOUND_SYMBOL;

    if (isalpha(ch) || ch == '_' || ch == '.' || ch == '/')
    {
	/* Alphanumeric symbol */
	int i= 0;
	if (ch == '/')
	    t.flag= TK_UNBOUND_LITERAL;
	else
	    sym[i++]= ch;

	for ( ;(ch= getinch(fp)) != EOF &&
	       (isalnum(ch) || ch == '_' || ch == '.'); i++)
	{
	    if (i >= MAXSYMLEN)
		die("Symbol name %.*s... too long",MAXSYMLEN-1,sym);
	    sym[i]= ch;
	}
	ungetinch(ch,fp);
	if (i == 0)	/* only happens if we had a lone "/" */
	{
	   t.flag= TK_UNBOUND_SYMBOL;
	   sym[i++]= '/';
	}
	sym[i]= '\0';
    }
    else
    {
	symbol: /* Other symbols are one character long */
	sym[0]= ch;
	sym[1]= '\0';
    }

    t.val= (void *)(strdup(sym));
    return &t;
}


/* ADD_FILE_NAME - Add a name to the file name list.  Returns the index of
 * the file name.  New memory will be allocated internally for a copy of the
 * name.
 */

struct fnl {char *name; struct fnl *next;} *fnamelist= NULL;

int add_file_name(char *fname)
{
    struct fnl *new, *curr, *prev;
    int i;

    /* Loop through existing list, looking for our name */
    for (curr= fnamelist, prev=NULL, i= 0;
         curr != NULL;
         prev= curr, curr= curr->next, i++)
   {
	/* Found the name - return it's index */
	if (!strcmp(fname, curr->name))
		return i;
    }

    /* Check for overflow */
    if (i > 255) die("Too many files included in one program");

    /* Name is new - make a node for it */
    new= (struct fnl *)malloc(sizeof(struct fnl));
    new->name= strdup(fname);
    new->next= NULL;

    /* Append to the list */
    if (fnamelist == NULL)
    	fnamelist= new;
    else
    	prev->next= new;

    return i;
}


/* FIND_FILE_NAME - Given an index, return the filename associated with it.
 */

char *find_file_name(int index)
{
    struct fnl *curr;
    int i;

    for (curr= fnamelist, i= 0; curr != NULL; curr= curr->next, i++)
	if (i == index) return curr->name;

    return "<unknown file>";
}


/* WRITE_FILE_LIST - Write out the file list in binary format to the given
 * open file.  Format consists of first a one-byte binary integer giving the
 * number of files in the list, followed by each null-terminated file name
 * in file_table order.
 */

void write_file_list(int fd)
{
    struct fnl *curr;
    int n;
    size_t len;

    /* Count elements in the list */
    for (curr= fnamelist, n= 0; curr != NULL; curr= curr->next, n++)
    	;

    /* Write the count */
    write_val(fd,1,n);

    /* Write out the file names */
    for (curr= fnamelist; curr != NULL; curr= curr->next)
    {
    	len= strlen(curr->name);
	write_val(fd,4,len);
	write(fd, curr->name, len);
    }
}
