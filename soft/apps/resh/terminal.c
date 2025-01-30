/*
 * Terminal handling code in C.
 */

#include <stddef.h>
#include <stdlib.h>
#include <termios.h>

int
termget(struct termios *tp)
{
	return(tcgetattr(0, tp));
}

int
termset(struct termios *tp)
{
	return(tcsetattr(0, TCSAFLUSH, tp));
}

int
termraw(struct termios *tp)
{
	struct termios raw;
	int r;

	memset(&raw, 0, sizeof(raw));
	termget(&raw);
	memset(tp, 0, sizeof(*tp));
	termget(tp);
	r = cfmakeraw(&raw);
	termset(&raw);

	return(r);
}

int
getch(void)
{
	char buf[16];
	ssize_t n;
	struct termios ttysave;

	termraw(&ttysave);
	tcflush(0, TCIOFLUSH);
	n = read(0, buf, sizeof(buf));
	termset(&ttysave);
	if (n <= 0)
		return(-1);
	if (n > 1)
		return(-n);
	return(buf[0]);
}

#include <stdio.h>
#include <ctype.h>
int
main(void)
{
	int ch;
	char vis[4];

	printf("Enter a character: ");
	fflush(stdout);
	ch = getch();
	snprintf(vis, sizeof vis, "%c", ch);
	if (iscntrl(ch))
		snprintf(vis, sizeof vis, "^%c", (ch & 0x7f) + '@');
	printf("\nYou entered: %02x (%s)\n", ch, vis);

	return(EXIT_SUCCESS);
}
