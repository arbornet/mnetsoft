/*
 * Do challenge and response reboots.
 * Hopefully, robocop will run enough that we can actually do this if
 * necessary; even if the system cannot fork, is the victim of a fork
 * bomb, or what have you.  This is our last-ditch effort to revive
 * Grex remotely.
 *
 * $Id: nprint.c 1089 2011-02-28 06:35:53Z cross $
 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
nwrite(int sd, char *buf, size_t buflen)
{
	char *b;
	int n, w;

	n = buflen;
	b = buf;
	while (n > 0) {
		w = write(sd, b, n);
		if (w < 0)
			return(-1);
		n -= w;
		b += w;
	}

	return(0);
}

int
nswrite(int sd, char *buf, size_t len)
{
	char lenbuf[64];
	int l;

	l = snprintf(lenbuf, sizeof(lenbuf), "%ld:", len);
	if (nwrite(sd, lenbuf, l) < 0)
		return(-1);
	if (nwrite(sd, buf, len) < 0)
		return(-1);
	if (nwrite(sd, ",\r\n", strlen(",\r\n")) < 0)
		return(-1);
	return(0);
}

int
nprint(int sd, const char *fmt, ...)
{
	va_list ap;
	char buf[128];
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	if (n >= sizeof(buf))
		return(-1);

	return(nswrite(sd, buf, n));
}

int
nread(int sd, char *buf, int buflen)
{
	char *b, s, rbuf[64];
	int n, r;

	b = rbuf;
	n = 0;
	memset(rbuf, '\0', sizeof(rbuf));
	memset(buf, '\0', buflen);
	for (b = rbuf, n = 0; n < (sizeof(rbuf) - 1); b++, n++) {
		if (read(sd, b, 1) != 1)
			return(-1);
		if (*b == ':')
			break;
	}
	if (*b != ':')
		return(-1);
	*b = '\0';
	b = NULL;
	n = strtol(rbuf, &b, 10);
	if (b != NULL && *b != '\0')
		return(-1);
	if (n < 0 || buflen <= n)
		return(-1);
	b = buf;
	while (n > 0) {
		r = read(sd, b, n);
		if (r <= 0)
			return(-1);
		b += r;
		n -= r;
	}
	*b = '\0';
	if (read(sd, &s, 1) != 1 || s != ',')
		return(-1);
	if (read(sd, &s, 1) != 1 && (s != '\n' || s != '\r'))
		return(-1);
	if (s == '\n')	/* Okay if it's just a newline. */
		return(0);
	if (read(sd, &s, 1) != 1 && s != '\n')
		return(-1);
	return(0);
}
