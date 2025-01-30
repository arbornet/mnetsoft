/*
 * Copyright (C) 2010  Cyberspace Communications, Inc.
 * All Rights Reserved
 *
 * $Id: genbsalt-freebsd.c 1006 2010-12-08 02:38:52Z cross $
 *
 * Generate a suitable salt for bcrypt.
 */

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char *enc64table =
	"./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static char *
enc64(char dst[24], unsigned char data[16])
{
	char *p;
	unsigned char *s;
	uint32_t shift;
	int i;

	p = dst;
	s = data;
	for (i = 0; i < 10; i++) {
		shift = s[0] << 16 | s[1] << 8 | s[2];
		s += 3;
		*p++ = enc64table[(shift >>  0) & 0x3F];
		*p++ = enc64table[(shift >>  6) & 0x3F];
		*p++ = enc64table[(shift >> 12) & 0x3F];
		*p++ = enc64table[(shift >> 18) & 0x3F];
	}
	*p = '\0';

	return(dst);
}

char *
bcrypt_gensalt(int rounds)
{
	static char salt[32];
	unsigned char data[16];
	char temp[24];

	memset(salt, '\0', sizeof(salt));
	snprintf(salt, sizeof(salt), "$2a$%02d$", rounds);
	arc4random_buf(data, sizeof(data));
	strlcat(salt, enc64(temp, data), sizeof(salt));

	return(salt);
}
