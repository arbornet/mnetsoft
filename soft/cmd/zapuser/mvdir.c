#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "zapuser.h"

/*
 * DELETED_NAME Figure out the directory to move the deleted directory to.
 * Should implement this is a more portable way.  It's currently somewhat
 * Grex-centric and not configurable.  Returns NULL if it can't figure one
 * out, otherwise returns pointer to a directory name stored in malloc'ed
 * memory.
 */

char *
deleted_name(char *homedir)
{
	char *a, *b;
	char *deldir;
	int n;
	char suf;
	time_t now = time(0L);
	struct tm *tm = localtime(&now);
	size_t len;

	if (homedir[0] != '/' ||
	    (a = strchr(homedir + 1, '/')) == NULL) {
		error("bizzare home directory %s", homedir);
		return NULL;
	}
	b = strrchr(homedir, '/');

	n = a - homedir;
	len = n + strlen(b) + 30 + 1;
	deldir = malloc(len);
	if (deldir == NULL) {
		error("out of memory in deleted directory name\n");
		exit(1);
	}
	snprintf(deldir, len, "%.*s/deleted/%s.%04d%02d%02dA",
		 n, homedir, b + 1,
		 tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

	/* Check that it doesn't already exist */
	n = strlen(deldir) - 1;
	deldir[n];
	suf = 'A';
	while (access(deldir, F_OK) == 0) {
		/* Exists */
		if (suf > 'Z') {
			error("%s already exists", deldir);
			return NULL;
		}
		deldir[n] = suf;
		suf++;
	}

	return deldir;
}


/*
 * MOVE_DIRECTORY Move the given home directory to a "deleted" area on the
 * same partition. Return pointer to new directory path on success (in
 * malloc'ed memory). return NULL on failure.
 */

char *
move_directory(char *homedir)
{
	char *deldir;

	if ((deldir = deleted_name(homedir)) == NULL)
		return NULL;

	if (rename(homedir, deldir)) {
		error("rename of directory '%s' to '%s' failed (%s)",
		      homedir, deldir, strerr(errno));
		return NULL;
	}
	return deldir;
}


/*
 * MOVE_MAIL Stick the named mailfile into the named "deleted directory."
 */

void
move_mail(char *mailfile, char *deldir)
{
	char *savefile;
	int old, new;
	char bf[BFSZ];
	int i;
	int n;
	size_t len;

	/* If the mailfile doesn't exist, don't bother */
	if ((old = open(mailfile, O_RDONLY)) < 0)
		return;

	n = strlen(deldir);
	len = n + 30;
	savefile = malloc(len);
	if (savefile == NULL) {
		error("out of memory for mail save file name\n");
		exit(1);
	}
	strlcpy(savefile, deldir, len);
	strlcat(savefile, "/UNREAD_MAIL", len);

	/* Check that it doesn't already exist */
	i = 0;
	n = strlen(savefile);
	while ((new = open(savefile, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0 &&
	       errno == EEXIST) {
		/* File exists */
		if (i > 99) {
			error("%s already exists - mail not saved", deldir);
			return;
		}
		snprintf(savefile + n, len - n, ".%d", ++i);
	}
	if (new < 0) {
		error("could not create %s (%s) - mail not saved",
		      savefile, strerr(errno));
		return;
	}
	/* Delete the old file */
	if (unlink(mailfile))
		error("could not unlink %s (%s)", mailfile, strerr(errno));

	/* Do the copy */
	while ((n = read(old, bf, BFSZ)) > 0)
		write(new, bf, n);

	close(old);
	close(new);
}
