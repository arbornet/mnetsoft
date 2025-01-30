/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <ctype.h>

#include "dict.h"
#include "str.h"
#include "stack.h"
#include "free.h"
#include "sysdict.h"
#include "http.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


typedef struct dictstack {
	HashTable hash;
	struct dictstack *next;
	} DictStack;

DictStack *dictstack= NULL;

HashEntry *find_dictstack(char *label);
HashEntry *add_dict(DictStack *s, char *label, Token *t, int flag);

#ifndef isascii
#define isascii(c)      ((unsigned)(c)<=0177)
#endif


/* DUMP_DICT - printout all variables in a dictionary.  Debugging only.
 */

void dump_dict(HashTable *dict, FILE *fp)
{
    int col= 0;
    int len,typestring= FALSE;
    char bf[BFSZ+1];
    HashEntry *h;
    HashSearch hs;

    for (h= FirstHashEntry(dict, &hs); h != NULL; h= NextHashEntry(&hs))
    {
	if (class(h->t) == TKC_FUNCTION) continue;

	if (type(h->t) == TK_INTEGER)
	    sprintf(bf,"%s=%ld",h->key, ival(h->t));
	else if (type(h->t) == TK_TIME)
	    sprintf(bf,"%s=@%ld",h->key, tval(h->t));
	else if (type(h->t) == TK_STRING)
	{
	    sprintf(bf,"%s=(",h->key);
	    typestring= TRUE;
	}
	else if (type(h->t) == TK_UNBOUND_LITERAL)
	    sprintf(bf,"%s=/%s",h->key, sval(h->t));
	else if (type(h->t) == TK_UNBOUND_SYMBOL)
	    sprintf(bf,"%s=%s",h->key, sval(h->t));
	else if (type(h->t) == TK_MARK)
	    sprintf(bf,"%s=mark",h->key);
	else if (type(h->t) == TK_PROCEDURE)
	    sprintf(bf,"%s={...}",h->key);
	else if (type(h->t) == TK_ARRAY)
	    sprintf(bf,"%s=[...]",h->key);
	else if (type(h->t) == TK_REGEX)
	    sprintf(bf,"%s=<<...>>",h->key);
	else
	    sprintf(bf,"%s=<weird type>",h->key);
	len= strlen(bf);
	if (col+len+1 > 78)
	    { col= 0; putc('\n',fp);}
	else if (col > 0)
	    { col++; putc(' ',fp);}
	fputs(bf,fp);
	col+= len;
	if (typestring)
	{
	    char *cp, *ch, clen;
	    char oct[5];
	    for (cp= sval(h->t); *cp != '\0'; cp++)
	    {
		switch (*cp)
		{
		case '\n': ch= "\\n";  clen= 2; break;
		case '\r': ch= "\\r";  clen= 2; break;
		case '\b': ch= "\\b";  clen= 2; break;
		case '\t': ch= "\\t";  clen= 2; break;
		case '\\': ch= "\\\\"; clen= 2; break;
		case '(':  ch= "\\(";  clen= 2; break;
		case ')':  ch= "\\(";  clen= 2; break;
		case '<':  ch= ttymode ? "<" : "&lt;";  clen= 1; break;
		case '>':  ch= ttymode ? ">" : "&gt;";  clen= 1; break;
		case '&':  ch= ttymode ? "&" : "&amp;"; clen= 1; break;
		default:
		    if (!isascii(*cp) || !isprint(*cp))
		    {
			sprintf(ch= oct,"\\%03o",(unsigned char)(*cp)); clen= 4;
		    }
		    else
		    {
			ch= NULL; clen= 1;
		    }
		    break;
		}
		if (col+clen > 78)
		    {col= 0; fputs("\\\n",fp);}
		if (ch == NULL)
		    putc(*cp, fp);
		else
		    fputs(ch, fp);
		col+= clen;
	    }
	    putc(')',fp);
	    col++;
	    typestring= FALSE;
	}
    }
    if (col != 0)
	putc('\n',fp);
}


