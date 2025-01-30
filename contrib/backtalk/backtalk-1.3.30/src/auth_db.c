/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Hash File Authentication Database Interface
 *
 * This module supports an authentication database formatted as required by
 * Apache mod_auth_db and mod_auth_dbm.  The AUTH_DBM file uses the user's
 * login name (not including the terminating null) as a key, and contains
 * the encrypted user passwords (including a terminating null).
 *
 * If SHARE_AUTH_IDENT is defined, the same file also contains ident
 * information. The key is still the same, but the data is a newline and
 * null terminated string like:
 *   <passwd>:<uid>:<gid>:<fullname>:<directory>:\n
 */


#include "common.h"
#include "authident.h"
#include "hashfile.h"

#include <ctype.h>

#include "set_uid.h"
#include "udb.h"
#include "str.h"
#include "lock.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_auth_module= "auth_db/auth_dbm (db="AUTH_DBM")";

/* AUTH_GETPASS - Get the encrypted password for a user.  Returns NULL if there
 * is no such user.
 */

#ifdef HAVEAUTH_GETPASS
char *auth_getpass(char *nam)
{
    static char pass[40];
    int rc;

    if (nam == NULL) return NULL;

    set_httpd();
    rc= fetchdbm(AUTH_DBM, nam, strlen(nam), pass, 40);
    set_cfadm();

    switch (rc)
    {
    case 0:  return pass;
    /*case 1:  die("auth_getpass cannot open db "AUTH_DBM" to read");*/
    default: return NULL;
    }
}
#endif


/* AUTH_WALK() - Get the first or next name out of the dbm file.
 */

#ifdef HAVEAUTH_WALK
char *auth_walk(int next)
{
    static char bf[MAX_LOGIN_LEN+1];

    if (next)
    {
	if (nextdbm(bf, MAX_LOGIN_LEN+1))
	    return NULL;
	else
	    return bf;
    }
    else
    {
	set_httpd();
	if (walkdbm(AUTH_DBM))
	{
	    set_cfadm();
	    die("auth_walk cannot open db "AUTH_DBM" to read");
	}
	set_cfadm();

	if (firstdbm(bf, MAX_LOGIN_LEN+1))
	    return NULL;
	else
	    return bf;
    }
}
#endif


/* AUTH_SEEK() - Seek to the named position in the dbm file.
 */

#ifdef HAVEAUTH_SEEK
void auth_seek(char *login)
{
    set_httpd();
    if (walkdbm(AUTH_DBM))
    {
	set_cfadm();
	die("auth_seek cannot open db "AUTH_DBM" to read");
    }
    set_cfadm();

    seekdbm(login, strlen(login));
}
#endif


/* AUTH_NEWPASS - change the user's password to the given value.  Pass in
 * the encrypted password.
 */

#ifdef HAVEAUTH_NEWPASS
#ifdef SHARE_AUTH_IDENT

void auth_newpass(struct passwd *pw, char *encpw)
{
    char bf[BFSZ];
    int rc;

    set_httpd();

    sprintf(bf,"%s:%d:%d:%s:%s:\n",
	    encpw, pw->pw_uid, pw->pw_gid,
	    pw->pw_gecos, pw->pw_dir);

    rc= storedbm(AUTH_DBM,
    		 pw->pw_name, strlen(pw->pw_name),
    		 bf, strlen(bf)+1,
    		 0);
    set_cfadm();

    if (rc) die("auth_newpass cannot open user database "AUTH_DBM);
}

#else

void auth_newpass(struct passwd *pw, char *encpw)
{
    int rc;

    set_httpd();

    rc= storedbm(AUTH_DBM,
    		 pw->pw_name, strlen(pw->pw_name),
    		 encpw, strlen(encpw)+1,
    		 0);
    set_cfadm();

    if (rc) die("auth_newpass cannot open user database "AUTH_DBM);
}
#endif /* !SHARE_AUTH_IDENT */
#endif /* HAVEAUTH_NEWPASS */


/* AUTH_ADD - Add an account to the authentication dtabase. */

#ifdef HAVEAUTH_ADD
#ifdef SHARE_AUTH_IDENT

void auth_add(char *lid, char *pw, int status,
    uid_t uid, gid_t gid, char *fn, char *dir)
{
    char bf[BFSZ];

    sprintf(bf,"%s:%d:%d:%s:%s:\n", mkpasswd(pw,status), uid, gid, fn, dir);

    set_httpd();
    if (storedbm(AUTH_DBM,
                 lid, strlen(lid),      /* don't include terminating NUL */
                 bf, strlen(bf)+1,      /* include terminating NUL */
                 1))                    /* create file if necessary */
    {
        set_cfadm();
        die("auth_add cannot open user database "AUTH_DBM" (%s)",errdbm());
    }
    set_cfadm();
}

#else

void auth_add(char *lid, char *pw, int status,
    uid_t uid, gid_t gid, char *fn, char *dir)
{
    char *data= mkpasswd(pw,status);

    set_httpd();
    if (storedbm(AUTH_DBM,
                 lid, strlen(lid),      /* Don't include terminating NUL */
                 data, strlen(data)+1,  /* Include terminating NUL */
                 1))                    /* Create file if necessary */
    {
        set_cfadm();
	die("auth_add cannot open user database "AUTH_DBM" (%s)",errdbm());
    }
    set_cfadm();
}

#endif /* !SHARE_AUTH_IDENT */
#endif /* HAVEAUTH_ADD */


/* AUTH_DEL - Delete the named user
 */

#ifdef HAVEAUTH_DEL
int auth_del(char *login)
{
    int rc;

    set_httpd();
    rc= deletedbm(AUTH_DBM, login, strlen(login), 0);
    set_cfadm();

    return rc;
}
#endif
