/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "sysdict.h"
#include "baaf_core.h"
#include "stack.h"
#include "str.h"

#if ATTACHMENTS

#include <sys/stat.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

extern int errno;


/* BAAF_PATH - Given the handle file name of a Backtalk Attachment Archive
 * file, construct the full path name.  We use two levels of hierarchical
 * directories under BAFF_DIR so that we don't create one gigantic directory,
 * but those are just constructed using a checksum on the file name.  So the
 * terminal file name uniquely determines the full path name, and you never
 * have the same filename appearing in two different directories.  Return
 * value is in dynamic memory.
 */

char *baaf_path(char *fname)
{
    int cksum= 0;
    int i;
    char c1, c2;
    char *out= (char *)malloc(strlen(fname)+6+strlen(ATTACH_DIR));
    char *slash;

    if ((slash= strchr(fname,'/')) != NULL) *slash='.';

    for (i= 0; fname[i] != '\0'; i++)
	cksum= cksum ^ (fname[i] << (i % 7));

    c1= (cksum & 0x1F);
    c1+= (c1 > 9) ? 'A'-9 : '0';
    c2= ((cksum >> 5) & 0x1F);
    c2+= (c2 > 9) ? 'A'-9 : '0';

    sprintf(out,ATTACH_DIR"/%c/%c/%s", c1, c2, fname);

    if (slash != NULL) *slash='/';

    return out;
}


/* DEL_ATTACH:  Delete an attachment
 */

void del_attach(char *hand)
{
    char *path= baaf_path(hand);

    baai_del(hand);

    unlink(path);
    free(path);
}

#endif /* ATTACHMENTS */
