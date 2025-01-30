/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Functions that work on lots of different types */

#include "backtalk.h"
#include "array.h"
#include "stack.h"
#include "free.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* FUNC_GET:   <array> <i> get <ith-element>
 *             <string> <i> get <ith-char>
 *             <dict> <key> get <value>
 * Get an element out of an array, string or dictionary.
 */

void func_get()
{
    Token source, selector;
    pop_any(&selector);
    pop_any(&source);

    if (class(source) == TKC_DICT)
    {
	char *key;
	HashEntry *h;

	if (type(selector) == TK_BOUND_LITERAL)
	    key= sysdict[ival(selector)].key;
	else if (type(selector) == TK_UNBOUND_LITERAL ||
		 type(selector) == TK_STRING)
	    key= sval(selector);
	else
	    die("Expected string or literal key value");

	h= FindHashEntry(hval(source),key);
	if (h == NULL) die("key not in dictionary");
	push_token(&(h->t),TRUE);
    }
    else
    {
	int i;

	if (type(selector) != TK_INTEGER)
	    die("Expected integer index");
	i= ival(selector);

	if (class(source) == TKC_ARRAY)
	{
	    Array *a= aval(source);
	    if (i < 0 || i >= a->sz)
		die("index out of range");
	    push_token(&(a->a[i]), TRUE);
	}

	else if (type(source) == TK_STRING)
	{
	    char *str= (char *)malloc(2);
	    if (i < 0 || i >= strlen(sval(source)))
		die("index out of range");
	    str[0]= sval(source)[i];
	    str[1]= '\0';
	    push_string(str,FALSE);
	}
	else
	    die("Expected source to be array, string or dictionary");
    }

    free_val(&source);
    free_val(&selector);
}


/* FUNC_PUT:   <array> <i> <any> put -
 *             <string> <i> <char or int> put -
 *             <dict> <key> <any> put -
 * Save a value to an element of an array.
 */

void func_put()
{
    Token t,dest,select;

    pop_any(&t);
    pop_any(&select);
    pop_any(&dest);

    if (class(dest) == TKC_DICT)
    {
	char *key;
	HashEntry *h;
	int isnew;

	if (type(select) == TK_BOUND_LITERAL)
	    key= sysdict[ival(select)].key;
	else if (type(select) == TK_UNBOUND_LITERAL ||
		 type(select) == TK_STRING)
	    key= sval(select);
	else
	    die("Expected string or literal key value");

	h= CreateHashEntry(hval(dest),key,&isnew);
	if (!isnew) free_val(&(h->t));
	h->t= t;
	h->flag= 0;
    }
    else
    {
	int i;
	int j;
	char *tmp;

	if (type(select) != TK_INTEGER)
	    die("Expected integer index");
	i= ival(select);

	if (class(dest) == TKC_ARRAY)
	{
	    Array *a= aval(dest);
	    if (i < 0) die("index out of range");

	    if (i >= a->sz)
	    {
		/* Grow array - filling in with empty string tokens */
		a->a= (Token *)realloc(a->a, (i + 1) * sizeof(Token));
		for (j= a->sz; j < i; j++)
		{
		   a->a[j].flag= TK_STRING|TKF_FREE;
		   a->a[j].val= (void *)malloc(1);
		   sval(a->a[j])[0]= '\0';
		}
		a->a[i]= t;
		a->sz= i + 1;
	    }
	    else
	    {
		/* Overwrite a value */
		free_val(&(a->a[i]));	/* Deallocate previous value */
		a->a[i]= t;
	    }
	}
#if 0
	/* This isn't going to work until we stop duplicating strings
	 * when we push them on the stack.
	 */
	else if (type(dest) == TK_STRING)
	{
	    if (i < 0 || i >= strlen(sval(dest)))
		die("Index out of range");
	    if (type(t) == TK_INTEGER)
	    {
		sval(dest)[i]= ival(t);
	    }
	    else if (type(t) == TK_STRING)
	    {
		sval(dest)[i]= sval(t)[0];
		free_val(&t);
	    }
	    else
		die("Value being put must be string or integer");
	}
#endif
    }
    free_val(&select);
    free_val(&dest);
}


/* FUNC_LENGTH: <string> length <len>
 * Find the length of a string or an array.
 */

void func_length()
{
    Token t;

    pop_any(&t);

    if (class(t) == TKC_STRING)
	push_integer(strlen(sval(t)));
    else if (class(t) == TKC_ARRAY)
	push_integer(aval(t)->sz);
    else if (class(t) == TKC_DICT)
	push_integer(hval(t)->numEntries);
    else
	die("Expected string or array type");

    free_val(&t);
}
