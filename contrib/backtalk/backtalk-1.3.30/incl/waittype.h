/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#if defined(HAVE_SYS_WAIT_H) || defined (HAVE_UNION_WAIT)
#include <sys/wait.h>
#endif

#ifdef HAVE_UNION_WAIT

/* This is for older Unixs where wait() returns status as a "union wait" */

typedef union wait waittype;

#ifndef WTERMSIG
#define TERMSIG(x) ((x).w_termsig)
#endif

#ifndef WIFEXITED
#define WIFEXITED(x) ((x).w_stopval != 0177 && WTERMSIG(x) == 0)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(x) ((x).w_retcode)
#endif

#else

/* in Modern Unixs return wait() returns status as a int */

typedef int waittype;

#ifndef WTERMSIG
#define WTERMSIG(x) ((x) & 0x7f)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(x) (((x) >> 8) & 0xff)
#endif

#ifndef WIFEXITED
#define WIFEXITED (WTERMSIG (x) == 0)
#endif
#endif /* WAITUNION */
