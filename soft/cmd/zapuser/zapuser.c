#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "zapuser.h"

int print_errors = 1;		/* Should we print error messages? */
int verbose = 0;		/* Should we talk alot? */
int protect_members = 1;	/* Can we delete members? */

int min_uid = 500;		/* Minimum uid we may zap */

char *curr_user = NULL;		/* User we are currently working on */
char *myid = NULL;		/* Who am I? */

struct udat *user = NULL;	/* List of users to delete */
int nuser = 0;			/* Number of valid entries in user list */
int usersz = 0;			/* Memory allocated to user list */

FILE *ttyr = NULL;		/* /dev/tty open for reading */


/*
 * INSERT_USER Add another user to the user list.  Do realloc stuff to allow
 * it to grow arbitrarily.  Initially only the login name is saved - other
 * info will be filled in later by check_user().
 */

void
insert_user(char *login)
{
	if (login == NULL)
		return;

	if (nuser >= usersz) {
		usersz = nuser + 512;
		user = (struct udat *) Realloc(user, usersz * sizeof(struct udat));
		if (user == NULL) {
			fprintf(stderr, "out of memory for user list\n");
			exit(1);
		}
	}
	user[nuser++].login = login;
}


/*
 * SORT_USERS Sort the user list data structure
 */

int
ud_comp(const void *aa, const void *bb)
{
	const struct udat *a = aa;
	const struct udat *b = bb;

	/* Nulls sort to end of list */
	if (a->login == NULL)
		return 1;
	if (b->login == NULL)
		return -1;
	return strcmp(a->login, b->login);
}

void
sort_users()
{
	qsort((void *) user, nuser, sizeof(struct udat), &ud_comp);
}

/*
 * CHECK_DELETED Check each user in the user list to see if they still exist.
 * If they do, set dir to NULL to indicate that we shouldn't delete them.
 */

void
check_deleted()
{
	char bf[BFSZ];
	FILE *fp;
	int ch, i = 0, j;

	if (verbose > 0)
		printf("scanning passwd to see if deletions worked\n");
	if ((fp = fopen("/etc/passwd", "r")) == NULL) {
		error("cannot open /etc/passwd");
		exit(1);
	}
	while ((ch = fgetc(fp)) != EOF) {
		if (i < 0) {
			if (ch == '\n')
				i = 0;
		} else if (ch == ':') {
			bf[i] = '\0';
			if ((j = in_user_list(bf)) >= 0) {
				error("deletion of %s from passwd file failed - "
				      "not deleting files", user[j].login);
				free(user[j].home);
				user[j].home = NULL;
			}
			i = -1;
		} else if (i < BFSZ) {
			bf[i++] = ch;
		} else {
			i = -1;
		}
	}
	fclose(fp);
}


/*
 * IN_USER_LIST Return -1 if the named user is not in the user list,
 * otherwise return the index.
 */

int
in_user_list(char *u)
{				/* Binary Search Version */
	int c, i;
	int lo = 0;
	int hi = nuser - 1;

	while (lo <= hi) {
		i = (lo + hi) / 2;
		if ((c = (user[i].login == NULL ? -1 : strcmp(u, user[i].login))) == 0)
			return i;
		else if (c < 0)
			hi = i - 1;
		else
			lo = i + 1;
	}
	return -1;
}


/*
 * CHECK_USER Check if a user is OK to delete, and stash other data about him
 * in the user list data structure.  If confirm true, prompt on stderr for
 * confirmation.  Returns true if we should not delete the named user.
 */

