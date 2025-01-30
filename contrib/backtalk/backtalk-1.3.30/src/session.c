/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Routines to manage the session database and create session id numbers */

#include "backtalk.h"
#include "sysdict.h"

#ifdef ID_SESSION	/* This is all used only with cookie authentication */

#include "session.h"
#include "sess.h"
#include "stack.h"
#include "cgi_cookie.h"
#include "udb.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char my_sid[SESSION_ID_LEN+1]= "";


/* SESSION_AUTH - Authenticate a user by fetching his cookie and checking it.
 * If successful, the user name is stored in "user" (which should be at least
 * MAX_LOGIN_LEN+1 bytes), and we return 0.  If the user has a cookie which
 * is not in the session database, return 2.  Otherwise, return 1.
 */

int session_auth(char *user)
{
    char *sid;
    char *ip;

    /* Get IP number */
    if ((ip= getenv("REMOTE_ADDR")) == NULL)
	return 1;	/* weird */

    /* Get session cookie */
    if ((sid= get_cookie(SESSION_COOKIE)) == NULL)
	return 1;	/* not logged in */

    /* Look up session */
    if (session_lookup(sid,ip,user))
	return 2;	/* authentication expired */

    /* Keep a copy for future func_killsession() calls to use */
    strncpy(my_sid,sid,SESSION_ID_LEN);
    my_sid[SESSION_ID_LEN]= '\0';
    free(sid);

    return 0;
}


/* FUNC_NEWSESSION:  <login> <password> newsession <rc>
 * Start a new session for the given user.  If the user does not exist, or the
 * password is incorrect, this pushes TRUE.  If the login is correct, it
 * creates an entry in the session database and sets a cookie with the session
 * id and returns FALSE.  For the cookie setting to work correctly, this should
 * be done before the first print, pr, or dumpstack statement.
 */

void func_newsession()
{
    char *passwd= pop_string();
    char *login= pop_string();
    char *ip, *sid;

    /* Check the login */
    if (!checkpassword(login,passwd))
    	goto fail;

    /* Get IP address */
    if ((ip= getenv("REMOTE_ADDR")) == NULL)
	goto fail;

    /* save session ID */
    if ((sid= new_session(login,ip)) == NULL)
	goto fail;
    strcpy(my_sid, sid);

    /* set cookie */
    if (set_cookie(strdup(SESSION_COOKIE),sid,0))
    {
	free(sid);
	goto fail;
    }

    /* Success */
    free(passwd);
    free(login);
    /* Don't free sid - it part of cookie table */
    push_integer(0);
    return;

fail:	/* Failure */
    free(passwd);
    free(login);
    push_integer(1);
    return;
}


/* FUNC_KILLSESSION:  - killsession -
 * Invalidate the current session ID, if any.  This logs the current user out.
 * Also erases our cookie.  This will erase any leftover invalid session
 * cookies even if there is no valid session in progress.
 */

void func_killsession()
{
    char *sid;

    /* Set up http headers to delete cookie from browser - expiration date in
     * the past makes this delete */
    set_cookie(strdup(SESSION_COOKIE),strdup(""),1);

    /* Delete the cookie from the session database */
    if (my_sid[0] == '\0')
    {
	/* If we are running a public script we might never have tried to
	 * authenticate.  We want to try so that we can get the session ID
	 * and delete it.
	 */
	if ((sid= get_cookie(SESSION_COOKIE)) == NULL) return;
	session_delete(sid);
	free(sid);
    }
    else
	session_delete(my_sid);
}

#else

void func_newsession()
{
    /* Always fail if we don't have session */
    func_pop();
    func_pop();
    push_integer(1);
    return;
}

void func_killsession() {}

#endif /* ID_SESSION */
