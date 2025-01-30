#include "common.h"
#include "baaf_core.h"
#include "baai.h"

#define MAX_TMP_AGE 60*60	/* Kill abandoned tmp files after an hour */
#define MAX_LINKLESS_AGE 60*60	/* Kill unlinked attachments after an hour */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_DIRENT_H
#include <dirent.h>
typedef struct dirent direntry;
#else
typedef struct direct direntry;
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

int verbose;

/* Kill any files in the tmp attach directory that are more than MAX_TMP_AGE
 * seconds old.
 */

kill_tmp()
{
    DIR *dir;
    direntry *en;
    struct stat st;
    time_t now= time(0L);

    if (chdir(ATTACH_DIR"tmp"))
	die("Cannot cd to "ATTACH_DIR"tmp");

    if ((dir= opendir(ATTACH_DIR"tmp")) == NULL)
	die("Cannot open "ATTACH_DIR"tmp");

    if (verbose) printf("Checking "ATTACH_DIR"tmp\n");

    while ((en= readdir(dir)) != NULL)
    {
	if (en->d_name[0] == '.') continue;
	if (stat(en->d_name, &st))
	    die("Cannot stat "ATTACH_DIR"tmp/%s",en->d_name);
	if (st.st_mtime + MAX_TMP_AGE < now)
	{
	    unlink(en->d_name);
	    if (verbose) printf("File %s: Deleted\n",en->d_name);
	}
	else
	    if (verbose) printf("File %s: OK\n",en->d_name);
    }
    closedir(dir);

}


/* Kill any real attachments that never got marked as linked to an item.
 */

kill_linkless()
{
    int next= FALSE;
    char *handle;
    int link;
    time_t date, now= time(0L);

    if (verbose) printf("Checking for linkless attachments\n");

    while (baai_walk(next,&handle,&link,NULL,NULL,NULL,NULL,NULL,NULL,
	&date,NULL))
    {
	if (link == 0 && date + MAX_LINKLESS_AGE < now)
	{
	    del_attach(handle);
	    if (verbose) printf("File %s: Deleted\n",handle);
	}
	else
	    if (verbose) printf("File %s: OK\n",handle);
	free(handle);
	next= TRUE;
    }
}


main(int argc,char **argv)
{
    verbose= (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'v');

    kill_tmp();
    kill_linkless();
}
