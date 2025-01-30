/**
 * Copyright (C) 2010  Cyberspace Communications, Inc.
 * All Rights Reserved
 *
 * $Id: nuseradd-openbsd.c 1006 2010-12-08 02:38:52Z cross $
 *
 * @name nuseradd.c
 *
 * A wrapper around the useradd(8) utility to enable privilege separation.
 *
 * @memo Wrapper around useradd(8).
 */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <login_cap.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
	/*A_ECHO,*/
	A_ARGVZERO,
	A_M,
	/*A_F, A_F_TIME,*/
	A_B, A_BASEDIR,
	A_C, A_COMMENT,
	A_G, A_GID,
	A_K, A_SKELDIR,
	A_L, A_LOGINCLASS,
	A_P, A_PASSWORD,
	A_R, A_UIDRANGE,
	A_S, A_SHELL,
	A_USER,
	A_NULL,
	A_MAXARGS,

	MINUID = 100
};

const char *OPTIONS = "b:c:k:p:s:?";
const char *USERADD = "/usr/sbin/useradd";
/*const char *USERADD = "/bin/echo";*/
const char *RESH = "/usr/local/bin/resh";

extern char  *getvalue(const char *);
extern char **getlist(const char *);
static void   usage(char *);

static char *prog;
static gid_t egid;
static uid_t euid;

static void
privdrop(void)
{
	egid = getegid();
	euid = geteuid();
	setegid(getgid());
	seteuid(getuid());
}

static void
privget(void)
{
	seteuid(euid);
	setegid(egid);
}

static void
initargs(char *args[A_MAXARGS])
{
	memset(args, 0, A_MAXARGS * sizeof(char *));
	/*args[A_ECHO] = "user";*/
	args[A_ARGVZERO] = "useradd";
	args[A_M] = "-m";
	/*args[A_F] = "-f";*/
	args[A_B] = "-b";
	args[A_C] = "-c";
	args[A_G] = "-g";
	args[A_K] = "-k";
	args[A_L] = "-L";
	args[A_P] = "-p";
	args[A_R] = "-r";
	args[A_S] = "-s";
}

static int
valid_group(char *group)
{
	gid_t	gid;
	char	*e;

	if (getgrnam(group) != NULL)
		return(1);
	e = NULL;
	gid = strtol(group, &e, 0);
	if ((e != NULL && *e != '\0') || gid <= 0)
		return(0);
	return(getgrgid(gid) != NULL);
}

static int
valid_loginclass(char *class)
{
	if (class == NULL || *class == '\0')
		return(1);
	return(login_getclass(class) != NULL);
}

static void
free_basedirs(char **basedirs)
{
	char	**dirp;

	for (dirp = basedirs; *dirp != NULL; dirp++)
		free(*dirp);
	free(basedirs);
}

static int
valid_basedir(char *basedir)
{
	char	**basedirs, **dirp;
	char	realbase[PATH_MAX];

	realpath(basedir, realbase);
	basedirs = getlist("userdev");
	if (basedirs == NULL)
		return 0;
	for (dirp = basedirs; *dirp != NULL; dirp++)
		if (strncmp(realbase, *dirp, strlen(*dirp)) == 0) {
			free_basedirs(basedirs);
			return(1);
		}
	free_basedirs(basedirs);
	return(0);
}

static int
valid_shell(char *shell)
{
	char	*s;

	if (strcmp(shell, RESH) == 0)
		return(1);
	setusershell();
	while ((s = getusershell()) != NULL)
		if (strcmp(s, shell) == 0) {
			endusershell();
			return(1);
		}
	endusershell();
	return(0);
}

static int
valid_user(char *username)
{
	char	*p;

	if (username == NULL)
		return(0);
	if (strlen(username) > 30)
		return(0);
	if (!isascii(*username) || !isalpha(*username) || !islower(*username))
		return(0);
	for (p = username; *p != '\0'; p++)
		if (!isascii(*p) || !isprint(*p) || !(isalnum(*p) || *p == '_'))
			return(0);
	return(1);
}

static int
valid_gecos(char *gecos)
{
	char	*p;

	if (gecos == NULL)
		return(0);
	for (p = gecos; *p != '\0'; p++)
		if (!isascii(*p) || !isprint(*p) ||
		    (!isalnum(*p) && *p != '.' && *p != ' ' && *p != '_'))
			return(0);
	return(1);
}

static int
valid_password(char *password)
{
	return(strchr(password, ':') == NULL);
}

