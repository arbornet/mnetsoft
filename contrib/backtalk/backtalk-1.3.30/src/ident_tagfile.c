/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Tagfile Identification Database Interface
 *
 * This module supports using tagfiles as an identification database.  This
 * is a file named 'id' that sits in the user's home directory and has lines
 * like 'fullname=(Jan Wolter)'.  The directory path isn't stored in the
 * tagfile, because you need it before you can access the tagfile.  Instead
 * we generate the directory name from the login name.
 */


#include "backtalk.h"
#include "authident.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "set_uid.h"
#include "str.h"
#include "udb.h"
#include "tag.h"
#include "lock.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_ident_module= "ident_tagfile";


/* SAVE_TAG - Write the given variable/value pair into a tag file, replacing
 * the old value, if there was one, or appending the value.  The value is
 * not changed if it was protected.  This always takes five arguments, but
 * the last may be either an integer or a string depending on the type.
 */

#if __STDC__
void save_tag(char *fname, int amadm, char *varname, int type, ...)
#else
void save_tag(fname, amadm, varname, type, va_alist)
    char *fname;
    int amadm;
    char *varname;
    int type;
    va_dcl
#endif
{
    va_list ap;
    taglist *tagl= NULL;

    VA_START(ap, type);

    /* Add our new tag to our tag list */
    if (type == TK_INTEGER)
	add_int_tag(&tagl, varname, va_arg(ap, int));
    else
	add_str_tag(&tagl, varname, va_arg(ap, char *));

    /* Update the tag file with the tag list */
    update_tag(fname, &tagl, amadm);
    
    va_end(ap);
}


/* IDENT_GETPWNAM - Get the database entry for the named user.
 */

#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *nam)
{
    static struct passwd pw;
    static char fn[BFSZ];
    char fname[BFSZ+1];
    char *dir, *var, *val;
    rtag *rt;
    int ch;
    int rc;

    pw.pw_name= nam;

    /* Construct the directory name */
    if (nam == NULL || (dir= homedirpath(nam)) == NULL)
	return NULL;
    pw.pw_dir= dir;

    /* Open tag file */
    sprintf(fname,"%s/"IDENT_FILE,dir);
    if ((rt= open_tag(fname,TRUE)) == NULL)
	return NULL;

    /* Read through tag file */
    while ((rc= get_tag(rt, &var, &val)) > 0)
    {
    	if (rc > 2) rc-= 2;

	if (!strcmp(var, "uid"))
	{
	    if (rc != 2) goto fail;
	    pw.pw_uid= atoi(val);
	}
	else if (!strcmp(var, "gid"))
	{
	    if (rc != 2) goto fail;
	    pw.pw_gid= atoi(val);
	}
	else if (!strcmp(var, "fullname"))
	{
	    if (rc != 1) goto fail;
	    strcpy(fn,val);
	    pw.pw_gecos= fn;
	}
    }

    close_tag(rt);
    return &pw;

fail:
    close_tag(rt);
    return NULL;
}
#endif


/* IDENT_NEWFNAME - change the user's fullname to the given value.
 */

#ifdef HAVEIDENT_NEWFNAME
void ident_newfname(struct passwd *pw, char *newfname)
{
    char *fname;

    /* Construct name of ident file */
    fname= gethomedir(pw->pw_name);
    strcat(fname,"/");
    strcat(fname,IDENT_FILE);

    flushusercache();

    save_tag(fname, 0, "fullname", TK_STRING, newfname);
}
#endif


/* IDENT_NEWGID - change the user's groupid to the given value.
 */

#ifdef HAVEIDENT_NEWGID
void ident_newgid(struct passwd *pw, gid_t newgid)
{
    char *fname;

    /* Construct name of ident file */
    fname= gethomedir(pw->pw_name);
    strcat(fname,"/");
    strcat(fname,IDENT_FILE);

    flushusercache();

    save_tag(fname, 1, "gid", TK_INTEGER, newgid);
}
#endif


/* IDENT_ADD - Add an entry to the user database describing a user with
 * the given login, uid, gid, password, fullname, and directory.
 */

#ifdef HAVEIDENT_ADD
void ident_add(char *lid, uid_t uid, gid_t gid, char *fn, char *dir)
{
    char fname[BFSZ];
    FILE *fp;

    sprintf(fname,"%s/"IDENT_FILE,dir);
    if ((fp= fopen(fname,"w")) == NULL)
	die("could not create ident file %s",fname);
    lock_exclusive(fileno(fp),fname);
    fprintf(fp,"*uid=%d\n*gid=%d\nfullname={%s}\n", uid, gid, fn);
    fclose(fp);
    unlock(fname);
}
#endif


/* IDENT_DEL - Delete the named user from the ident database.  Actually, there
 * is nothing to do here, since we will always be deleting the directory
 * next, and that will take care of everything.
 */

#ifdef HAVEIDENT_DEL
void ident_del(char *name) { }
#endif
