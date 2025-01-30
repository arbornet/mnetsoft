/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "byteorder.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* Is this machine big_endian? */
#ifdef __BYTE_ORDER
int big_endian= (__BYTE_ORDER == __BIG_ENDIAN);
#else
#ifdef BYTE_ORDER
int big_endian= (BYTE_ORDER == BIG_ENDIAN);
#else
int big_endian;
#define TEST_ENDIAN
#endif
#endif


void check_endian()
{
#ifdef TEST_ENDIAN
    int one= 1;

    /* Figure out if this is a big_endian machine */
    big_endian= !(*((char *)(&one)));
#endif /* TEST_ENDIAN */
}

#ifndef HAVE_HTONL
unsigned int swap32(unsigned int x)
{
    unsigned int y;

    ((char *)(&y))[3]= ((char *)(&x))[0];
    ((char *)(&y))[2]= ((char *)(&x))[1];
    ((char *)(&y))[1]= ((char *)(&x))[2];
    ((char *)(&y))[0]= ((char *)(&x))[3];
    return y;
}

unsigned short int swap16(unsigned short int x)
{
    unsigned short int y;

    ((char *)(&y))[1]= ((char *)(&x))[0];
    ((char *)(&y))[0]= ((char *)(&x))[1];
    return y;
}
#endif
