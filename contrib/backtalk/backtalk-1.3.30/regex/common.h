#include <sys/types.h>
#include "../incl/config.h"
#include "../incl/byteorder.h"

#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#else
void *malloc(), *realloc();
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#define	_POSIX2_RE_DUP_MAX	255
#define	CHAR_MIN	(-128)
#define	CHAR_MAX	127
#define	CHAR_BIT	8
#endif

#ifndef HAVE_MEMMOVE
#define memmove(x,y,n) re_memmove((char *)(x), (char *)(y), (int)n)
#endif

#ifndef HAVE_MEMCMP
#define memcmp(x,y,n) re_memcmp((char *)(x), (char *)(y), (int)n)
#endif

#ifndef HAVE_MEMSET
#define memset(x,c,n) re_memset((char *)(x), c, n)
#endif
