/*
 * Simple pager.
 */

#include <sys/types.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "libgrex.h"
#include "resh.h"

int rows = 24;
int columns = 80;
int ttyfd = -1;

static void
resize(void)
{
	struct winsize ws;

	rows = 24;
	if (ttyfd < 0)
		ttyfd = open("/dev/tty", O_RDWR);
	if (ttyfd < 0) {
		warning("can't open /dev/tty: %r");
		return;
	}
	if (!isatty(ttyfd)) {
		warning("pg: tty fd does not reference a tty?");
		return;
	}
	memset(&ws, 0, sizeof(ws));
	if (ioctl(ttyfd, TIOCGWINSZ, &ws) < 0) {
		warning("pg: ioctl(TIOCGWINSZ) failed: %r");
		return;
	}
	rows = ws.ws_row;
	columns = ws.ws_col;
}

static void
sigwinch(int signo)
{
	if (signo != SIGWINCH)
		return;
	resize();
}

static void
init(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigwinch;
	sigaction(SIGWINCH, &sa, NULL);
	resize();
}

void
page(FILE *fp)
{
	char buf[2048];
	int i, ch;
	const char *msg = "Press <space> to continue, 'q' to quit";

	for ( ; ; ) {
		for (i = 0; i < (rows - 1); i += ((strlen(buf) + columns - 1) / columns)) {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				return;
			fputs(buf, stdout);
		}
		write(ttyfd, msg, strlen(msg));
		fflush(stdout);
		ch = getch(ttyfd);
		printf("\r%-*s\r", strlen(msg), " ");
		if (ch == 'Q' || ch == 'q')
			break;
	}
}

int
main(int argc, char *argv[])
{
	FILE *fp;
	int fd;

	init();
	if (argc == 1)
		page(stdin);
	else while (*++argv) {
		if (STREQ(*argv, "-")) {
			page(stdin);
			continue;
		}
		fd = openr(*argv);
		if (fd < 0 || (fp = fdopen(fd, "rb")) == NULL) {
			warning("Opening %s failed: %r", *argv);
			continue;
		}
		page(fp);
		fclose(fp);
	}

	return(EXIT_SUCCESS);
}
