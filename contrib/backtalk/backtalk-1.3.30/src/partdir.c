/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "partdir.h"


#ifdef PART_DIR

/* BBSPARTPATH -- Construct the full path name of the participation file,
 * given the file name.  It is saved in the passed-in buffer.  Returns a
 * pointer to "path" -- Yapp 3.0 version.
 */

#ifdef MIXED_CASE_LOGINS
#define lc(x) (isupper(x) ? tolower(x) : (x))
#else
#define lc(x) (x)
#endif

char *bbspartpath(char *path, char *id, char *name)
{
#if PART_DIR_LEVEL == 0
    sprintf(path,PART_DIR"%s/%s", id, name);
#elif PART_DIR_LEVEL == 1
    sprintf(path,PART_DIR"%c/%s/%s", lc(id[0]), id, name);
#else
    sprintf(path,PART_DIR"%c/%c/%s/%s",
      lc(id[0]), id[1]=='\0' ? '_' : lc(id[1]), id, name);
#endif
    return(path);
}

/* MKPARTDIR - This function creates a missing participation file directory,
 * possibly hierarchical.  It's given the full path name of the file we want
 * to store in that directory.
 */

void mkpartdir(char *name)
{
    char *dir1= strdup(name);
    char *slash= strrchr(dir1,'/');
    int rc;

    *slash= '\0';
    rc= mkdir(dir1,0700);
#if PART_DIR_LEVEL > 0
    if (rc && errno == ENOENT)
    {
	/* First hierarchical directory missing - create it */
	char *dir2= strdup(dir1);
	slash= strrchr(dir2,'/');
	*slash= '\0';
	rc= mkdir(dir2,0700);
#if PART_DIR_LEVEL > 1
	if (rc && errno == ENOENT)
	{
	    /* Second hierarchical directory missing - create it */
	    char *dir3= strdup(dir2);
	    slash= strrchr(dir3,'/');
	    *slash= '\0';
	    mkdir(dir3,0700);
	    mkdir(dir2,0700);
	    free(dir3);
	}
#endif /* PART_DIR_LEVEL > 1 */
	mkdir(dir1,0700);
	free(dir2);
    }
#endif /* PART_DIR_LEVEL > 0 */
    free(dir1);
}
#endif /* PART_DIR */
