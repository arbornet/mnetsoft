/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "printtoken.h"
#include "sysdict.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* PRINT_TOKEN - Print the a token, in input format so we can see its type.
 */

void print_token(FILE *fp, Token *t)
{

    if (type(*t) == TK_INTEGER) 
	fprintf(fp,"%ld",ival(*t));

    else if (type(*t) == TK_TIME) 
    {
	/* There isn't really an input form for times, so fudge it */
	char *tm= ctime((time_t *)&t->val);
	tm[7]= tm[10]= tm[19]= '_';
	tm[24]= '\0';
	fputs(tm+4,fp);
    }

    else if (type(*t) == TK_UNBOUND_LITERAL)
	fprintf(fp,"/%s",sval(*t));
    else if (type(*t) == TK_BOUND_LITERAL) 
	fprintf(fp,"/%s",sysdict[ival(*t)].key);

    else if (type(*t) == TK_UNBOUND_SYMBOL)
	fputs(sval(*t),fp);
    else if (type(*t) == TK_BOUND_SYMBOL) 
	fputs(sysdict[ival(*t)].key,fp);

    else if (type(*t) == TK_MARK)
	fprintf(fp,"MARK[%d]",ival(*t));

    else if (type(*t) == TK_FUNCTION)
	fputs(function_name(t->val),fp);

    else if (type(*t) == TK_DYNAMIC)
	fprintf(fp,"%s",sval(*t));

    else if (type(*t) == TK_REGEX)
	fputs(ttymode?"<<regex>>":"&lt;&lt;regex&gt;&gt;",fp);

    else if (type(*t) == TK_STRING)
    {
	char *cp;
	fputc('(',fp);
	for(cp= sval(*t); *cp != '\0'; cp++)
	    switch (*cp)
	    {
	    case '\n': fputs("\\n",fp); break;
	    case '\b': fputs("\\b",fp); break;
	    case '\t': fputs("\\t",fp); break;
	    case '\\': fputs("\\\\",fp); break;
	    case '(':  fputs("\\(",fp); break;
	    case ')':  fputs("\\)",fp); break;
	    case '<':  fputs(ttymode ? "<" : "&lt;",fp); break;
	    case '>':  fputs(ttymode ? ">" : "&gt;",fp); break;
	    case '&':  fputs(ttymode ? "&" : "&amp;",fp); break;
	    default:   fputc(*cp,fp); break;
	    }
	fputc(')',fp);
    }
    else if (type(*t) == TK_PROCEDURE || type(*t) == TK_ARRAY)
    {
	int i;
	fputc((type(*t) == TK_ARRAY) ? '[' : '{' ,fp);
	for (i= 0; i < aval(*t)->sz; i++)
	{
	    if (i != 0) fputc(' ',fp);
	    print_token(fp,&(aval(*t)->a[i]));
	}
	fputc((type(*t) == TK_ARRAY) ? ']' : '}' ,fp);
    }
    else if (type(*t) == TK_DICT)
    {
	fputs("DICTIONARY",fp);
    }
    else
    {
	fputs("UNPRINTABLE",fp);
    }
}


/* FUNCTION_NAME - Return the name of a built-in function.
 */

char *function_name(void *func)
{
    int i;

    for (i= 0; i < SYSDICTLEN; i++)
	if (sysdict[i].t.val == func && sysdict[i].t.flag & TK_FUNCTION)
	    return sysdict[i].key;
    return "(function)";
}
