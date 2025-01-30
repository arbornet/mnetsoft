/*
 * Utility functions; this opens only "regular" files.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "libgrex.h"
#include "resh.h"

int
openr(const char *filename)
{
	int fd, oerrno;
	struct stat s;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return(fd);
	if (fstat(fd, &s) < 0) {
		oerrno = errno;
		close(fd);
		errno = oerrno;
		return(-1);
	}
	if ((s.st_mode & S_IFMT) != S_IFREG) {
		close(fd);
		errno = EINVAL;
		return(-1);
	}
	if (s.st_size > 65536) {
		close(fd);
		errno = EINVAL;
		return(-1);
	}

	return(fd);
}

int
spawn(int argc, char *argv[])
{
	pid_t pid;
	int status;

	status = 0;
	pid = vfork();
	if (pid < 0) {
		warning("vfork failed: %r");
		return(EXIT_FAILURE);
	}
	if (pid == 0) {
		execvp(argv[0], argv);
		warning("%s failed: %r", argv[0]);
		_exit(EXIT_FAILURE);
	} else {
		waitpid(pid, &status, 0);
	}

	return(status);
}

int
getch(int fd)
{
	char buf[16];
	ssize_t n;
	struct termios ttysave;
	struct termios raw;

	memset(&ttysave, 0, sizeof(ttysave));
	memset(&raw, 0, sizeof(raw));
	tcgetattr(fd, &ttysave);
	tcgetattr(fd, &raw);
	cfmakeraw(&raw);
	tcsetattr(fd, TCSAFLUSH, &raw);
	n = read(fd, buf, sizeof(buf));
	tcsetattr(fd, TCSAFLUSH, &ttysave);
	if (n <= 0)
		return(-1);
	if (n > 1)
		return(-n);
	return(buf[0]);
}