int
check_user(struct udat * user, int confirm)
{
	struct passwd *pw;
	char bf[BFSZ];
	int ans, ch;

	curr_user = user->login;

	/* User exists? */
	if (verbose > 1)
		printf("checking that user %s exists\n", user->login);
	if ((pw = getpwnam(user->login)) == NULL) {
		error("no such user in passwd file");
		return 1;
	}
	/* No uid's under 1000 */
	if (verbose > 1)
		printf("checking uid\n");
	if (pw->pw_uid < min_uid) {
		error("cannot delete account with uid below %d", min_uid);
		return 1;
	}
	/* Unless -m flag was given, protect members and staff */
	if (verbose > 1)
		printf("checking group membership\n");
	if (protect_members && is_member(user->login, pw->pw_gid)) {
		error("user is member or staff and no -m flag given");
		return 1;
	}
	/* Home directory must start with /a/, /c/, /d/, etc.  */
	if (verbose > 1)
		printf("checking directory\n");
	if (!dir_ok(pw->pw_dir)) {
		error("home directory %s fails sanity checks", pw->pw_dir);
		return 1;
	}
	/* Check for immortality */
	if (verbose > 1)
		printf("checking immortality\n");
	if (is_immortal(user->login)) {
		error("user %s is immortal", user->login);
		return 1;
	}
	/*
	 * Under Marcus's hacked SunOS there was a getmailfilename() routine
	 * defined in libshadow) to get mailbox name.  We haven't even
	 * implemented hierarchical mail on OpenBSD yet, so we don't have
	 * that.  For now we are just calling a dummy routine.
	 */
	if (!getmailfilename(pw, "@/etc/mailfmt", bf, BFSZ)) {
		error("could not figure out mailbox name");
		return 1;
	}
	user->mailbox = strdup(bf);

	if (confirm) {
		/* Describe the user a bit */
		fprintf(stderr, "User: %s\nFull Name: %s\nHome: %s\n",
			user->login, pw->pw_gecos, pw->pw_dir);

		/* Ask if we should delete him */
		fprintf(stderr, "OK to Delete %s? ", user->login);
		fflush(stderr);
		if ((ans = fgetc(ttyr)) != '\n' && ans != EOF)
			while ((ch = fgetc(ttyr)) != '\n' && ch != EOF);
		if (ans != 'y' && ans != 'Y')
			return 1;
	}
	/* Passed all sanity checks - save information */
	if (verbose > 1)
		printf("ok to delete\n");
	user->uid = pw->pw_uid;
	user->home = strdup(pw->pw_dir);
	snprintf(bf, BFSZ, "%.64s:%.50s:%d:%d:%.50s:%ld:%ld:%.200s:%.50s:%.50s",
		 user->login,
		 pw->pw_passwd,
		 pw->pw_uid,
		 pw->pw_gid,
		 pw->pw_class,
		 pw->pw_change,
		 pw->pw_expire,
		 pw->pw_gecos,
		 pw->pw_dir,
		 pw->pw_shell);
	user->line = strdup(bf);

	if (user->mailbox == NULL || user->home == NULL || user->line == NULL) {
		fprintf(stderr, "out of memory for user info");
		exit(1);
	}
	return 0;
}


/*
 * RM_FILE Delete a file if it exists.  File name is given sprintf-style.
 * Error message is printed if the file exists and could not be deleted.
 * Format should be given so it cannot overflow a BFSZ buffer.  Desc is a
 * short description to use in error message.
 */

void
rm_file(char *desc, char *fmt,...)
{
	va_list ap;
	char bf[BFSZ];

	va_start(ap, fmt);
	vsnprintf(bf, BFSZ, fmt, ap);
	if (unlink(bf) && errno != ENOENT)
		error("deletion of %s file %s failed (%s)", desc, bf, strerr(errno));
}


/*
 * DELETE_FILES Delete all the files belonging to the user.  If delete is not
 * true, just move them aside instead.
 */

void
delete_files(struct udat * user, int delete)
{
	int i;
	char *deldir;

	curr_user = user->login;

	/* Delete various files that might be around */
	if (delete) {
		if (verbose > 1)
			printf("removing mailbox %.500s\n", user->mailbox);
		rm_file("mailbox", "%.500s", user->mailbox);
	}
	if (verbose > 1)
		printf("removing any screen file\n");
	rm_file("screen", "/tmp/screens/S-%.500s", user->login);
	if (verbose > 1)
		printf("removing any layer file\n");
	rm_file("layer", "/tmp/layers/%.500s", user->login);

	if (delete) {
		/* Delete home directory */
		if (verbose > 1)
			printf("deleting home %s\n", user->home);
		delete_directory(user->uid, user->home);
	} else {
		/* Move home directory and mail */
		if (verbose > 1)
			printf("moving home %s\n", user->home);
		if ((deldir = move_directory(user->home)) != NULL) {
			if (verbose > 1)
				printf("moving mailbox\n");
			move_mail(user->mailbox, deldir);
			free(deldir);
		}
	}
	if (verbose > 1)
		printf("user deleted\n");
}


/*
 * FREE_USER Deallocate memory being used to store info about the given user
 */

