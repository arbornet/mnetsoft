/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Routines to maintain a session database on an SQL server */

#include "backtalk.h"
#include "sysdict.h"
#include "entropy.h"

#ifdef ID_SESSION	/* This is all used only with cookie authentication */
char *showopt_session_module= "sess_sql";

#include "sess.h"
#include "udb.h"
#include "sql.h"
#include "sqlutil.h"
#include "sqlqry.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* NEW_SESSION - Create new session for the given user.  Pass in a encrypted
 * password and the ip address (in ascii numbers-and-dots form) he connected
 * from.  You should already have checked that it is a valid login.  This will
 * create the session entry in the session database and return the session id
 * in malloced memory.  The caller should set the cookie.
 */

char *new_session(char *user, char *ip)
{
    time_t now= time(0L);
    char *sid;

    /* Generate the new session id and make a copy */
    sid= strdup(get_noise());

    /* First reap any old sessions */
    qry_sess_reap(now - idict(VAR_EXPIRE_SESSION));

    /* Then create the new one */
    qry_sess_new(sid, user, now, ip);

    return sid;
}


/* SESSION_LOOKUP - Look up the given session id in the session file.  If it
 * is there, and the process id (given here in ascii numbers-and-dots form)
 * matches, then update the last access time in the file and save the user
 * name in the "user" buffer (which should be at least MAX_LOGIN_LEN+1 bytes)
 * and return 0.  Otherwise return 1.
 */

int session_lookup(char *sid, char *ip, char *user)
{
    SQLhandle sqlh;
    char *usr,*ltm,*lip;
    time_t now= time(0L);

    make_sql_connection();

    sqlh= qry_sess_get(sid);
    if (sql_nrows(sqlh) == 0)
    	return 1;

    /* Trap bad data */
    if ((usr= sql_fetchstr(sqlh,0)) == NULL ||
        (ltm= sql_fetchstr(sqlh,1)) == NULL ||
        (lip= sql_fetchstr(sqlh,2)) == NULL)
    	return 1;

    /* Trap expired sessions */
    if (atol(ltm) + idict(VAR_EXPIRE_SESSION) < now)
    	return 1;

#ifdef CHECK_SESSION_IP
    /* Trap IP mismatches */
    if (strcmp(ip,lip))
    	return 1;
#endif

    /* User is OK */
    strncpy(user,usr,MAX_LOGIN_LEN);
    user[MAX_LOGIN_LEN]= '\0';

    qry_sess_refresh(sid, now);

    return 0;
}


/* SESSION_DELETE - Delete the given session ID from the session file, if
 * it exists.  It is not an error if it does not exist.
 */

void session_delete(char *sid)
{
    make_sql_connection();
    qry_sess_kill(sid);
}

#endif /* ID_SESSION */
