/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SQL Database Authentication Database Interface
 *
 * This module supports an authentication database stored in an SQL table.
 * Any of several different SQL servers are supported through the various
 * src/sql_*.c files.  The specific SQL commands used (and thus the table
 * and field names) are configured in the sql/<server>/<template> files.
 * Apache authentications modules like mod_auth_pgsql can be used with this.
 *
 * If SHARE_AUTH_IDENT is defined, the same database (not necessarily the
 * same table) also contains ident information (uid, gid, fullname, and
 * directory names).
 */

#include "common.h"
#include "authident.h"
#include "sql.h"
#include "sqlutil.h"
#include "sqlqry.h"
#include "udb.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_auth_module= "auth_sql";

/* AUTH_GETPASS - Get the encrypted password for a user.  Returns NULL if there
 * is no such user.  Returns an empty string if he has no password.
 */

#ifdef HAVEAUTH_GETPASS
char *auth_getpass(char *nam)
{
    static char pass[40];
    SQLhandle sqlh;
    int nrow;
    char *p;

    if (nam == NULL) return NULL;

    make_sql_connection();

    /* Construct and execute the query */
    sqlh= qry_auth_getpass(nam);

    /* Check number of result rows */
    if ((nrow= sql_nrows(sqlh)) == 0)
    {
	sql_got(sqlh);
	return NULL;
    }
    if (nrow > 1)
    	die("auth_getpass found more than one user with login id %s",nam);

    /* Get password from only result row */
    p= sql_fetchstr(sqlh, 0);
    if (p == NULL)
    	pass[0]= '\0';
    else
    {
    	strncpy(pass,p,39);
	pass[40]= '\0';
    }

    /* Free statement handle */
    sql_got(sqlh);

    return pass;
}
#endif


/* AUTH_WALK() - Get the first or next name out of the sql user list.
 */

static SQLhandle aw_sqlh= NULL;

#ifdef HAVEAUTH_WALK
char *auth_walk(int next)
{
    static char bf[MAX_LOGIN_LEN+1];
    char *nam;

    if (!next)
    {
	make_sql_connection();
	if (aw_sqlh != NULL) sql_got(aw_sqlh);
    	aw_sqlh= qry_auth_logins(0);
    }
    else if (aw_sqlh == NULL)
	die("auth_walk asked for next login before first login");

    if (sql_nextrow(aw_sqlh) && (nam= sql_fetchstr(aw_sqlh, 0)) != NULL)
    {
	strncpy(bf, nam, MAX_LOGIN_LEN);
	bf[MAX_LOGIN_LEN]= '\0';
	return bf;
    }
    else
    {
	/* No more rows */
	sql_got(aw_sqlh);
	aw_sqlh= NULL;
	return NULL;
    }
}
#endif


/* AUTH_SEEK() - Seek to the named position in the sql user database.
 */

#ifdef HAVEAUTH_SEEK
void auth_seek(char *login)
{
    if (aw_sqlh != NULL) sql_got(aw_sqlh);
    aw_sqlh= qry_auth_logins_after(login,0);
}
#endif


/* AUTH_NEWPASS - change the user's password to the given value.  Pass in
 * the encrypted password.
 */

#ifdef HAVEAUTH_NEWPASS
void auth_newpass(struct passwd *pw, char *encpw)
{
    make_sql_connection();
    qry_auth_newpass(pw->pw_name, encpw);
}
#endif /* HAVEAUTH_NEWPASS */


/* AUTH_ADD - Add an account to the authentication database. */

#ifdef HAVEAUTH_ADD
void auth_add(char *lid, char *pw, int status,
    uid_t uid, gid_t gid, char *fn, char *dir)
{
    make_sql_connection();
    qry_auth_add(lid, mkpasswd(pw,status), uid, gid, fn, dir);
}
#endif /* HAVEAUTH_ADD */


/* AUTH_DEL - Delete the named user
 */

#ifdef HAVEAUTH_DEL
int auth_del(char *login)
{
    SQLhandle sqlh;

    /* First see if he exists */
    sqlh= qry_auth_getpass(login);

    /* Check number of result rows */
    if (sql_nrows(sqlh) == 0)
    {
	sql_got(sqlh);
	return 1;
    }
    sql_got(sqlh);

    /* Then delete him */
    qry_auth_del(login);
    return 0;
}
#endif
