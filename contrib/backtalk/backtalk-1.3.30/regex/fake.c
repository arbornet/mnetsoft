#include "common.h"

#ifndef HAVE_MEMMOVE
/*
 - memmove - fake ANSI C routine
 */
char *re_memmove(dst, src, count)
char *dst;
char *src;
size_t count;
{
	register char *s;
	register char *d;
	register size_t n;

	if (dst > src)
		for (d = dst+count, s = src+count, n = count; n > 0; n--)
			*--d = *--s;
	else
		for (d = dst, s = src, n = count; n > 0; n--)
			*d++ = *s++;

	return(dst);
}
#endif


#ifndef HAVE_MEMCMP
int re_memcmp(char *a, char *b, int n)
{
    unsigned char ca,cb;

    while (n-- > 0)
    {
	ca= *((unsigned char *)a);
	cb= *((unsigned char *)b);
	a++; b++;
	if (ca != cb)
	    return ca - cb;
    }
    return 0;
}
#endif


#ifndef HAVE_MEMSET
char *re_memset(char *dst, int c, int n)
{
    unsigned char *p= dst;
    for ( ; n > 0; n--)
	*(p++)= (unsigned char)c;
    return dst;
}
#endif
