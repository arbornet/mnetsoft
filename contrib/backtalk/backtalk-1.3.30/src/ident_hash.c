/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Hash File Identification Database Interface
 *
 * This module supports a user identification database saved as a hash file.
 * The user's login name (not including the terminating null) is used as a
 * key to get blocks of data which are terminated by a newline and a null and
 * look like:
 *
 *    <password>:<uid>:<gid>:<fullname>:<directory>:\n
 *
 * The <password> will only be valid if the database is shared with the
 * authentication database.
 *
 * For systems that need to be able to look up users by uid number (so far
 * this never happens and it probably never will) this also maintains and
 * uses an alternate database indexed by the user's uid number (as a four
 * byte binary number) and containing lines like:
 *
 *    <password>:<login>:<gid>:<fullname>:<directory>:\n
 *
 * The <password>, <gid>, <uid>, <fullname> and <directory> values should
 * always be identical to the ones in the other hash file.
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
#include "hashfile.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_ident_module= "ident_hash (db="DBM_NAM_FILE")";

/* IDENT_GETPWNAM - Get user data from the DBM_NAM_FILE.  Save info into a
 * standard passwd structure in static storage.  Return NULL on failure.
 *
 * DBM_NAM_FILE entries look like:
 *   <encryptpassswd>:<uid>:<gid>:<fullname>:<directory>
 */

#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *nam)
{
    static char bf[BFSZ];
    static char name[MAX_LOGIN_LEN+1];
    static struct passwd pw;
    char *e, *b;
    int rc;

    /* Fetch the line from the db file */
    set_httpd_if_shared();
    rc= fetchdbm(DBM_NAM_FILE, nam, strlen(nam), bf, BFSZ);
    set_cfadm_if_shared();

    /* Return NULL if not found */
    if (rc) return NULL;

    /* Copy name into static storage */
    strcpy(name,nam);
    pw.pw_name= name;

    /* Get password */
    if ((e= strchr(bf,':')) == NULL) return NULL;
    pw.pw_passwd= bf; *e= '\0';

    /* Get uid */
    b= e+1;
    if ((e= strchr(b,':')) == NULL) return NULL;
    pw.pw_uid= atoi(b);

    /* Get gid */
    b= e+1;
    if ((e= strchr(b,':')) == NULL) return NULL;
    pw.pw_gid= atoi(b);
    
    /* Get fullname */
    b= e+1;
    if ((e= strchr(b,':')) == NULL) return NULL;
    pw.pw_gecos= b; *e= '\0';
    
    /* Get directory */
    b= e+1;
    pw.pw_dir= b;
    if ((e= strchr(b,':')) != NULL) *e= '\0';

    return &pw;
}
#endif


/* GETIDENTUID - Get user data from the DBM_UID_FILE.  Save info into a
 * standard passwd structure in static storage.  Return NULL on failure.
 *
 * DBM_UID_FILE entries look like:
 *   <encryptpassswd>:<loginuid>:<gid>:<fullname>:<directory>
 */

#ifdef HAVEIDENT_GETPWUID
struct passwd *ident_getpwuid(uid_t uid)
{
    static char bf[BFSZ];
    static struct passwd pw;
    char *e, *b;
    int rc;

    /* Fetch the line from the db file */
    set_httpd_if_shared();
    rc= fetchdbm(DBM_UID_FILE, (char *)&uid, sizeof(uid), bf, BFSZ);
    set_cfadm_if_shared();

    /* Return NULL if not found */
    if (rc) return NULL;

    /* Get password */
    if ((e= strchr(bf,':')) == NULL) return NULL;
    pw.pw_passwd= bf; *e= '\0';

    /* Get name */
    b= e+1;
    if ((e= strchr(b,':')) == NULL) return NULL;
    pw.pw_name= b; *e= '\0';

    /* Save uid */
    pw.pw_uid= uid;

    /* Get gid */
    b= e+1;
    if ((e= strchr(b,':')) == NULL) return NULL;
    pw.pw_gid= atoi(b);
    
    /* Get fullname */
    b= e+1;
    if ((e= strchr(b,':')) == NULL) return NULL;
    pw.pw_gecos= b; *e= '\0';
    
    /* Get directory */
    b= e+1;
    pw.pw_dir= b;
    if ((e= strchr(b,':')) != NULL) *e= '\0';
    
    return &pw;
}
#endif


/* IDENT_WALK - Return the "first" user in the dbm file.
 */

#ifdef HAVEIDENT_WALK
char *ident_walk(int flag)
{
    static char bf[MAX_LOGIN_LEN+1];

    if (!flag)
    {
	set_httpd_if_shared();
	if (walkdbm(DBM_NAM_FILE))
	{
	    set_cfadm_if_shared();
	    die("ident_walk cannot open name db "DBM_NAM_FILE" to read");
	}
	set_cfadm_if_shared();
	
	if (firstdbm(bf, MAX_LOGIN_LEN+1))
	    return NULL;
	else
	    return bf;
    }
    else
    {
	if (nextdbm(bf, MAX_LOGIN_LEN+1))
	    return NULL;
	else
	    return bf;
    }
}
#endif


/* IDENT_SEEK - Set the scan to start after the named user.  Future nextusr()
 * calls will continue from there.
 */

