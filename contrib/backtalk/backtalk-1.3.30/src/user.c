/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* USER INFORMATION
 *
 * This file contains routines to access information about the user.  Mostly
 * it calls the lower level routines in udb.c, which in turn call whichever
 * authentication (auth_*.c) and identification (ident_*.c) modules are
 * linked in.
 */

#include "backtalk.h"

#include <pwd.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "user.h"
#include "udb.h"
#include "group.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


struct passwd *upw= NULL;	/* NULL means user is anonymous */
char *new_fname= NULL;		/* If fullname has changed, this is it */


/* DUP_PASSWD - Given a pointer to a 'struct passwd' duplicate it into
 * dynamically allocated memory and return a pointer to the duplicate.
 */

struct passwd *dup_passwd(struct passwd *old)
{
    struct passwd *new;

    if (old == NULL) return NULL;

    new= (struct passwd *)malloc(sizeof(struct passwd));

    new->pw_name= old->pw_name ? strdup(old->pw_name) : NULL;
    new->pw_passwd= old->pw_passwd ? strdup(old->pw_passwd) : NULL;
    new->pw_uid= old->pw_uid;
    new->pw_gid= old->pw_gid;
    new->pw_gecos= old->pw_gecos ? strdup(old->pw_gecos) : NULL;
    new->pw_dir= old->pw_dir ? strdup(old->pw_dir) : NULL;
#if 0
    new->pw_shell= old->pw_shell ? strdup(old->pw_shell) : NULL;
#else
    new->pw_shell= NULL;
#endif
    return new;
}


/* GETUSER - Identify the user and save his database informtion into the
 * upw variable.  On failure, we set upw to NULL.  We return a error code:
 *   0 - success
 *   1 - failure - no authentication - should treat as anonymous user
 *   2 - failure - authentication expired - should re-authenticate
 * This must be called before any call to user_fname(), user_id(), user_dir(),
 * user_uid() or user_gid() and before calls to func_changename(),
 * func_changepass() or func_removeuser() which apply to the current user.
 *
 * This supports several different methods of identifying the user:
 *   ID_REMOTE_USER - read user ID from 'REMOTE_USER' env variable.
 *   ID_SESSION - get session ID from cookie and look up user in session db.
 *   ID_GETUID - person running backtalk is the user.
 * More than one of these can be turned on, in which case they are attempted
 * in the order listed.  If the global variable direct_exec is turned on,
 * we won't do the first two.  If we do use ID_GETID, the global direct_exec
 * gets turned on.
 *
 * If secure is true, then we don't trust environment variables unless we
 * are running as either HTTPD or root.
 */

int getuser(int secure)
{
    char *nam;
    uid_t realuid;
    int rc= 1;
#ifdef ID_SESSION
    char bf[MAX_LOGIN_LEN+1];
#endif

    realuid= getuid();
    upw= NULL;		/* Yes, this is needed! */

#if defined(ID_REMOTE_USER) || defined(ID_SESSION)
    /* Try getting ID from environment, if environment is trustworthy */
    if ((!secure || realuid == SERVER_UID || realuid == 0) && !direct_exec)
    {
#ifdef ID_REMOTE_USER
	/* Try getting ID from REMOTE_USER variable */
	nam= getenv("REMOTE_USER");
	if (nam == NULL)
#endif
	{
#ifdef ID_SESSION
	    /* Try getting ID from session cookie */
	    if ((rc= session_auth(nam= bf)) != 0)
#endif
		/* No user */
		return rc;
	}

	/* Get more user info */
	upw= dup_passwd(getdbnam(nam));
#ifdef AUTO_CREATE_DIR
	/* Create him a home directory if he doesn't have one */
	if (upw != NULL && access(upw->pw_dir,F_OK))
	{
	    char bf[BFSZ];
	    mk_directory(bf, nam);
	}
#endif
	return (upw == NULL);
    }
#endif /* ID_REMOTE_USER || ID_SESSION */

    /* Running 'backtalk -d' or not running secure Backtalk as HTTPD */

#ifdef ID_GETUID
    /* Try getting user based on UID number */
#ifdef UID_MAP_FILE
    /* So obsolete we never even tested it */
    if ((nam= mapuid(realuid)) != NULL)
	upw= dup_passwd(getdbnam(nam));
#else
    upw= dup_passwd(getdbuid(realuid));
#endif /* UID_MAP_FILE */
    direct_exec= TRUE;
#endif /* ID_GETUID */

#ifdef AUTO_CREATE_DIR
    /* Create him a home directory if he doesn't have one */
    if (upw != NULL && access(upw->pw_dir,F_OK))
    {
	char bf[BFSZ];
	mk_directory(bf, nam);
    }
#endif

    return (upw == NULL);
}



/* USER_FNAME - Return full name of the user.  Returns "" if the user is
 * anonymous.
 */

char *user_fname()
{
    return (new_fname == NULL ?
		(upw == NULL ?  "" : upw->pw_gecos) : new_fname);
}


/* USER_ID - Return login id of the user.  Returns "" if the user is anonymous.
 */

char *user_id()
{
    return (upw == NULL ? "" : upw->pw_name);
}


/* USER_DIR - Return home directory of the user.  Returns "" if the user is
 * anonymous.
 */

char *user_dir()
{
    return (upw == NULL ? "" : upw->pw_dir);
}


/* USER_UID - Return uid of the user.  Returns -1 if he is anonymous.
 */

int user_uid()
{
    return (upw == NULL ? -1 : upw->pw_uid);
}


/* USER_GID - Return primary group id of the user.  Returns -1 if he is
 * anonymous.
 */

int user_gid()
{
    return (upw == NULL ? -1 : upw->pw_gid);
}

/* USER_GNAME - Return the primary group name for the user.  Returns NULL if
 * he is anonymous.
 */

char *user_gname()
{
    static char *gname= NULL;
    int gid= user_gid();

    if (gname != NULL) return gname;
    if (gid == -1) return NULL;
    return gname= groupname(gid);
}


/* USER_CFADM - Return true if the user is in the CFADM group  Return value
 * if 1 if it is our primary group number, 2 if it is a secondary group,
 * and 0 if it is neither.
 */

int user_cfadm()
{
    if (upw == NULL) return 0;
#ifdef CFADM_GID
    return ingroupno(CFADM_GID,upw->pw_gid,upw->pw_name);
#else
#ifdef CFADM_GROUP
    return ingroupname(CFADM_GROUP,upw->pw_gid,upw->pw_name);
#else
    return 0;
#endif
#endif
}


/* USER_GRADM - Return true if the user is in the GRADM group.  Return
 * value is 1 if it is our primary group number, 2 if it is a secondary group,
 * and 0 if it is neither.
 */

int user_gradm()
{
    if (upw == NULL) return 0;
#ifdef GRADM_GID
    return ingroupno(GRADM_GID,upw->pw_gid,upw->pw_name);
#else
#ifdef GRADM_GROUP
    return ingroupname(GRADM_GROUP,upw->pw_gid,upw->pw_name);
#else
    return 0;
#endif
#endif
}

/* FREE_USER - Deallocate memory for user.  Used for memory debugging only.
 */

#ifdef CLEANUP
void free_user()
{
    if (upw->pw_name != NULL) free(upw->pw_name);
    if (upw->pw_passwd != NULL) free(upw->pw_passwd);
    if (upw->pw_gecos != NULL) free(upw->pw_gecos);
    if (upw->pw_dir != NULL) free(upw->pw_dir);
    if (upw != NULL) free(upw);
    upw= NULL;
}
#endif
