/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "token.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* TOKEN_TO_STRING - Given a token of any type, generate a string version of
 * it in newly malloc'ed memory.  Conversions work like this:
 *
 *  INTEGER - string form of number.
 *  TIME - ctime string.
 *  STRING - no change.
 *  LITERAL - name of literal.
 *  SYMBOLS - name of symbol.
 *  anything else - null string.
 */

char *token_to_string(Token *t)
{
    char *new;
    char buf[40];

   if (class(*t) == TKC_STRING)
   {
   	return strdup(sval(*t));
   }

   else if (type(*t) == TK_INTEGER)
   {
   	sprintf(buf,"%ld",ival(*t));
	return strdup(buf);
   }

   else if (type(*t) == TK_TIME)
   {
   	new= ctime((time_t *)&t->val);
	new[24]= '\0';
	return strdup(new);
   }

   else if (type(*t) == TK_BOUND_SYMBOL || type(*t) == TK_BOUND_LITERAL)
   {
	return strdup(sysdict[ival(*t)].key);
   }

   else if (class(*t) == TKC_ARRAY)
   {
	/* This isn't amazingly efficient, but it shouldn't be too common */
	Array *a= aval(*t);
	char **elem= (char **)malloc(a->sz * sizeof(char *));
	char *p, *q;
	int i, n= 0;

   	for (i= 0; i < a->sz; i++)
	{
	    elem[i]= token_to_string(&(a->a[i]));
	    n+= strlen(elem[i]);
	}
	q= new= (char *)malloc(n);
   	for (i= 0; i < a->sz; i++)
	{
	    for (p= elem[i]; *p != '\0'; p++)
		*(q++)= *p;
	    free(elem[i]);
	}
	*q= '\0';
	free(elem);
	return new;
   }

   else
   {
   	new= (char *)malloc(1);
	*new= '\0';
	return new;
   }
}