/* DUMP_DICT_STACK - Print all variables in all dictionaries on the stack.
 * Debugging only.
 */

void dump_dict_stack(FILE *fp)
{
    DictStack *ds;
    int n;

    /* Count number of user dictionaries */
    for (ds= dictstack, n= 0; ds != NULL; ds= ds->next, n++)
	;

    for (ds= dictstack; n > 0; n--, ds= ds->next)
    {
	fprintf(fp,"User Dictionary %d:\n", n-1);
	dump_dict(&ds->hash, fp);
	fputc('\n', fp);
    }

    fprintf(fp,"System Dictionary:\n", n);
    if (syshash.buckets == NULL)
	fprintf(fp,"(not yet initialized)\n");
    else
	dump_dict(&syshash, fp);
}


/* BIND_SYSDICT - Given a label, find it in the system dictionary.  If
 * it is there, return it's index.  If not, return -1.
 */

int bind_sysdict(char *label)
{
    HashEntry *h;

    if ((h= FindHashEntry(&syshash,label)) == NULL)
    	return -1;
    else
    	return h-sysdict;
}


/* GET_SYSDICT - Given a label, find it's value in the system dictionary.
 * If it is there, return it's value.  If not, return NULL;
 */

Token *get_sysdict(char *label)
{
    int i= bind_sysdict(label);
    if (i < 0) return NULL;
    return &(sysdict[i].t);
}


/* FIND_DICTSTACK - return a pointer to the HashEntry structure for the named
 * symbol in the topmost dictionary in which it is defined.  Returns NULL
 * if it is not defined.  Note that we do not check in the system dictionary
 * since those references are supposed to have been resolved at compile
 * time.
 */

HashEntry *find_dictstack(char *label)
{
    DictStack *s;
    HashEntry *h;

    for (s= dictstack; s != NULL; s= s->next)
    {
	if ((h= FindHashEntry(&(s->hash),label)) != NULL)
	    return h;
    }
    return NULL;
}


/* GET_DICT - Return a pointer to the Token structure for the given symbol
 * name.  This searches the entire dictionary stack, and if it isn't in the
 * dictionary it looks in the environment.  If it isn't there either, it
 * returns NULL.  We don't search the constant dictionary or the system
 * dictionary, because that is supposed to have been done at compile time.
 */

Token *get_dict(char *label)
{
    HashEntry *h;
    static Token t;
#ifdef DFLT_SCRIPT_NAME
    static char dflt_script_name[]= DFLT_SCRIPT_NAME;
#endif

    /* Search the Dictionary Stack */
    if ((h= find_dictstack(label)) != NULL)
	return &(h->t);

    /* HTTP_HOST environment variable needs special treatment */
    if (!strcmp(label,"HTTP_HOST"))
	t.val= get_http_host();
    /* Look for other things in the environment */
    else if ((t.val= (void *)getenv(label)) == NULL)
    {
#ifdef DFLT_SCRIPT_NAME
	/* SCRIPT_NAME has a default value if it isn't defined */
	if (!strcmp(label,"SCRIPT_NAME"))
	    t.val= dflt_script_name;
	else
#endif
	    return NULL;
    }
    
    /* Return the environment variable */
    t.flag= TK_STRING;
    return &t;
}


/* GET_SYMBOL - Return a pointer to the Token structure for the given symbol,
 * which may be either a bound or unbound literal or token structure.  Returns
 * NULL if the symbol is unbound and undefined.
 */

Token *get_symbol(Token *s)
{
    if (class(*s) == TKC_INTEGER)
	/* Bound symbol or literal - return sysdict entry */
	return &sysdict[ival(*s)].t;
    else
	return get_dict(sval(*s));
}


/* LABEL_OK - Check if the given character string obeys the rules to be a valid
 * symbol name.
 */

