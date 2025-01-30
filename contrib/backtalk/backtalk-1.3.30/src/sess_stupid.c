/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Dummy session database for sites that simply put the user's login ID in
 * a cookie instead of using a session ID.  This is seriously stupid, because
 * anyone can masquerade as anyone else simply by changing their cookie.  This
 * module is in Backtalk only to support integration with foreign cgi's
 * written by people who don't care about security.
 */

#include "backtalk.h"
#include "sysdict.h"

#ifdef ID_SESSION	/* This is all used only with cookie authentication */
char *showopt_session_module= "sess_stupid";


/* NEW_SESSION - Create new session for the given user.  Really just returns
 * the user id that was passed in, since that is our <hmm> "session id".
 */

char *new_session(char *user, char *ip)
{
    return strdup(user);
}


/* SESSION_LOOKUP - Look up the given session id in the session file.  If it
 * is there, and the process ip (given here in ascii numbers-and-dots form)
 * matches, then update the last access time in the file and save the user
 * name in the "user" buffer (which should be at least MAX_LOGIN_LEN+1 bytes)
 * and return 0.  Otherwise return 1.
 *
 * OK, really we don't look anything up.  We just copy the sid into the user
 * buffer and always return 1.
 */

int session_lookup(char *sid, char *ip, char *user)
{
    strncpy(user, sid, MAX_LOGIN_LEN);
    user[MAX_LOGIN_LEN]= '\0';
    return 1;
}


/* SESSION_DELETE - Delete the given session ID from the session file, if
 * it exists.  It is not an error if it does not exist.
 *
 * Actually, this is a no-op.  Logging out is done by killing the cookie,
 * for what little good it does.
 */

void session_delete(char *sid)
{
}

#endif /* ID_SESSION */
