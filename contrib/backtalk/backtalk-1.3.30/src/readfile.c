/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"
#include "readfile.h"
#include "lock.h"

#include <sys/stat.h>


/* READ_FILE - Read the contents of the named file into a malloced memory
* area, and return a pointer to it.  Return NULL on failure.  The name is
* a full path name.
 */

char *read_file(char *fname, int lock)
{
    struct stat st;
    FILE *fp;
    int len;
    char *bf;

    if ((fp= fopen(fname,"r")) == NULL)
	return NULL;

    if (fstat(fileno(fp),&st))
    {
	fclose(fp);
	return NULL;
    }
    if (lock) lock_shared(fileno(fp),fname);

    /* This is marginally bogus -- the file might have grown since the stat */
    bf= (char *)malloc(st.st_size+1);
    len= fread(bf, 1, st.st_size, fp);
    bf[len]= '\0';

    fclose(fp);
    if (lock) unlock(fname);

    return bf;
}
