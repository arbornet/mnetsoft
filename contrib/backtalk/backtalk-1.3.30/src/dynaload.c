/* Copyright 2000, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#ifdef DYNALOAD
#include "dynaload.h"
#include <dlfcn.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifndef RTLD_LAZY		/* Linux, Solaris, AIX, SGI */
# ifdef DL_LAZY
#  define RTLD_LAZY DL_LAZY	/* OpenBSD */
# else
#  define RTLD_LAZY 1		/* SunOS */
# endif
#endif

/* Table of loadable libraries */
struct dyna_tab {
    char *filename;
    void *handle;
    } dynatab[N_LIB]=
    {
	/* { DYNALIB_DIR LIB_USRADM_NAME DYNALIB_SUFFIX, NULL }, */
	/* { DYNALIB_DIR LIB_GRPADM_NAME DYNALIB_SUFFIX, NULL }, */
	{ DYNALIB_DIR LIB_CNFADM_NAME DYNALIB_SUFFIX, NULL },
    };

/* DYNALOAD
 *  Given a TK_DYNAMIC token, get a function name from the string value,
 *  and a library ID from the line value, and load that library (if it isn't
 *  already loaded) and bind that symbol.  Then edit the token to turn it
 *  into a TK_FUNCTION token pointing to the new function and return a pointer
 *  to the function.
 *
 *  Ordinary t points to an entry in the system dictionary, and so the
 *  conversion to a TK_FUNCTION token means that this routine will be called
 *  no more than once for any function.
 */

void (*dynaload(Token *t))(void)
{
    int lib= t->line;
    void *dlh;
    void (*func)();
    char *err;

    /* Link in the library, if it isn't already in */
    if (dynatab[lib].handle == NULL)
    {
	if (!(dynatab[lib].handle= dlopen(dynatab[lib].filename, RTLD_LAZY)))
	    die("Could not load dynamic library %s: %s",
	    	dynatab[lib].filename, dlerror());
    }

    /* Find the address of the function */
    func= (void (*)())dlsym(dynatab[lib].handle,sval(*t));
    if ((err= dlerror()) != NULL)
    	die("Could not find %s in dynamic library %s: %s", 
	    sval(*t), dynatab[lib].filename, err);

    /* Convert the token to a TK_FUNCTION token */
    t->line= 0;
    t->flag= TK_FUNCTION;
    t->val= (void *)func;

    return func;
}

#endif /*DYNALOAD*/
