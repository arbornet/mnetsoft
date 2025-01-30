#include "zapuser.h"

#include <stdlib.h>
#include <errno.h>
#include <dirent.h>


char *mathom_dir = NULL;


/*
 * DELETE_MATHOM Go through the mathom directory, looking for filenames of
 * the form <login> or <login>.* for each login in the global user list.
 * Delete all the files found.  This makes only one pass through the mathom
 * directory.
 */

void
delete_mathom()
{
	DIR *dp;
	struct dirent *dr;
	char *p;

	if (mathom_dir == NULL)
		return;

	if (chdir(mathom_dir)) {
		error("cannot cd to directory '%s' (%s)",
		      mathom_dir, strerr(errno));
		return;
	}
	if ((dp = opendir(".")) == NULL) {
		error("cannot read mathom directory '%s' (%s)",
		      mathom_dir, strerr(errno));
		return;
	}
	while ((dr = readdir(dp)) != NULL) {
		/*
		 * Skip . and .. entries (and any other filenames starting
		 * with .)
		 */
		if ((p = strchr(dr->d_name, '.')) == dr->d_name)
			continue;

		if (p != NULL)
			*p = '\0';
		if (in_user_list(dr->d_name) >= 0) {
			if (p != NULL)
				*p = '.';
			if (unlink(dr->d_name))
				error("could not remove mathom file %s/%s (%s)",
				      mathom_dir, dr->d_name, strerr(errno));
		}
	}
}
