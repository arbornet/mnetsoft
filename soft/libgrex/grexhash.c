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
/*
 *  Modified by Dan Cross for use with LibreSSL/OpenSSL.
 */

#include <sys/types.h>
#include <openssl/sha.h>

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

/* eventually, this should go in a separate header file: */
typedef uint32_t kg_key[5];

/*
 * kg_realmofhost.  return the "domain" of the host in question.
 *
 *  This is not the final real thing.  When a realm file is supported,
 *  we'll want to default to everything past the first '.' in the
 *  hostname & use the realm file to flatten things out...
 */
#define FOREIGN(a, b, c) ((strlen(a) - c) == 2 && (strlen(b) - c) <= 6)

static int
kg_realmofhost(char *hostname, char *realm, int rsize)
{
	char *s1, *s2, *s3, *s4;
	int c;

	s2 = hostname;
	s3 = s4 = 0;
	c = 0;
	for (s1 = hostname; (s1 = strchr(s1, '.')) != NULL; ++s1) {
		if (!s1[1]) {
			c = 1;
			break;
		}
		s4 = s3, s3 = s2, s2 = s1 + 1;
	}
	if (!s3) {
		return 1;	/* XXX */
	}
	if (FOREIGN(s2, s3, c) && !s4) {	/* XXX */
		return 1;
	}
	if (!s4 || !FOREIGN(s2, s3, c))
		s4 = s3;
	if (!s4) {
		return 1;	/* XXX */
	}
	s2 = 0;
	s3 = realm;
	if (c)
		++rsize;
	while ((c = *s4++) != '\0') {
		if (--rsize <= 0)
			return 1;
		s2 = (c == '.') ? s3 : 0;
		*s3++ = (islower(c) ? toupper(c) : c);
	}
	if (s2)
		*s2 = 0;
	else
		*s3 = 0;

	return 0;
}

/*
 *	kg_localrealm
 *
 *	return the realm of the local host.
 */