void label_ok(char *l)
{
    char *p;

    if (!isascii(l[0]) || !isprint(l[0]) || isdigit(l[0]))
	die("Symbol %s has bad initial character\n",l);
    
    if (ispunct(l[0]))
    {
	if (l[1] != '\0')
	    die("Illegal symbol %s",l);
    }
    else
    {
	for (p= l+1; *p != '\0'; p++)
	    if (!isalnum(*p) && *p != '_' && *p != '.')
		die("Bad character in symbol %s",l);
	    
	if (p - l > MAXSYMLEN)
	    die("Symbol name %s too long",l);
    }
}


/* FUNC_DICT: - dict <dictionary>
 * create a new dictionary
 */

void func_dict()
{
    HashTable *t= (HashTable *)malloc(sizeof(HashTable));
    InitHashTable(t);
    push_dict(t,FALSE);
}


/* FUNC_BEGIN: <dictionary>  begin -
 * push a dictionary on the dictionary stack
 */

void func_begin()
{
    HashTable *d= pop_dict();
    DictStack *s;

    /* Create a dictstack entry containing the hash */
    s= (DictStack *)malloc(sizeof(DictStack));
    s->hash= *d;
    if (d->buckets == d->staticBuckets)
    	s->hash.buckets= s->hash.staticBuckets;
    free(d);

    /* Link it in at top of stack */
    s->next= dictstack;
    dictstack= s;
}


/* FUNC_END:  - end -
 * pop a dictionary off the stack
 */

void func_end()
{
    DictStack *s;
    int i;

    if (dictstack == NULL)
	die("No User Dictionaries on Dictionary Stack");
    
    /* Remove dictionary from dictionary stack */
    s= dictstack;
    dictstack= s->next;

    /* Free up hash and everything in it */
    DeleteHashTable(&(s->hash));

    /* Free dictionary stack entry */
    free(s);
}

#ifdef CLEANUP

/* FREE_DICTSTACK - This deallocates everything still on the dictionary stack.
 * We only bother doing this if we are testing for memory leaks.
 */
void free_dictstack()
{
    HashEntry *h;
    HashSearch hs;

    /* deallocate all user dictionaries */
    while (dictstack != NULL)
	func_end();

    /* deallocate all malloced variables in the system dictionary */
    for (h= FirstHashEntry(&syshash, &hs); h != NULL; h= NextHashEntry(&hs))
	free_val(&(h->t));
}

#endif


/* DUP_DICT - If the given system dictionary value was not already in
 * dynamic memory, then copy it into dynamic memory.  Returns a pointer
 * to the value.
 */

char *dup_dict(int i)
{
    Token *tk= &(sysdict[i].t);

    if (!(tk->flag & (TKF_FREE)))
    {
	tk->val= strdup(tk->val);
	tk->flag|= TKF_FREE;
    }
    return (char *)tk->val;
}


/* SETS - Set the given system dictionary entry to string value.  This assumes
 * the variable was already a string type.  There is a seti() too, but it's
 * a macro.
 */

char *sets(int i, char *val, int copy)
{
    Token *tk= &(sysdict[i].t);

    if ((tk->flag & (TKF_FREE)))
    {
	free(tk->val);
	if (copy == SM_KEEP) tk->flag&= ~TKF_FREE;
    }
    else if (copy != SM_KEEP)
	tk->flag|= TKF_FREE;

    tk->val= (void *)((copy == SM_COPY) ? strdup(val) : val);
    return (char *)(tk->val);
}

/* SETSN - Set the given system dictionary entry to string value, not to exceed
 * the first n characters.  This always makes a copy.
 */

char *setsn(int i, char *val, int n)
{
    Token *tk= &(sysdict[i].t);

    if ((tk->flag & (TKF_FREE)))
	free(tk->val);
    else
	tk->flag|= TKF_FREE;

    tk->val= (void *)strndup(val, n);
    return (char *)(tk->val);
}


/* SET_DICT - Change the given dictionary entry to this token.
 */