static int
valid_uidrange(char *uidrange)
{
	uid_t min, max;
	char *s, *p, *minstr, *maxstr;

	s = strdup(uidrange);
	if (s == NULL)
		return(0);
	minstr = s;
	p = strchr(minstr, '.');
	if (p == NULL) {
		free(s);
		return(0);
	}
	*p++ = '\0';
	if (*p != '.') {
		free(s);
		return(0);
	}
	*p++ = '\0';
	maxstr = p;

	p = NULL;
	min = strtol(minstr, &p, 0);
	if (p != NULL && *p != '\0') {
		free(s);
		return(0);
	}
	p = NULL;
	max = strtol(maxstr, &p, 0);
	if (p != NULL && *p != '\0') {
		free(s);
		return(0);
	}
	free(s);

	if (min < MINUID)
		return(0);
	if (max > UID_MAX)
		return(0);

	return(1);
}

static int
baditem(const char *what, char *conf, int (*valid)(char *), char *val)
{
	if (val == NULL || *val == '\0') {
		fprintf(stderr, "Missing or empty %s!%s\n", what, conf);
		return(1);
	}
	if (valid != NULL && !valid(val)) {
		fprintf(stderr, "Bad %s: %s%s\n", what, val, conf);
		return(1);
	}
	return(0);
}

static void
checkargs(char *args[A_MAXARGS])
{
	int	errs;

	errs = 0;
	errs += baditem("username", "", valid_user, args[A_USER]);
	errs += baditem("group", " (Check 'group' in conf file)",
		    valid_group, args[A_GID]);
	errs += baditem("base directory", " (check 'userdev' in conf file)",
		    valid_basedir, args[A_BASEDIR]);
	errs += baditem("comment", "", valid_gecos, args[A_COMMENT]);
	errs += baditem("login class", " (Check 'loginclass' in conf file)",
		    valid_loginclass, args[A_LOGINCLASS]);
	errs += baditem("password", "", valid_password, args[A_PASSWORD]);
	errs += baditem("shell", "", valid_shell, args[A_SHELL]);
	errs += baditem("UID range", " (Check 'uidrange' in conf file)",
		    valid_uidrange, args[A_UIDRANGE]);
	errs += baditem("Skeleton directory", "", NULL, args[A_SKELDIR]);

	if (errs != 0)
		usage(prog);
}

int
main(int argc, char *argv[])
{
	char	*args[A_MAXARGS], *env[] = { "PATH=/bin:/usr/sbin", NULL };
	int	ch, oerrno;

	/*
	 * First things first; get rid of our privileges until we need them
	 * and initialize everything.
	 */
	privdrop();
	prog = basename(argv[0]);
	initargs(args);

	/*
	 * Get configuration values from the Perl interpreter.
	 */
	/*args[A_F_TIME] = "1";*/
	args[A_GID] = getvalue("group");
	args[A_UIDRANGE] = getvalue("uidrange");
	args[A_LOGINCLASS] = getvalue("loginclass");

	if (args[A_LOGINCLASS] == NULL)
		args[A_LOGINCLASS] = strdup("default");

	/*
	 * Now, process command line arguments.
	 */
	while ((ch = getopt(argc, argv, OPTIONS)) != -1) {
		switch (ch) {
		case 'b':
			args[A_BASEDIR] = optarg;
			break;
		case 'c':
			args[A_COMMENT] = optarg;
			break;
		case 'k':
			args[A_SKELDIR] = optarg;
			break;
		case 'p':
			args[A_PASSWORD] = optarg;
			break;
		case 's':
			args[A_SHELL] = optarg;
			break;
		case '?':	/* FALLTHROUGH */
		default:
			usage(prog);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage(prog);
	args[A_USER] = argv[0];
	args[A_NULL] = NULL;

	/*
	 * Now, check args. Note: Login class is a special case; it
	 * may be empty.
	 */
	checkargs(args);

	if (strcmp(args[A_LOGINCLASS], "default") == 0) {
		free(args[A_LOGINCLASS]);
		args[A_LOGINCLASS] = strdup("");
		if (args[A_LOGINCLASS] == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
	}

	/* get our privileges back and invoke useradd.  */
	privget();
	execve(USERADD, args, env);

	/*
	 * If we get here, something went wrong.
	 * Drop privs and print an error.
	 */
	oerrno = errno;
	setgid(getgid());
	setuid(getuid());
	fprintf(stderr, "Running %s failed: %s\n", USERADD, strerror(oerrno));

	free(args[A_GID]);
	free(args[A_UIDRANGE]);
	free(args[A_LOGINCLASS]);

	return(EXIT_FAILURE);
}

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s -b <basedir> -c '<comment>' "
	    "-k <skeldir> -p '<hashed password>' -s <shell>.\n", prog);
	exit(EXIT_FAILURE);
}
