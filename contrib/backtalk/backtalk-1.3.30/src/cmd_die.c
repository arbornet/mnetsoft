/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Utility routines for command line programs. */

#include "common.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* DIE - Exit with an error condition.  Arguments are like printf(), specifying
 * a message to print before exiting.
 */

#if __STDC__
void die(const char *fmt, ...)
#else
void die(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
    va_list ap;

    VA_START(ap,fmt);
    vfprintf(stderr, fmt, ap);
    putc('\n',stderr);
    va_end(ap);
    exit(1);
}
