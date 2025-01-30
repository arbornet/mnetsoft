/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* This defines functions used in the SQL query code. */

#include "common.h"

#ifdef USE_SQL

#include "str.h"
#include "qry.h"
#include "sql.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* MAKEQRY - Create a query buffer and descriptor.  We allocate to the given
 * size.
 */

void makeqry(struct sqlbuf *b)
{
    b->bf= (char *)malloc(BFSZ);
    b->bf[0]= '\0';
    b->sz= BFSZ;
    b->i= 0;
}


/* GROWQRY - Enlarge an existing query buffer so its size is as given.
 */

void growqry(struct sqlbuf *b, int sz)
{
    b->bf= (char *)realloc(b->bf, sz);
    b->sz= sz;
}


/* ADDQRY - This is a sprintf()-like function that prints to the end of the
 * query buffer, growing it as needed.
 */

#if __STDC__
void addqry(struct sqlbuf *b, const char *fmt, ...)
#else
void addqry(b, fmt, va_alist)
    struct sqlbuf *b;
    const char *fmt;
    va_dcl
#endif
{
    va_list ap;
    int left, n;

    for (;;)
    {
	if ((left= b->sz - b->i) <= 1)
	{
	    growqry(b, b->i+BFSZ);
	    left= b->sz - b->i;
	}

	VA_START(ap,fmt);
	n= vsnprintf(b->bf + b->i, left, fmt, ap);
	if (n == -1)
	    growqry(b, b->i+BFSZ);
	else if (n > left-1)
	    growqry(b, n+1);
	else
	{
	    b->i+= n;
	    return;
	}
    }
}


/* ADDQUOTEQRY - Add the given string to the end of the query, doing an
 * sql_quote() on it first.
 */

void addquoteqry(struct sqlbuf *b, char *str)
{
    int need= (str == NULL) ? 4 : 2*strlen(str)+3;
    if (b->i + need >= b->sz)
    	growqry(b, b->i + need + 100);
    sql_quote(str, b->bf + b->i);
    b->i+= strlen(b->bf + b->i);
}


/* FINALQRY - Return the result.
 */

char *finalqry(struct sqlbuf *b)
{
    return b->bf;
}


/* RESETQRY - Empty, but do not deallocate the query buffer.
 */

void resetqry(struct sqlbuf *b)
{
    b->i= 0;
}


/* FREEQRY - Deallocate the query buffer.
 */

void freeqry(struct sqlbuf *b)
{
    free(b->bf);
    b->bf= NULL;
    b->sz= b->i= 0;
}

#endif /* USE_SQL */