void set_dict(HashEntry *h, Token *t, int flag)
{
    /* May only change these when loading initializing file */
    if ((h->flag & (DT_PROTECT | DT_CONSTANT)) && !initializing)
	die("Cannot change protected variable %s",h->key);

    /* Deallocate memory for old string or array values */
    free_val(&(h->t));

    h->t= *t;
    h->flag|= flag;
}


/* ADD_DICT - insert a new symbol into a dictionary, resizing it if necessary.
 * Return a pointer to the new entry.
 */

HashEntry *add_dict(DictStack *s, char *label, Token *t, int flag)
{
    HashEntry *h;
    int isnew;

    h= CreateHashEntry(&(s->hash), label, &isnew);
    if (!isnew) free_val(&(h->t));
    h->flag= flag;
    h->t= *t;
    return h;
}


/* DEL_DICT - Remove the named symbol from the given dictionary.  Return
 * true if we succeeded.
 */

int del_dict(HashTable *t, char *label)
{
    HashEntry *h;

    if ((h= FindHashEntry(t, label)) == NULL)
	return 0;

    if (h->flag & DT_CONSTANT)
	die("cannot undefine constant variable %s", label);

    DeleteHashEntry(h);
    return 1;
}


/* SET_SYS_DICT - save a query variable.  First choice is to put it in an
 * existing system dictionary variable, second choice is to put it in the
 * user dictionary.  The value is always given as a string, but will be
 * converted to an integer, if appropriate.  If copy is true and the variable
 * is of a string type, new memory will be allocated for the variable and
 * copied in.
 *
 * If the variable is flagged multi-valued, the new value is appended to the
 * old value, with a comma separator, instead of replacing the old value.
 */

void set_sys_dict(char *label, char *val, int copy)
{
    HashEntry *h;

    /* Find the variable */
    if ((h= FindHashEntry(&syshash,label)) != NULL)
    {
	if ((h->flag & (DT_PROTECT | DT_CONSTANT)) && !initializing)
	    die("Attempt to redefine protected variable %s",label);
	if (class(h->t) == TKC_FUNCTION)
	    die("Attempt to redefine built-in function %s",label);
    }
    else if (dictstack == NULL)
	die("Cannot save query variable %s -- no user dictionary", label);
    else if ((h= FindHashEntry(&(dictstack->hash),label)) == NULL)
    {
	/* Create a new variable */
	Token tk;
	tk.flag= TK_STRING;
	tk.val= (void *)"";
	h= add_dict(dictstack, label, &tk, DT_MULTI);
    }

    /* Variable exists - store into it */
    if (class(h->t) == TKC_INTEGER)
    {
	/* Store value in existing integer variable */
	/* Maybe check integer syntax someday */
	h->t.val= (void *)atol(val);
    }
    else if (class(h->t) == TKC_TIME)
    {
	/* Store value in existing time variable */
	/* Really need an atotm() function */
	h->t.val= (void *)atol(val);
    }
    else if (class(h->t) == TKC_STRING)
    {
	/* Store value in existing string, procedure or symbol variable */
	if (h->flag & DT_MULTI && sval(h->t)[0] != '\0')
	{
	    /* MULTISTRING */
	    int ol= strlen((char *)h->t.val);
	    int nl= strlen(val);

	    if (h->t.flag & TKF_FREE)
	    {
		h->t.val= (void *)realloc(sval(h->t),ol+nl+2);
	    }
	    else
	    {
		char *m= (char *)malloc(ol+nl+2);
		strncpy(m, sval(h->t), ol);
		h->t.val= (void *)m;
	    }
	    sval(h->t)[ol]= ',';
	    strcpy(sval(h->t)+ol+1,val);

	    if (copy == SM_FREE) free(val);
	}
	else
	{
	    /* Regular string */
	    if (h->t.flag & TKF_FREE)
		free(sval(h->t));

	    if (copy == SM_COPY)
		h->t.val= (void *)strdup(val);
	    else
		h->t.val= (void *)val;

	    if (copy == SM_KEEP)
		h->t.flag&= ~TKF_FREE;
	    else
		h->t.flag|= TKF_FREE;
	}
    }
    else
	die("query sets variable %s which is not integer, string or time",
	    label);
}


