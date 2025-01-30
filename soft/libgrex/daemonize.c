/**
 * @name daemonize
 * @param dir Directory to execute in.
 * @param pidfile Filename to write our PID into.
 * @see fork(2), setsid(2), close(2), open(2), dup(2)
 * @memo Become a daemon.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libgrex.h"

void
daemonize(char *dir, char *pidfile)
{
	pid_t	pid;
	long	fd, maxfd;
	char	buf[64];

	if (dir == NULL)
		dir = "/";
	if (pidfile == NULL)
		pidfile = "/dev/null";

	pid = fork();
	if (pid < 0)
		fatal("daemonize: Can't fork: %r.");
	if (pid > 0)
		_exit(EXIT_SUCCESS);
	setsid();
	pid = fork();
	if (pid < 0)
		fatal("daemonize: Can't fork: %r.");
	if (pid > 0)
		_exit(EXIT_SUCCESS);
	umask(0);
	if (chdir(dir) < 0)
		fatal("daemonize: Can't chdir to %s: %r.");
	maxfd = sysconf(_SC_OPEN_MAX);
	for (fd = 0; fd < maxfd; fd++)
		close(fd);
	open("/dev/null", O_RDONLY);	/* FILENO_STDIN */
	open("/dev/null", O_WRONLY);	/* FILENO_STDOUT */
	open("/dev/null", O_WRONLY);	/* FILENO_STDERR */
	fd = open(pidfile, 0644, O_CREAT | O_WRONLY);
	if (fd < 0)
		return;
	snprintf(buf, sizeof(buf), "%ld\n", (unsigned long)pid);
	write(fd, buf, strlen(buf));
	close(fd);
}
