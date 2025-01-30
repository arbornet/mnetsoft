/*
 * Copyright (C) 2010  Cyberspace Communications, Inc.
 * All Rights Reserved
 *
 * $Id: setquota.c 1006 2010-12-08 02:38:52Z cross $
 *
 * setquota.c - set a user's quota based on values given on the command
 * line.  Used by pnewuser.
 */

#include <sys/types.h>
#include "sysquota.h"

#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ".\n");
	exit(EXIT_FAILURE);
}

u_int64_t
estrtou64(const char *str)
{
	long long r;
	char *e;

	e = NULL;
	errno = 0;
	r = strtoll(str, &e, 0);
	if ((e != NULL && *e != '\0') || (r == 0 && errno == EINVAL) ||
	    ((r == LLONG_MIN || r == LLONG_MAX) && errno == ERANGE))
		fatal("Poorly formatted unsigned integer \"%s\"", str);

	return(r);
}

u_int32_t
estrtou32(const char *str)
{
	long r;
	char *e;

	e = NULL;
	errno = 0;
	r = strtol(str, &e, 0);
	if ((e != NULL && *e != '\0') || (r == 0 && errno == EINVAL) ||
	    ((r == LONG_MIN || r == LONG_MAX) && errno == ERANGE))
		fatal("Poorly formatted unsigned integer \"%s\"", str);

	return(r);
}

void
setquota(const char *device, uid_t user,
    u_int64_t hard, u_int64_t soft,
    u_int32_t ihard, u_int32_t isoft)
{
	struct dqblk q;
	int cmd;

	memset(&q, 0, sizeof(q));
	cmd = QCMD(Q_GETQUOTA, USRQUOTA);
	if (quotactl(device, cmd, (int)user, (char *)&q) < 0)
		perror("quotactl(Q_GETQUOTA, USRQUOTA) failed");
	q.dqb_bhardlimit = hard;
	q.dqb_bsoftlimit = soft;
	q.dqb_ihardlimit = ihard;
	q.dqb_isoftlimit = isoft;
	cmd = QCMD(Q_SETQUOTA, USRQUOTA);
	if (quotactl(device, cmd, (int)user, (char *)&q) < 0)
		perror("quotactl(Q_SETQUOTA, USRQUOTA) failed");
}

void
dosetquota(char *argv[6])
{
	char *user, *device;
	struct passwd *pw;
	u_int64_t hard, soft;
	u_int32_t ihard, isoft;

	user = argv[0];
	device = argv[1];
	hard = estrtou64(argv[2]);
	soft = estrtou64(argv[3]);
	ihard = estrtou32(argv[4]);
	isoft = estrtou32(argv[5]);
	pw = getpwnam(user);
	if (pw == NULL)
		fatal("Unknown user %s", user);
	setquota(device, pw->pw_uid, hard, soft, ihard, isoft);
}

int
parseline(char *line, char *argv[6])
{
	int argc;
	char **vp, *s;

	argc = 0;
	vp = argv;
	s = line;
	while (argc < 6 && (*vp = strsep(&s, " \t\n\r\v\f")) != NULL) {
		if (**vp != '\0') {
			vp++;
			argc++;	
		}
	}
	return(argc == 6 && *s == '\0');
}

int
main(int argc, char *argv[])
{
	char *args[6], line[256];

	while (fgets(line, sizeof(line), stdin) != NULL) {
		if (!parseline(line, args)) {
			fprintf(stderr, "Error parsing ling: %s\n", line);
			continue;
		}
		dosetquota(args);
	}

	return(EXIT_SUCCESS);
}