void
free_user(struct udat * user)
{
	free(user->home);
	free(user->line);
	free(user->mailbox);
	/* Don't free user->login - it isn't always malloced memory */
}

/*
 * ZAP_PASSWD Delete all users in the userlist from the master.passwd file.
 * This is openbsd specific.
 */

void
zap_passwd()
{
	char buf[8192];
	char *p;
	int masterfd, tempfd;
	FILE *masterfp, *tempfp;
	int i;
	off_t filesize;
	struct stat st;

	/* Disable core dumps, disable most signals, do some initialization */
	pw_init();

	/*
	 * useradd does a flock() lock on the master file before creating the
	 * temp lock file, but nothing else does.  Let's not.
	 */

	/* Create temp file and lock it */
	for (i = 1; (tempfd = pw_lock(0)) < 0; i++) {
		if (errno != EEXIST) {
			error("Could not create " _PATH_MASTERPASSWD_LOCK ": %s",
			      strerror(errno));
			return;
		}
		if (i == 4 && verbose > 0)
			fputs("Attempting to lock password file..", stderr);
		if (i % 16 == 4)
			fputc('.', stderr);
		usleep(250000);
	}
	if (i > 4 && verbose > 0)
		fputs("Locked\n", stderr);
	if ((tempfp = fdopen(tempfd, "w")) == NULL) {
		error("Could not fdopen " _PATH_MASTERPASSWD_LOCK ": %s",
		      strerror(errno));
		pw_abort();
		return;
	}
	/* Open master passwd to read */
	masterfd = open(_PATH_MASTERPASSWD, O_RDONLY, 0);
	if (masterfd < 0 || fcntl(masterfd, F_SETFD, 1) == -1) {
		pw_abort();
		error("Cannot open " _PATH_MASTERPASSWD " to read");
		return;
	}
	if ((masterfp = fdopen(masterfd, "r")) == NULL) {
		pw_abort();
		error("Could not fdopen " _PATH_MASTERPASSWD);
		return;
	}
	/* Copy master into temp lock file, dropping users in the userlist */
	filesize = 0L;
	while (fgets(buf, sizeof(buf), masterfp) != NULL) {
		if (strchr(buf, '\n') == NULL) {
			pw_abort();
			error(_PATH_MASTERPASSWD " line too long:\n%.20s...", buf);
			return;
		}
		if ((p = strchr(buf, ':')) == NULL) {
			pw_abort();
			error(_PATH_MASTERPASSWD " line has no colon:\n%s", buf);
			return;
		}
		*p = '\0';
		if (in_user_list(buf) >= 0)
			continue;
		*p = ':';

		if (fputs(buf, tempfp)) {
			pw_abort();
			error("write to " _PATH_MASTERPASSWD " failed: %s", strerror(errno));
			return;
		}
		filesize += strlen(buf);
	}
	fclose(masterfp);
	fflush(tempfp);

	/* Check file size - This is probably unnecessary paranoia */
	if (fstat(tempfd, &st) < 0) {
		pw_abort();
		error("can't stat " _PATH_MASTERPASSWD_LOCK);
		return;
	}
	if (st.st_size != filesize) {
		pw_abort();
		error(_PATH_MASTERPASSWD_LOCK " wrong size: %ld instead of %ld",
		      st.st_size, filesize);
		return;
	}
	fclose(tempfp);

	if (verbose > 0)
		printf("rebuilding hashed passwd file\n");
	if (pw_mkdb(NULL, 0) < 0) {
		pw_abort();
		error("Cannot update password database");
	}
	if (verbose > 0)
		printf("passwd update complete\n");
}

