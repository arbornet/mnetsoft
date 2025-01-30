/*
 * warning.c -- Error reporting functions.
 *
 * This file contains routines warning() and fatal(), for printing
 * warning and error messages.  In addition to error reporting,
 * fatal() will call exit() with EXIT_FAILURE.  These routines take
 * printf()-style `format' strings, extended in such a way that
 * the new `%r' verb will be interpolated with the contents of the
 * system error string corresponding to the current value of the
 * global integer `errno.'
 *
 * $Id: warning.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libgrex.h"

static void	fmterror(char *, va_list);


/**
 * @name fatal
 * @param fmt printf(3) like format specifier.
 * @returns fatal() doesn't return.
 *
 * Note that if `fmt' contains the string "%r", this will be
 * interpolated with error string corresponding to the current
 * value of the global `errno'.
 *
 * @memo Print an error message and exit with a failure status.
 */
void
fatal(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	fmterror(fmt, ap);
	va_end(ap);

	exit(EXIT_FAILURE);
}

/**
 * @name warning
 * @param fmt printf(3) like format specifier.
 * @returns void
 *
 * Note that if `fmt' contains the string "%r", this will be
 * interpolated with error string corresponding to the current
 * value of the global `errno'.
 *
 * @memo Print an error message.
 */
void
warning(char *fmt, ...)
{
	va_list	ap;

	va_start(ap, fmt);
	fmterror(fmt, ap);
	va_end(ap);
}

/*
 * This routine is complex.  The idea is that we try to do the best
 * job we can of printing out an error or a warning, regardless of
 * what errors we may encounter while trying to do so.  The big reason
 * for complexity is that we try to interpolate the value of the
 * current system error string into the format using a new format
 * verb, "%r", which requires modifying the format string prior to
 * calling fprintf().  We attempt to do this in a 1K buffer on the
 * stack, which should be adequate for most needs, and should also
 * succeed even in the face of most errors.  If 1K is too small,
 * however, we try to allocate memory on the heap, which might fail.
 * If it does, we make a attempt to print out the message and return.
 *
 * This does add a fair amount of complexity, but at least it's all
 * trapped in one routine.
 */
static void
fmterror(char *fmt, va_list ap)
{
	size_t	l;
	char	*a, *b, *e, *f, *p, *t, buf[1024];

	e = strerror(errno);
	a = NULL;
	f = buf;
	l = strlen(fmt) + 1;
	if (l > sizeof(buf)) {
		a = malloc(l);
		if (a == NULL) {
			perror("fmterror: malloc failed");
			vfprintf(stderr, fmt, ap);
			return;
		}
		f = a;
	}
	strlcpy(f, fmt, l);
	while ((p = strstr(f, "%r")) != NULL) {
		l = strlen(f) - 2 + strlen(e) + 1;
		t = buf;
		if (l > sizeof(buf)) {
			t = (a == NULL) ? malloc(l) : realloc(f, l);
			if (t == NULL) {
				perror("fmterror: realloc failed");
				vfprintf(stderr, f, ap);
				free(f);
				return;
			}
			if (a == NULL) {
				a = t;
				strlcpy(a, buf, l);
			}
		}
		b = t + strlen(f) - 2 + strlen(e) - strlen(p + 2);
		memmove(b, p + 2, strlen(f) - (p - f) - 2 + 1);
		memmove(t + (p - f), e, strlen(e));
		f = t;
	}
	if (libgrex_program_name__ != NULL && *libgrex_program_name__ != '\0')
		fprintf(stderr, "%s: ", libgrex_program_name__);
	vfprintf(stderr, f, ap);
	fprintf(stderr, "\n");
	if (a != NULL)
		free(f);
}