static int
kg_localrealm(char *lrealm, int lsize)
{
	char foo[512];
#ifndef	GREXHASHREALM
	if (gethostname(foo, sizeof foo) < 0)
		return 1;
#else
	snprintf(foo, sizeof(foo), "%s", GREXHASHREALM);
#endif
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
static int
kg_string_to_key(char *clear, char *user, char *instance, char *realm, kg_key key)
{
	SHA_CTX si;
	int len, i;
	char pad[128];

	/* compute:
	 *      hmac(k, M) = shs(k2 || shs(k1 || M))
	 *      where
	 *              k1 = ipad || k
	 *              k2 = k || opad
	 *              M = user || instance || realm
	 * inspired by Adam Back <aba@dcs.ex.ac.uk>'s
	 * description of HMACs, in coderpunks list mail,
	 * 24 April 1997.  Adam's description is based
	 * on http://www.research.ibm.com/security/bck2.ps
	 * (Bellare, Canetti, Krawczyk. _Keying Hash Functions
	 * for Message Authentication_. IBM June 1996).
	 * We use a different method to derive k1 & k2.
	 */
	len = strlen(clear) + 1;
	for (i = 0; i < 128; ++i)
		pad[i] = '\\';
	SHA1_Init(&si);
	SHA1_Update(&si, pad, 128 - (len & 63));
	SHA1_Update(&si, clear, len);
	SHA1_Update(&si, user, strlen(user) + 1);
	SHA1_Update(&si, instance, strlen(instance) + 1);
	SHA1_Update(&si, realm, strlen(realm) + 1);
	SHA1_Final((unsigned char *)key, &si);
	for (i = 0; i < 128; ++i)
		pad[i] = '6';
	SHA1_Init(&si);
	SHA1_Update(&si, clear, len);
	SHA1_Update(&si, pad, 128 - (len & 63));
	SHA1_Update(&si, key, SHA_DIGEST_LENGTH);
	SHA1_Final((unsigned char *)key, &si);
	memset(&si, 0, sizeof si);

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
static int
kg_bin_to_str(unsigned char *key, int ksize, char *str, int strsize)
{
	unsigned char *temp, *up;
	unsigned r;
	int i;
	int size, hlen;
	unsigned char ktemp[128];
	char *rp;

	/* 81799.97 = ((ln(256)/ln(85)) * 65536) */
	hlen = 1 + (((ksize * 81800) + 65535) >> 16);
	if (hlen > strsize) {
		*str = 0;
		return 1;	/* XXX */
	}
	if (ksize + hlen < sizeof ktemp) {
		temp = ktemp;
	} else {
		temp = malloc(ksize + hlen);
	}
	memmove(key, temp, ksize);
	rp = (char *)temp + (ksize + hlen);
	*--rp = 0;
	for (int nz = 1, size = 0; size < hlen; ++size) {
		for (r = 0, up = temp, i = 0; i < ksize; ++i, ++up) {
			nz |= *up;
			r <<= 8;
			r += *up;
			*up = r / HBASE;
			r %= HBASE;
		}
#if 0
		if (!nz)
			break;
#endif
		r += 0x21;	/* '!' */
		if (r >= 0x2c)
			++r;	/* ',' */
		if (r >= 0x3a)
			++r;	/* ':' */
		if (r >= 0x3b)
			++r;	/* ';' */
		if (r >= 0x5c)
			++r;	/* '\\' */
		*--rp = r;
		nz = 0;
	}
	strlcpy(str, rp, strsize);
	memset(temp, 0, ksize + hlen);
	if (temp != ktemp)
		free(temp);
	return 0;
}

#ifdef	WANT_KG_STR_TO_BIN

/*
 *	kg_str_to_bin
 *
 *	Convert an "ascii text" representation back
 *	to the original binary data.
 */
static int
kg_str_to_bin(char *str, unsigned char *key, int ksize)
{
	int i;
	unsigned char *up;
	unsigned r;

	for (i = 0; i < ksize; ++i)
		key[i] = 0;
	while ((r = *str++) != '\0') {
		if (r == 0x5c)
			return 1;	/* '\\' */
		if (r > 0x5c)
			--r;
		if (r == 0x3b)
			return 1;	/* ';' */
		if (r == 0x3a)
			return 1;	/* ':' */
		if (r > 0x3a)
			r -= 2;
		if (r == 0x2c)
			return 1;	/* ',' */
		if (r > 0x2c)
			--r;
		r -= 0x21;	/* '!' */
		if (r >= HBASE)
			return 1;	/* XXX */
		for ((up = key + ksize), i = ksize; --up, --i >= 0;) {
			r += (*up * (unsigned)HBASE);
			*up = r;
			r >>= 8;
		}
		if (r)
			return 1;	/* XXX */
	}
	return 0;
}

#endif				/* WANT_KG_STR_TO_BIN */

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
static char *
kg_pwhash(char *clear, char *user, char result[HLEN + 3], int rsize)
{
	int i;
	kg_key key;
	char realm[64];

	if (rsize < 3)
		return 0;
	i = kg_localrealm(realm, sizeof realm);
	if (i)
		return 0;
	kg_string_to_key(clear, user, "", realm, key);
	strlcpy(result, "%!", rsize);
	i = kg_bin_to_str((unsigned char *)key, sizeof key, result + 2, rsize - 2);
	memset(key, 0, sizeof key);
	if (i)
		return 0;
	return result;
}

/**
 * @name grexhash_r
 *
 * Return the grexhash of the given password for the given user,
 * storing the return value in supplied buffer `res', of length
 * `reslen'.  Returns NULL if the buffer is too small.
 *
 * @memo Return the Grex hash of the password supplied for the given user.
 */
char *
grexhash_r(char *pass, char *user, char *res, size_t reslen)
{
	if (reslen < HLEN)
		return NULL;
	return kg_pwhash(pass, user, res, reslen);
}