main(int argc, char **argv)
{
	int i, j;
	int from_stdin = 0;
	int rflag = 0;
	char bf[BFSZ];
	char *reason = NULL;
	int prompt = isatty(0);
	int confirm = 1;
	int delete = 0;
	int ntozap;
	char *p;
	struct rlimit rlim;

	/* Don't dump core (could contain part of shadow file) */
	rlim.rlim_cur = rlim.rlim_max = 0;
	setrlimit(RLIMIT_CORE, &rlim);

#ifndef DEBUG
	if (getuid() != 0) {
		fprintf(stderr, "you are not root!\n");
		exit(1);
	}
#endif

	/* Load config file */
	read_config();

	/* Parse command line arguments */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (rflag) {
				rflag = 0;
				reason = argv[i];
			} else
				insert_user(argv[i]);
		} else {
			if (argv[i][1] == '\0')
				from_stdin = 1;
			else
				for (j = 1; argv[i][j] != '\0'; j++)
					switch (argv[i][j]) {
					case 'd':
						delete = 1;
						break;
					case 'i':
						confirm = 0;
						break;
					case 'v':
						verbose++;
						break;
					case 's':
						print_errors = 0;
						break;
					case 'm':
						protect_members = 0;
						break;
					case 'r':
						rflag = 1;
						break;
					default:
						goto usage;
					}
		}
	}

	/* Read additional users to delete from stdin */
	if (from_stdin || nuser == 0) {
		for (;;) {
			if (prompt)
				fputs("User to delete: ", stdout);
			if (fgets(bf, BFSZ, stdin) == NULL) {
				if (prompt)
					putchar('\n');
				break;
			}
			if (prompt && bf[0] == '\n')
				break;
			if (bf[0] == '#')
				continue;
			if ((p = strchr(bf, '\n')) != NULL)
				*p = '\0';
			if (strlen(bf) > MAX_LOGIN) {
				fprintf(stderr, "%s: Login '%s' too long\n", argv[0], bf);
				continue;
			}
			insert_user(strdup(bf));
			if (prompt)
				break;
		}
	}
	if (nuser == 0) {
		fprintf(stderr, "nobody to delete?\n");
		exit(1);
	}
	/* If no reason given on command line, and interactive, prompt for it */
	if (reason == NULL) {
		if (reason == NULL && prompt) {
			fputs("Why? ", stdout);
			if (fgets(bf, BFSZ, stdin) != NULL) {
				if ((p = strchr(bf, '\n')) != NULL)
					*p = '\0';
				reason = bf;
			}
		} else
			reason = "";
	}
	/*
	 * Get login for logging - prefer name from utmp or environment,
	 * since these are more likely to be real identity, rather than just
	 * 'root'. Under sunOS we also tried cuserid(), but that has been
	 * depreciated. With OpenBSD getlogin() should always work for us, so
	 * it's moot. If you port this to something else, doing some version
	 * of getpwuid(getuid()) as a last chance fallback might be a good
	 * idea.
	 */
	if ((myid = getlogin()) == NULL &&
	    (myid = getenv("LOGNAME")) == NULL &&
	    (myid = getenv("USER")) == NULL) {
		error("can not figure out who is running me");
		return;
	}
	/* Load list of staffers and members who we won't delete */
	if (protect_members)
		find_members();

	/* Get a non-redirected stream to ask for confirmation on */
	if (confirm) {
		if (prompt)
			ttyr = stdin;
		else if ((ttyr = fopen("/dev/tty", "r+")) == NULL)
			error("cannot open /dev/tty to read+write");
	}
	/* Load immortals file */
	load_immortals();

	/* Get data about all users, and do sanity checking */
	ntozap = nuser;
	for (i = 0; i < nuser; i++) {
		if (check_user(&user[i], confirm)) {
			user[i].login = NULL;
			ntozap--;
		} else
			zulog(user[i].line, reason);
	}
	curr_user = NULL;
	if (ntozap == 0)
		exit(1);
	if (confirm && ttyr != NULL && ttyr != stdin)
		fclose(ttyr);
	free_immortals();

	if (verbose > 0)
		printf("sorting users\n");
	sort_users();

	/* Nuke the passwd file entries */
	if (verbose > 0)
		printf("removing users from passwd\n");
	zap_passwd();

	check_deleted();

	/* Nuke the user's files */
	for (i = 0; i < nuser; i++) {
		if (user[i].login == NULL || user[i].home == NULL)
			continue;
		if (verbose > 0)
			printf("deleting files of %s\n", user[i].login);
		delete_files(&user[i], delete);

		/*
		 * This deletes homedir, mailbox, and passwd file line.  It
		 * is safe to do before deleting mathom and reaping the
		 * outbound file.
		 */
		free_user(&user[i]);
	}
	curr_user = NULL;

	if (verbose > 0)
		printf("deleting from exim.outbound file\n");
	reap_outbound();

	if (verbose > 0)
		printf("deleting mathom\n");
	delete_mathom();

	exit(0);

usage:
	fprintf(stderr, "usage: %s [-dimsvv] [-r <reason>] [-] <user>...\n");
	exit(1);
}