/* FUNC_XDEF:  <value> <label> xdef -
 * Sets <label> to the given value in the current top dictionary.
 */

void func_xdef()
{
    Token label, value;

    pop_literal(&label);
    pop_any(&value);
    do_def(&label,&value,0);
}


/* FUNC_DEF:  <label> <value> def -
 * Sets <label> to the given value in the current top dictionary.
 */

void func_def()
{
    Token label, value;

    pop_any(&value);
    pop_literal(&label);
    do_def(&label,&value,0);
}

#if 1
/* FUNC_DEFCONSTANT:  <label> <value> defconstant -
 * Sets <label> to the given value in the current top dictionary.
 */

void func_defconstant()
{
    Token label,value;

    if (!initializing)
	die("Constants can only be defined to constant values");
    pop_any(&value);
    pop_literal(&label);
    do_def(&label,&value,DT_CONSTANT);
}


/* FUNC_XDEFCONSTANT:  <value> <label> xdefconstant -
 * Sets <label> to the given value in the current top dictionary.
 */

void func_xdefconstant()
{
    Token label,value;

    if (!initializing)
	die("Constants can only be defined to constant values");
    pop_literal(&label);
    pop_any(&value);
    do_def(&label,&value,DT_CONSTANT);
}
#else
void func_defconstant()
{
    die("Panic: defconstant command in compiled program");
}

void func_xdefconstant()
{
    die("Panic: xdefconstant command in compiled program");
}
#endif

void func_undefconstant()
{
    die("Panic: undefconstant command in compiled program");
}

void do_def(Token *label, Token *value, int flag)
{
    HashEntry *h;

    if (type(*label) == TK_UNBOUND_LITERAL)
    {
	/* Unbound symbols */
	label_ok(sval(*label));

	if ((h= FindHashEntry(&(dictstack->hash),sval(*label))) == NULL)
	    (void)add_dict(dictstack,sval(*label),value,flag);
	else
	    set_dict(h,value,flag);
	
	free_val(label);
    }
    else
	/* Bound symbols */
	set_dict(sysdict+ival(*label),value,flag);
}


/* FUNC_XSTORE:  <value> <label> xstore -
 * Sets <label> to the given value in the topmost dictionary where it is
 * defined, or in the current dictionary if it wasn't previously defined.
 */

void func_xstore()
{
    Token label, value;

    pop_literal(&label);
    pop_any(&value);
    do_store(&label,&value);
}


/* STORE_DICT - store the given token with as the given variable in the top
 * most dictionary where it is defined, or in the current dictionary if it
 * isn't defined.  If sysd is false, don't try to store it in the system
 * dictionary (used if we already know it isn't there).
 */

void store_dict(char *label, Token *value, int sysd)
{
    HashEntry *h;
    int i;

    if (sysd && (i= bind_sysdict(label)) >= 0)
    {
	set_dict(sysdict+i,value,0);
    } else if ((h= find_dictstack(label)) == NULL)
	(void)add_dict(dictstack,label,value,0);
    else
	set_dict(h,value,0);
}


/* FUNC_STORE:  <label> <value> store -
 * Sets <label> to the given value in the topmost dictionary where it is
 * defined, or in the current dictionary if it wasn't previously defined.
 */

void func_store()
{
    Token label, value;

    pop_any(&value);
    pop_literal(&label);
    do_store(&label,&value);
}

void do_store(Token *label, Token *value)
{
    if (type(*label) == TK_UNBOUND_LITERAL)
    {
	/* Unbound symbols */
	label_ok(sval(*label));
	store_dict(sval(*label),value, FALSE);
	    free_val(label);
    }
    else
	/* Bound symbols */
	set_dict(sysdict+ival(*label),value,0);
}

