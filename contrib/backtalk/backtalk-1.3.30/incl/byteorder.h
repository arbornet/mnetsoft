/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

extern int big_endian;
void check_endian(void);

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_HTONL
#include <netinet/in.h>
#else
unsigned int swap32(unsigned int x);

unsigned short int swap16(unsigned short int x);

#define htonl(x) (big_endian ? (x) : swap32(x))
#define htons(x) (big_endian ? (x) : swap16(x))
#define ntohl(x) (big_endian ? (x) : swap32(x))
#define ntohs(x) (big_endian ? (x) : swap16(x))
#endif
