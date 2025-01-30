/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Flat Textfile Identification Database Interface
 *
 * This module supports a user identification database formatted much like
 * a standard unix /etc/passwd file.  Each line looks like:
 *
 *   <login>:<passwd>:<uid>:<gid>:<fullname>:<homedir>:
 *
 * Here <uid> is the user's uid number, <gid> is his primary group ID,
 * <fullname> is the user's full name, and <homedir> is the full path of the
 * directory where his configuration and participation files are saved.
 */

#include "common.h"
#include "authident.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "set_uid.h"
#include "str.h"
#include "lock.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_ident_module= "ident_text (file="PASSWD_FILE")";

#ifndef HAVE_FGETPWENT

/* FGETPWENT - Read a passwd-file-like line from the given file descriptor.
 * This is a built-in function under Solaris, but not apparantly much of
 * anyplace else.  THIS IS NOT VERY ROBUST!  It skips lines it doesn't
 * understand.
 */

struct passwd *fgetpwent(FILE *fp)
{
    static char bf[BFSZ+1];
    static struct passwd pw;
    char *p, *b, *e;

    while ((p= fgets(bf, BFSZ, fp)) != NULL)
    {
	/* Skip lines longer than BFSZ */
	if ((p= strchr(bf,'\n')) == NULL)
	{
	    int ch;
	    while ((ch= fgetc(fp)) != '\n' && ch != EOF)
		;
	}
	else
	{
	    /* Get login name */
	    b= bf;
	    if ((e= strchr(b,':')) == NULL) continue;
	    pw.pw_name= b; *e= '\0';

	    /* Get password */
	    b= e+1;
	    if ((e= strchr(b,':')) == NULL) continue;
	    pw.pw_passwd= b; *e= '\0';

	    /* Get uid */
	    b= e+1;
	    if (!isascii(*b) || !isdigit(*b)) continue;
	    if ((e= strchr(++b,':')) == NULL) continue;
	    pw.pw_uid= atoi(b);

	    /* Get gid */
	    b= e+1;
	    if ((e= strchr(b,':')) == NULL) continue;
	    pw.pw_gid= atoi(b);
	    
	    /* Get fullname */
	    b= e+1;
	    if ((e= strchr(b,':')) == NULL) continue;
	    pw.pw_gecos= b; *e= '\0';
	    
	    /* Get directory */
	    b= e+1;
	    pw.pw_dir= b;
	    if ((e= strchr(b,':')) != NULL) *e= '\0';

	    return &pw;
	}
    }
    return NULL;

}
#endif /* HAVE_FGETPWENT */


/* GETPWNU - Get a particular name or uid from PASSWD_FILE.  Either the name
 * should be NULL or the uid should be -1, to indicate that the other is the
 * one we are searching for.  This is not a very fast implementation.
 */

struct passwd *getpwnu(char *nam, uid_t uid)
{
    FILE *pwfp;
    struct passwd *pw;

    set_httpd_if_shared();
    if ((pwfp= fopen(PASSWD_FILE,"r")) == NULL)
    {
	set_cfadm_if_shared();
	die("getpwnu cannot open user database "PASSWD_FILE);
    }
    lock_shared(fileno(pwfp),PASSWD_FILE);

    while ((pw= fgetpwent(pwfp)) != NULL)
    {
	if ((uid >= 0 && pw->pw_uid == uid) ||
	    (nam != NULL && !strcmp(nam, pw->pw_name)))
		break;
    }

    fclose(pwfp);
    unlock(PASSWD_FILE);
    set_cfadm_if_shared();
    return pw;
}


#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *login)
{
    return getpwnu(login,-1);
}
#endif


#ifdef HAVEIDENT_GETPWUID
struct passwd *ident_getpwuid(uid_t uid)
{
    return getpwnu(NULL,uid);
}
#endif


/* IDENT_WALKPW - This runs without locks, which may occasionally give glitches,
 * but this is never used for anything very critical (like authentication)
 * and we have no way of knowing when we are done with the file, so we'd
 * have to hold the lock till we exit.
 */

#ifdef HAVEIDENT_WALKPW
FILE *dbfp= NULL;
struct passwd *ident_walkpw(int flag)
{
    /* If flag is 0, goto begining of file */
    if (!flag)
    {
	if (dbfp == NULL)
	{
	    /* Open the file handle */
	    set_httpd_if_shared();
	    dbfp= fopen(PASSWD_FILE,"r");
	    set_cfadm_if_shared();
	    if (dbfp == NULL)
		die("resetpw cannot open user database "PASSWD_FILE);
	}
	else
	    /* Rewind the file handle */
	    rewind(dbfp);
    }
 
    return fgetpwent(dbfp);
}
#endif


/* IDENT_NEWFNAME - change the user's fullname to the given value.
 */

#ifdef HAVEIDENT_NEWFNAME
void ident_newfname(struct passwd *pw, char *newfname)
{
    char bf[BFSZ];

#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%s:%d:%d:%s:%s:\n", pw->pw_name, pw->pw_passwd,
	pw->pw_uid, pw->pw_gid, newfname, pw->pw_dir);
#else
    sprintf(bf,"%s:*:%d:%d:%s:%s:\n", pw->pw_name,
	pw->pw_uid, pw->pw_gid, newfname, pw->pw_dir);
#endif

    set_httpd_if_shared();
    update_pw(PASSWD_FILE, pw->pw_name, bf);
    set_cfadm_if_shared();

    flushusercache();
}
#endif


/* IDENT_NEWGID - change the user's groupid to the given value.
 */

#ifdef HAVEIDENT_NEWGID
void ident_newgid(struct passwd *pw, gid_t newgid)
{
    char bf[BFSZ];

#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%s:%d:%d:%s:%s:\n", pw->pw_name, pw->pw_passwd,
	pw->pw_uid, newgid, pw->pw_gecos, pw->pw_dir);
#else
    sprintf(bf,"%s:*:%d:%d:%s:%s:\n", pw->pw_name,
	pw->pw_uid, newgid, pw->pw_gecos, pw->pw_dir);
#endif

    set_httpd_if_shared();
    update_pw(PASSWD_FILE, pw->pw_name, bf);
    set_cfadm_if_shared();

    flushusercache();
}
#endif


/* IDENT_ADD - Add an entry to the user database describing a user with
 * the given login, uid, gid, password, fullname, and directory.
 */

#ifdef HAVEIDENT_ADD
void ident_add(char *lid, uid_t uid, gid_t gid, char *fn, char *dir)
{
    char bf[BFSZ];
    int fd;
    size_t len;

    sprintf(bf,"%s:*:%d:%d:%s:%s:\n", lid, uid, gid, fn, dir);
    len= strlen(bf);
    
    if ((fd= open(PASSWD_FILE, O_WRONLY|O_APPEND)) < 0)
	die("ident_add cannot open "PASSWD_FILE" to write");
    lock_exclusive(fd,PASSWD_FILE);

    if (write(fd, bf, len) != len)
	die("write to "PASSWD_FILE" failed");
    
    close(fd);
    unlock(PASSWD_FILE);
}
#endif


/* DELIDENT - Delete the named user from the ident database.
 */

#ifdef HAVEIDENT_DEL
void ident_del(char *name)
{
    update_pw(PASSWD_FILE, name, NULL);
}
#endif