/* FUNC_UNDEF:  <label> undef -
 * Remove the given label from the topmost dictionary in which it is defined.
 * If it isn't defined, or is only defined in the system dictionary, this is
 * a no-op.
 */

void func_undef()
{
    DictStack *s;
    Token label;

    if (!pop_literal(&label))
	die("cannot undefine system dictionary symbol %s",
	    sysdict[ival(label)].key);

    for (s= dictstack; s != NULL; s= s->next)
    {
	if (del_dict(&(s->hash),sval(label)))
	    return;
    }
    free_val(&label);
}


/* FUNC_INC: <var> inc -
 * Increment an integer varible.   Equivalent to "<var> 1 + <var> xstore",
 * but faster.  It is an error if the variable is undefined or is not an
 * integer.
 */

void func_inc()
{
    Token var;
    HashEntry *h= NULL;

    if (!pop_literal(&var))
	/* Bound variable */
	h= sysdict + ival(var);
    else if ((h= find_dictstack(sval(var))) == NULL)
	die("attempt to increment undefined variable %s",sval(var));

    free_val(&var);

    if (type(h->t) != TK_INTEGER)
	die("attempt to increment non-integer variable %s",h->key);

    h->t.val= (void *)(ival(h->t)+1);
}


/* FUNC_DEC: <var> dec -
 * Decrement an integer varible.   Equivalent to "<var> 1 - <var> xstore",
 * but faster.  It is an error if the variable is undefined or is not an
 * integer.
 */

void func_dec()
{
    Token var;
    HashEntry *h= NULL;

    if (!pop_literal(&var))
	/* Bound variable */
	h= sysdict + ival(var);
    else if ((h= find_dictstack(sval(var))) == NULL)
	die("attempt to decrement undefined variable %s",sval(var));

    free_val(&var);

    if (type(h->t) != TK_INTEGER)
	die("attempt to decrement non-integer variable %s",h->key);

    h->t.val= (void *)(ival(h->t)-1);
}


/* FUNC_DEFINED:  <label> defined <bool>
 * This checks if a label is defined.  It simply pushes true or false on the
 * stack.
 */

void func_defined()
{
    Token label;

    /* If label is bound or defined, push a TRUE */
    push_integer(!pop_literal(&label) || get_dict(sval(label)) != NULL);
    free_val(&label);
}


/* FUNC_DEFAULT:  <label> <default-value> default <value>
 * This checks if a label is defined.  If it is, it pushes the value of the
 * label.  If it isn't it pushs the default value.
 */

void func_default()
{
    Token deflt, label, *value;

    pop_any(&deflt);
    if (pop_literal(&label))
    {
    	/* Unbound - Is it defined? */
    	if ((value= get_dict(sval(label))) == NULL)
	{
	    /* No, push back the default */
	    push_token(&deflt, FALSE);
	}
	else
	{
	    /* Yes, push the value */
	    free_val(&deflt);
	    push_token(value, TRUE);
	}
    }
    else
    {
    	/* Bound - push its value */
	free_val(&deflt);
	push_token(&sysdict[ival(label)].t, TRUE);
    }
    free_val(&label);
}


/* CSDICT - just like sdict(), but a function.  This is used by modules that
 * are shared with the partutil program.
 */

char *csdict(int i)
{
    return sdict(i);
}


/* FUNC_CONSTANT:  <label> constant <bool>
 * This checks if the given label is a defined constant.  It is always
 * resolved at compile time, so there is no code here.
 */

void func_constant() {}

/* FUNC_KNOWN:  <dict> <label> known <bool>
 * Check if a label is defined in a dictionary.  Label can be a literal or
 * a string.
 */

void func_known()
{
    char *key= pop_litname();
    HashTable *t= pop_dict();

    int rc= (FindHashEntry(t,key) != NULL);
    free(key);
    free_dict(t);
    push_integer(rc);
}
