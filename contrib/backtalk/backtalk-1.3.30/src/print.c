/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "dict.h"
#include "print.h"
#include "sysdict.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* PRINT_VALUE - Print the value of a token, without giving any clue to its
 * type.  The contents of arrays and procedures are printed recursively.
 * Marks and regular expressions are not printed at all.
 */

void print_value(FILE *fp, Token *t)
{
    Token *val;

    if (type(*t) == TK_INTEGER) 
	fprintf(fp,"%ld",ival(*t));
    else if (type(*t) == TK_TIME)
    {
	char *tm= ctime((time_t *)&t->val);
	tm[24]= '\0';
	fputs(tm,fp);
    }
    else if (type(*t) == TK_BOUND_SYMBOL)
	fputs(sysdict[ival(*t)].key,fp);
    else if (type(*t) == TK_BOUND_LITERAL) 
	print_value(fp,&sysdict[ival(*t)].t);
    else if (type(*t) == TK_UNBOUND_LITERAL) 
    {
	if ((val= get_dict(sval(*t))) != NULL)
	    print_value(fp,val);
    }
    else if (class(*t) == TKC_STRING)
	fputs(sval(*t),fp);
    else if (class(*t) == TKC_ARRAY)
    {
	int i;
	for (i= 0; i < aval(*t)->sz; i++)
	    print_value(fp,&(aval(*t)->a[i]));
    }
}
