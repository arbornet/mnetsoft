/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include "str.h"
#include "cgi_cookie.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* GET_COOKIE - Given a cookie name, return it's value, or NULL if it is not
 * defined.  This gets cookies out of the HTTP_COOKIE environment variable
 * The returned value is in malloc'ed memory, and is the caller's responsiblity
 * to free.
 */

char *get_cookie(char *name)
{
    char *env, *b, *s, *e;
    int nlen;

    if ((env= getenv("HTTP_COOKIE")) == NULL)
	return NULL;

    nlen= strlen(name);

    for (;;)
    {
	/* b -> start of next cookie name */
	if (*(b= firstout(env," \n\r")) == '\0')
	    return NULL;

	/* s -> equal sign ending cookie name */
	if ((s= strchr(b,'=')) == NULL)
	    return NULL;

	/* e -> first character after end of cookie value */
	e= firstin(s+1,";\n\r");

	if (nlen == s-b && !strncmp(b,name,nlen))
	{
	    char *v;

	    v= (char *)malloc(e-s);
	    strncpy(v,s+1,e-s-1);
	    v[e-s-1]= '\0';
	    return v;
	}

	if (*e != ';')
	    return NULL;

	env= e + 1;
    }
}
