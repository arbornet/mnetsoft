/* (c) 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* No Authentication Database Interface
 *
 * This module is used when no authentication database is wanted.  This can
 * happen if you are doing form-based logins but using some other program to
 * handle logins and set the cookies.  Backtalk only needs to be able to
 * check session IDs, not logins.
 *
 * In this case, there is no database, and all attempts to login fail.  We
 * can't create accounts or change passwords.
 */


#include "common.h"
#include "authident.h"

char *showopt_auth_module= "auth_none";

/* AUTH_GETPASS - Get the encrypted password for a user.  Returns NULL if there
 * is no such user, which is always the case.
 */

#ifdef HAVEAUTH_GETPASS
char *auth_getpass(char *nam)
{
    return NULL;
}
#endif
