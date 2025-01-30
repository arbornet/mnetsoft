/*
 *	mkp2.c
 *
 * Grex string to key function.
 * this is NOT part of the JFH shadow code distribution.
 */
/*
 * Copyright (c) 1995 Marcus D. Watts  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Marcus D. Watts.
 * 4. The name of the developer may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * MARCUS D. WATTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "shs.h"
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
extern char *malloc();
#if HAS_MEMSET
#define bzero(s,c)	memset(s,0,c)
#define bcopy(a,b,c)	memcpy(b,a,c)
#endif
#ifdef MKP2

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$Id: mkp2.c,v 1.1 2008-09-04 02:25:58 cross Exp $";
#endif /* LIBC_SCCS and not lint */


/* eventually, this should go in a separate header file: */
typedef long kg_key[5];

/*
 * kg_realmofhost.  return the "domain" of the host in question.
 *
 *  This is not the final real thing.  When a realm file is supported,
 *  we'll want to default to everything past the first '.' in the
 *  hostname & use the realm file to flatten things out...
 */

kg_realmofhost(hostname, realm, rsize)
	char *hostname, *realm;
{
	char *s1, *s2, *s3, *s4;
	int c;

	s2 = hostname;
	s3 = s4 = 0;
	c = 0;
	for (s1 = hostname; s1 = strchr(s1, '.'); ++s1)
	{
		if (!s1[1])
		{
			c = 1;
			break;
		}
		s4 = s3, s3 = s2, s2 = s1+1;
	}
#if 0
printf ("s4 = <%s> s3 = <%s> s2=<%s>", s4, s3, s2);
#endif
	if (!s3)
	{
#if 0
printf (" no s3 failed\n");
#endif
		return 1;	/* XXX */
	}
#define FOREIGN ((strlen(s2)-c) == 2 && (strlen(s3)-c) <= 6)
	if (FOREIGN && !s4)	/* XXX */
	{
#if 0
printf (" foreign & no s4 failed\n");
#endif
		return 1;
	}
	if (!s4 || !FOREIGN)
		s4 = s3;
	if (!s4)
	{
#if 0
printf (" no s4 failed\n");
#endif
		return 1;	/* XXX */
	}
	s2 = 0;
	s3 = realm;
	if (c) ++rsize;
	while (c = *s4++)
	{
		if (--rsize <= 0)
			return 1;
		s2 = (c == '.') ? s3 : 0;
		*s3++ = (islower(c) ? toupper(c) : c);
	}
	if (s2) *s2 = 0;
	else *s3 = 0;
#if 0
printf (" result <%s>\n", realm);
#endif
	return 0;
}

/*
 *	kg_localrealm
 *
 *	return the realm of the local host.
 */

kg_localrealm(lrealm, lsize)
	char *lrealm;
{
	char foo[512];
	if (gethostname(foo, sizeof foo) < 0)
		return 1;
	return kg_realmofhost(foo, lrealm, lsize);
}

/*
 * kg_string_to_key
 *
 * use sha to compute the shs hash of:
 *	password NUL user NUL instance NUL realm NUL
 * actually, now computes the hmac of the user + instance
 *	+ realm using the password as the key.
 *
 * the output key is 160 bits (20 bytes, 5 longs.), output
 *	in network byte order.
 *
 * the user, instance, & realm constitute a "salt".
 *  This salt is intended to make attacks difficult.
 *  The attacks this guards against are dictionary attacks
 *  (because every user has his own different keyspace),
 *  and a compromise of one cell (because the keys
 *  will be different in every cell.)
 */

int kg_string_to_key(clear, user, instance, realm, key)
	char *clear, *user, *instance, *realm;
	kg_key key;
{
	SHS_INFO si[1];
#if 0
	shsInit(si);
	shsUpdate(si, clear, strlen(clear)+1);
	shsUpdate(si, user, strlen(user)+1);
	shsUpdate(si, instance, strlen(instance)+1);
	shsUpdate(si, realm, strlen(realm)+1);
	shsFinal(si);
#else
	int len, i;
	char pad[128];

	/* compute:
	 *	hmac(k, M) = shs(k2 || shs(k1 || M))
	 *	where
	 *		k1 = ipad || k
	 *		k2 = k || opad
	 *		M = user || instance || realm
	 * inspired by Adam Back <aba@dcs.ex.ac.uk>'s
	 * description of HMACs, in coderpunks list mail,
	 * 24 April 1997.  Adam's description is based
	 * on http://www.research.ibm.com/security/bck2.ps
	 * (Bellare, Canetti, Krawczyk. _Keying Hash Functions
	 * for Message Authentication_. IBM June 1996).
	 * We use a different method to derive k1 & k2.
	 */
	len = strlen(clear)+1;
	for (i = 0; i < 128; ++i) pad[i] = 0x5c;
	shsInit(si);
	shsUpdate(si, pad, 128-(len&63));
	shsUpdate(si, clear, len);
	shsUpdate(si, user, strlen(user)+1);
	shsUpdate(si, instance, strlen(instance)+1);
	shsUpdate(si, realm, strlen(realm)+1);
	shsFinal(si);
	bcopy(shsDigest(si), key, SHS_DIGESTSIZE);
	for (i = 0; i < 128; ++i) pad[i] = 0x36;
	shsInit(si);
	shsUpdate(si, clear, len);
	shsUpdate(si, pad, 128-(len&63));
	shsUpdate(si, key, SHS_DIGESTSIZE);
	shsFinal(si);
#endif
	bcopy(shsDigest(si), key, SHS_DIGESTSIZE);
	bzero(si, sizeof si);
	return 0;
}