#ifdef HAVEIDENT_SEEK
void ident_seek(char *login)
{
    set_httpd_if_shared();

    if (walkdbm(DBM_NAM_FILE))
    {
	set_cfadm_if_shared();
	die("ident_seek cannot open name db "DBM_NAM_FILE" to read");
    }
    set_cfadm_if_shared();
    
    seekdbm(login, strlen(login));
}
#endif


/* NEWFNAME - change the user's fullname to the given value.
 */

#ifdef HAVEIDENT_NEWFNAME
void ident_newfname(struct passwd *pw, char *newfname)
{
    char bf[BFSZ];

    set_httpd_if_shared();

#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%d:%d:%.300s:%.300s:\n", pw->pw_passwd,
	pw->pw_uid, pw->pw_gid, newfname, pw->pw_dir);
#else
    sprintf(bf,"*:%d:%d:%.300s:%.300s:\n",
	pw->pw_uid, pw->pw_gid, newfname, pw->pw_dir);
#endif

    if (storedbm(DBM_NAM_FILE,
    		 pw->pw_name, strlen(pw->pw_name),
    		 bf, strlen(bf)+1, 0))
    {
	set_cfadm_if_shared();
    	die("Cannot open user database "DBM_NAM_FILE" (%s)",errdbm());
    }

#ifdef DBM_UID_FILE
#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%.300s:%d:%.300s:%.300s:\n", pw->pw_passwd,
	pw->pw_name, pw->pw_gid, newfname, pw->pw_dir);
#else
    sprintf(bf,"*:%.300s:%d:%.300s:%.300s:\n",
	pw->pw_name, pw->pw_gid, newfname, pw->pw_dir);
#endif

    if (storedbm(DBM_UID_FILE,
    		 (char *)&pw->pw_uid, sizeof(uid_t),
    		 bf, strlen(bf)+1, 0))
    {
	set_cfadm_if_shared();
    	die("Cannot open user database "DBM_UID_FILE" (%s)",errdbm());
    }
#endif

    set_cfadm_if_shared();
    flushusercache();
}
#endif /* HAVEIDENT_NEWFNAME */


/* IDENT_NEWGID - change the user's primary gid to the given value.
 */

#ifdef HAVEIDENT_NEWGID
void ident_newgid(struct passwd *pw, gid_t newgid)
{
    char bf[BFSZ];

    set_httpd_if_shared();

#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%d:%d:%.300s:%.300s:\n", pw->pw_passwd,
	pw->pw_uid, newgid, pw->pw_gecos, pw->pw_dir);
#else
    sprintf(bf,"*:%d:%d:%.300s:%.300s:\n",
	pw->pw_uid, newgid, pw->pw_gecos, pw->pw_dir);
#endif

    if (storedbm(DBM_NAM_FILE,
    		 pw->pw_name, strlen(pw->pw_name),
    		 bf, strlen(bf)+1, 0))
    {
	set_cfadm_if_shared();
    	die("Cannot open user database "DBM_NAM_FILE" (%s)",errdbm());
    }

#ifdef DBM_UID_FILE
#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%.300s:%d:%.300s:%.300s:\n", pw->pw_passwd,
	pw->pw_name, newgid, pw->pw_gecos, pw->pw_dir);
#else
    sprintf(bf,"*:%.300s:%d:%.300s:%.300s:\n",
	pw->pw_name, newgid, pw->pw_gecos, pw->pw_dir);
#endif

    if (storedbm(DBM_UID_FILE,
    		 (char *)&pw->pw_uid, sizeof(uid_t),
    		 bf, strlen(bf)+1, 0))
    {
	set_cfadm_if_shared();
    	die("Cannot open user database "DBM_UID_FILE" (%s)",errdbm());
    }
#endif

    set_cfadm_if_shared();
    flushusercache();
}
#endif /* HAVEIDENT_NEWGID */


/* IDENT_ADD - Add an entry to the user database describing a user with
 * the given login, uid, gid, password, fullname, and directory.
 */

#ifdef HAVEIDENT_ADD
void ident_add(char *lid, uid_t uid, gid_t gid, char *fn, char *dir)
{
    char bf[BFSZ];

    set_httpd_if_shared();

    sprintf(bf,"*:%d:%d:%s:%s:\n", uid, gid, fn, dir);

    if (storedbm(DBM_NAM_FILE,
		 lid, strlen(lid),	/* don't include terminating NUL */
		 bf, strlen(bf)+1,	/* include terminating NUL */
		 1))			/* create file if necessary */
    {
	set_cfadm_if_shared();
	die("Cannot open user database "DBM_NAM_FILE" (%s)",errdbm());
    }

#ifdef DBM_UID_FILE

    sprintf(bf,"*:%s:%d:%s:%s:\n", lid, gid, fn, dir);

    if (storedbm(DBM_UID_FILE,
		 (char *)&uid, sizeof(uid_t),
		 bf, strlen(bf)+1,	/* include terminating NUL */
		 1))			/* create file if necessary */
    {
	set_cfadm_if_shared();
	die("Cannot open user database "DBM_UID_FILE" (%s)",errdbm());
    }
#endif

    set_cfadm_if_shared();
}
#endif


/* IDENT_DEL - Delete the named user from the ident database.
 */

#ifdef HAVEIDENT_DEL
void ident_del(char *login)
{
    deletedbm(DBM_NAM_FILE, login, strlen(login), 0);
#ifdef DBM_UID_FILE
    deletedbm(DBM_UID_FILE, (char *)&pw->pw_uid, sizeof(pw->pw_uid), 0);
#endif /* DBM_UID_FILE */
}
#endif
