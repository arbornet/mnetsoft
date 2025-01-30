#include <sys/file.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>

#include "zapuser.h"

char *log_file = NULL;
char *err_file = NULL;

/*
 * ERROR Print and log an error message.
 */

void
error(char *fmt,...)
{
	va_list ap;
	FILE *fp;
	time_t tm;
	uid_t uid;

	va_start(ap, fmt);

	if (err_file != NULL) {
		uid = geteuid();
		seteuid(0);
		if ((fp = fopen(err_file, "a")) == NULL) {
			fprintf(stderr, "Cannot write to error file %s\n", err_file);
			err_file = NULL;
			print_errors = 1;
		} else {
			tm = time(0L);
			flock(fileno(fp), LOCK_EX);
			fprintf(fp, "%20.20s: ", ctime(&tm) + 4);
			if (curr_user != NULL)
				fprintf(fp, "error deleting %s - ", curr_user);
			vfprintf(fp, fmt, ap);
			putc('\n', fp);
			fclose(fp);
		}
		seteuid(uid);
	}
	if (print_errors) {
		if (curr_user != NULL)
			fprintf(stderr, "error deleting %s - ", curr_user);
		vfprintf(stderr, fmt, ap);
		putc('\n', stderr);
	}
	va_end(ap);
}

/*
 * LOG Log a deletion.  The passwd file line and reason are passed in.
 */

void
zulog(char *pwline, char *why)
{
	static int log_fd = -1;
	time_t tm;
	char *bf;
	size_t len;

	if (log_file == NULL)
		return;

	if (log_fd < 0) {
		if ((log_fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, 0600)) < 0) {
			error("cannot open log file '%s' (%s) - not logging", log_file,
			      strerr(errno));
			log_file = NULL;
			return;
		}
	}
	len = strlen(pwline) + strlen(why) + 100;
	bf = malloc(len);
	tm = time(0L);
	snprintf(bf, len, "%20.20s %.30s [%s] %s\n",
		 ctime(&tm) + 4, myid, pwline,
		 why != NULL ? why : "-");

	/* Use atomic write call, so locking is not needed */
	write(log_fd, bf, strlen(bf));
}