#define HBASE 85
#define HLEN 26

/*
 *	kg_bin_to_str
 *
 *	Convert a binary data object to an "ascii text"
 *	representation (base 85).  The result is one long string
 *	of nearly all the printable characters (space, comma and
 *	colon are excluded for use in the password file & other
 *	such uses.)
 *
 *	returns 1 on failure (no room, etc.)
 */

kg_bin_to_str(key, ksize, str, strsize)
	unsigned char *key;
	char *str;
{
	unsigned char *temp, *up;
	unsigned r;
	int i;
	int size, nz, hlen;
	unsigned char ktemp[128];
	char *rp;

	/* 81799.97 = ((ln(256)/ln(85)) * 65536) */
	hlen = 1 + (((ksize * 81800) + 65535) >> 16);
	if (hlen > strsize)
	{
		*str = 0;
		return 1;	/* XXX */
	}
	if (ksize+hlen < sizeof ktemp)
	{
		temp = ktemp;
	} else {
		temp = malloc(ksize+hlen);
	}
	bcopy(key, temp, ksize);
	rp = ((char*) (temp)) + (ksize+hlen);
	*--rp = 0;
	for (nz = 1, size = 0;size < hlen;++size)
	{
		for (r = 0, up = temp, i = 0; i < ksize; ++i, ++up)
		{
			nz |= *up;
			r <<= 8;
			r += *up;
			*up = r / HBASE;
			r %= HBASE;
		}
#if 0
		if (!nz) break;
#endif
#if 0
printf ("r = %ld (%c)\n", r, r + 0x21);
#endif
		r += 0x21;	/* '!' */
		if (r >= 0x2c) ++r;	/* ',' */
		if (r >= 0x3a) ++r;	/* ':' */
		if (r >= 0x3b) ++r;	/* ';' */
		if (r >= 0x5c) ++r;	/* '\\' */
		*--rp = r;
		nz = 0;
	}
	strcpy(str, rp);
	bzero(temp, ksize+hlen);
	if (temp != ktemp)
		free(temp);
	return 0;
}

/*
 *	kg_str_to_bin
 *
 *	Convert an "ascii text" representation back
 *	to the original binary data.
 */

kg_str_to_bin(str, key, ksize)
	unsigned char *key;
	char *str;
{
	int c, i;
	unsigned char *up;
	unsigned r;

#if 0
printf ("kg_str_to_bin: str=%#lx %d<%s> key=%#lx\n",
str, strlen(str), str, key);
#endif
	for (i = 0; i < ksize; ++i)
		key[i] = 0;
	while (r = *str++)
	{
#if 0
printf ("Working on %d (%c)\n", r, *str);
#endif
		if (r == 0x5c) return 1;	/* '\\' */
		if (r > 0x5c) --r;
		if (r == 0x3b) return 1;	/* ';' */
		if (r == 0x3a) return 1;	/* ':' */
		if (r > 0x3a) r -= 2;
		if (r == 0x2c) return 1;	/* ',' */
		if (r > 0x2c) --r;
		r -= 0x21;			/* '!' */
		if (r >= HBASE)
			return 1;	/* XXX */
		for ((up = key+ksize), i = ksize; --up, --i >= 0; )
		{
			r += (*up * (unsigned) HBASE);
#if 0
printf ("Storing %x at %#lx\n", r, up);
#endif
			*up = r;
			r >>= 8;
		}
		if (r) return 1;	/* XXX */
	}
	return 0;
}

/*
 *	kg_pwhash
 *
 *	generate an "encrypted" password for the local
 *	machine, given the user & cleartext password.
 *
 *	the result must be "large enough" (HLEN+3, or currently
 *	29 characters).
 *
 *	the result is prefixed with "%!" as a tag for
 *	the *kind* of password generated.  The Unix "crypt"
 *	routine & variants, which use base-64, will never
 *	output a '!' in any position.
 *
 *	Update.  We now output '%!'.  JFH uses a prefix
 *	of '!' to disable accounts.
 */

char *kg_pwhash(clear, user, result, rsize)
	char *clear, *user;
	char result[HLEN+3];
{
	int i;
	kg_key key;
	char realm[64];

	if (rsize < 3) return 0;
	i = kg_localrealm(realm, sizeof realm);
	if (i)
		return 0;
	kg_string_to_key(clear, user, "", realm, key);
	strcpy(result, "%!");
	i = kg_bin_to_str((unsigned char*) key, sizeof key, result+2, rsize-2);
	bzero(key, sizeof key);
	if (i) return 0;
	return result;
}
#endif
