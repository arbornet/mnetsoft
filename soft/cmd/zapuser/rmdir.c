#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>

#include "zapuser.h"

/* ====== DIRECTORY STACK ROUTINES ====================================== */

/*
 * The directory stack is used while doing a depth-first walk of the
 * directory trees, deleting as you go.  dirstack[0] is the full path name of
 * the root of the directory tree.  dirstack[1] is a subdirectory of that,
 * dirstack[2] is a subdirectory of that, and so on, down to dirstack[dstop]
 * which is the name of the current subdirectory.  Stack is grown with
 * realloc() if it isn't big enough.
 */

char **dirstack = NULL;		/* Stack of directory names */
long dssize = 0;		/* Number of entries in dirstack array */
long dstop = -1;		/* Index of last used directory name in
				 * dirstack */


/*
 * DS_CURR Return the current directory name as a string.  Used mainly for
 * debug output, but also gives absolute path of directory.  The path is
 * returned in malloced memory which the calling program should free.
 */

char *
ds_curr()
{
	long plen = 0, psz = BFSZ;
	char *path = (char *) malloc(psz);
	int i, l;

	for (i = 0; i <= dstop; i++) {
		l = strlen(dirstack[i]);
		if (plen + 2 + l >= psz) {
			psz = plen + l + BFSZ;
			path = (char *) Realloc(path, psz);
			if (path == NULL) {
				fprintf(stderr, "out of memory for path name\n");
				exit(1);
			}
		}
		if (plen != 0)
			path[plen++] = '/';
		strlcpy(path + plen, dirstack[i], psz - plen);
		plen += l;
	}
	return path;
}


/*
 * DS_PUSH Push a directory name onto the directory stack and cd into it.  We
 * assume that if the directory path is not absolute, then we are in the
 * parent directory.  Return true if the chdir fails, with errno set.
 */

int
ds_push(char *dir)
{
	if (verbose > 1)
		printf("entering directory %s\n", dir);

	if (++dstop >= dssize) {
		dssize += BFSZ;
		dirstack = (char **) Realloc(dirstack, dssize * sizeof(char *));
	}
	dirstack[dstop] = strdup(dir);

	/* Give ourselves read, write and execute access to the directory */
	chmod(dir, 0700);

	return chdir(dir);
}

/*
 * DS_INIT Initialize the directory stack, with given root directory name,
 * and cd to that directory.  This should be called once before any other ds_*
 * functions.
 */

int
ds_init(char *root)
{
	dssize = BFSZ;
	dirstack = (char **) malloc(dssize * sizeof(char *));
	if (dirstack == NULL) {
		fprintf(stderr, "out of memory for directory stack\n");
	}
	dstop = -1;
	return ds_push(root);
}


/*
 * DS_FREE Discard the directory stack.  Can be done to release memory after
 * we are done.
 */

void
ds_free()
{
	int i;

	for (i = 0; i <= dstop; i++)
		free(dirstack[i]);
	free(dirstack);
	dstop = -1;
	dssize = 0;
	dirstack = NULL;
}


/*
 * DS_POP Pop a directory name from the directory stack.  Returns a pointer
 * to malloced memory, which the caller should free() when they are done with
 * it.  Returns NULL if the stack is empty.
 */

char *
ds_pop()
{
	char *d;

	if (dstop < 0)
		return NULL;

	if (verbose > 1)
		printf("leaving directory %s\n", dirstack[dstop]);

	dstop--;

	if (verbose > 1 && dstop >= 0) {
		printf("returning to directory %s\n", d = ds_curr());
		free(d);
	}
	return dirstack[dstop + 1];
}

/* ====== END OF DIRECTORY STACK ROUTINES ================================== */


/*
 * DELETE_DIRECTORY Delete a directory tree rooted at dir belonging to the
 * user with the given uid.  All deletion is done with our uid set to the
 * user, so we cannot be mislead by symbolic links to delete anything other
 * than that user's files. (We also check if things are symbolic links, but
 * what was a real directory when we checked it could have turned into a
 * symbolic link by the time we cd to it.
 */

int
delete_directory(uid_t uid, char *dir)
{
	struct stat st;
	DIR *dp;
	struct dirent *dr;
	char *d, *sd;

	setuid(0);		/* Set real uid to superuser */
	seteuid(uid);		/* Set effective uid to user */

	if (verbose > 1)
		printf("running as uid %d\n", geteuid());

	/* Initialize directory stack and cd into root directory */
	if (ds_init(dir)) {
		error("Could not cd to directory %s (%s)", dir, strerr(errno));
		goto fail;
	}
	/* Outer look - loops through directory and all subdirectories */
	while (1) {
		/*
		 * Open the directory (should have given ourselves read
		 * access before chdir-ing into the directory)
		 */
		if ((dp = opendir(".")) == NULL) {
			error("Could not open directory %s (%s)", d = ds_curr(),
			      strerr(errno));
			free(d);
			goto fail;
		}
		/* Double check that the open directory has the right owner */
		if (fstat(dir_fileno(dp), &st) || st.st_uid != uid) {
			error("Directory %s not owned by user %d", d = ds_curr(), uid);
			free(d);
			goto fail;
		}
		while ((dr = readdir(dp)) != NULL) {
			/* Skip . and .. entries */
			if (!strcmp(dr->d_name, ".") || !strcmp(dr->d_name, "..")) {
				if (verbose > 1)
					printf("ignoring %s\n", dr->d_name);
				continue;
			}
			/* Stat the file - if it's gone, just ignore it */
			if (lstat(dr->d_name, &st)) {
				if (verbose > 1)
					printf("file %s vanished\n", dr->d_name);
				continue;
			}
			if (S_ISDIR(st.st_mode)) {
				/*
				 * Subdirectory - switch to working on that
				 * instead
				 */
				if (ds_push(dr->d_name)) {
					error("could not chdir into %s (%s)",
					      d = ds_curr(), strerr(errno));
					free(d);
					goto fail;
				}
				/* Close parent directory (frees dr->d_name) */
				closedir(dp);
				goto newdir;
			} else {
				/* Not a directory - just unlink it */
				if (verbose > 1)
					printf("deleting file %s\n", dr->d_name);
				if (unlink(dr->d_name)) {
					error("could not unlink %s/%s (%s)",
					      d = ds_curr(), dr->d_name, strerr(errno));
					free(d);
					goto fail;
				}
			}
		}

		/* Hit end of directory - close it */
		closedir(dp);

		/* CD out of directory before deleting it */
		if (chdir("..")) {
			error("could not chdir to parent of %s (%s)",
			      d = ds_curr(), strerr(errno));
			free(d);
			goto fail;
		}
		/* Delete it */
		sd = ds_pop();
		if (verbose > 1)
			printf("deleting directory %s\n", sd);
		/*
		 * Restore effective uid to root before deleting home
		 * directory
		 */
		if (dstop < 0)
			seteuid(0);
		if (rmdir(sd)) {
			if (dstop < 0)
				error("could not delete %s (%s)", sd, strerr(errno));
			else {
				error("could not delete %s/%s (%s)",
				      d = ds_curr(), sd, strerr(errno));
				free(d);
			}
			free(sd);
			goto fail;
		}
		free(sd);

		/* check if it was root dir */
		if (dstop < 0)
			break;

newdir:	;
	}

	ds_free();
	return 0;

fail:
	seteuid(0);
	ds_free();
	return 1;
}
