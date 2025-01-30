/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SQL Identification Database Interface
 *
 * This module supports a user identification database stored in an SQL
 * table (or multiple SQL tables that can be joined into a single table).
 * Any of several different SQL servers are supported through the various
 * src/sql_*.c files.  The specific SQL commands used (and thus the table
 * and field names) are configured in the sql/<server>/<template> files.
 *
 * If SHARE_AUTH_IDENT is defined, the same database (not necessarily the
 * same table) also contains authentication information (mainly a password).
 */


#include "common.h"
#include "authident.h"
#include "sql.h"
#include "sqlutil.h"
#include "sqlqry.h"

#include <ctype.h>

#include "udb.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_ident_module= "ident_sql";

/* After the SQL query has been submitted, load the returned data into
 * a struct passwd.  Type is
 *   0 = getpwnam - nam is passed in
 *   1 = getpwuid - uid is passed in
 *   2 = users - neither is passed in
 */

#if defined(HAVEIDENT_GETPWNAM) || defined(HAVEIDENT_GETPWUID) || defined(HAVEIDENT_WALKPW)
struct passwd *ident_fetchpw(SQLhandle sqlh, int type, char *nam, uid_t uid)
{
    static struct passwd pw;
    static char bf[BFSZ+1];
    char *d;
    int i, fn= 0;
    bf[BFSZ]= '\0';

    if (type == 0)
    {
	/* Copy name into static storage */
	strncpy(bf,nam,BFSZ);
    }
    else
    {
	/* Get name from SQL */
	d= sql_fetchstr(sqlh, fn++);
	strncpy(bf,d,BFSZ);
    }
    pw.pw_name= bf;
    i= strlen(bf)+1;

    /* Copy encrypted password into static storage */
    d= sql_fetchstr(sqlh, fn++);
    if (d == NULL)
    	pw.pw_passwd= NULL;
    else
    {
	strncpy(bf+i,d,BFSZ-i);
	pw.pw_passwd= bf+i;
	i+= strlen(bf+i)+1;
    }

    /* Get uid */
    if (type == 1)
	/* Use uid passed in */
	pw.pw_uid= uid;
    else
	/* Get uid from database */
	pw.pw_uid= atoi(sql_fetchstr(sqlh,fn++));


    /* Get gid */
    pw.pw_gid= atoi(sql_fetchstr(sqlh,fn++));
    
    /* Get fullname */
    d= sql_fetchstr(sqlh, fn++);
    strncpy(bf+i,d,BFSZ-i);
    pw.pw_gecos= bf+i;
    i+= strlen(bf+i)+1;
    
    /* Get directory */
    d= sql_fetchstr(sqlh, fn++);
    if (d == NULL)
  	pw.pw_dir= NULL;
    else
    {
	strncpy(bf+i,d,BFSZ-i);
	pw.pw_dir= bf+i;
    }

    return &pw;
}
#endif


/* IDENT_GETPWNAM - Get user data from the SQL server.  Save info into a
 * standard passwd structure in static storage.  Return NULL on failure.
 */

#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *nam)
{
    SQLhandle sqlh;
    int nrow;

    if (nam == NULL) return NULL;

    make_sql_connection();

    /* Do the SQL query */
    sqlh= qry_ident_getpwnam(nam);

    /* Check number of result rows */
    if ((nrow= sql_nrows(sqlh)) == 0)
    {
    	sql_got(sqlh);
	return NULL;
    }
    if (nrow > 1)
    	die("ident_getpwnam found more than one user with login id %s",nam);

    return ident_fetchpw(sqlh,0,nam,0);
}
#endif


/* GETIDENTUID - Get user data from the SQL server.  Save info into a
 * standard passwd structure in static storage.  Return NULL on failure.
 */

#ifdef HAVEIDENT_GETPWUID
struct passwd *ident_getpwuid(uid_t uid)
{
    SQLhandle sqlh;

    make_sql_connection();

    /* Do the SQL query */
    sqlh= qry_ident_getpwuid(uid);

    /* Check number of result rows */
    if ((nrow= sql_nrows(sqlh)) == 0)
    {
    	sql_got(sqlh);
	return NULL;
    }
    if (nrow > 1)
    	die("ident_getpwnam found more than one user with login id %s",nam);

    return ident_fetchpw(sqlh,1,NULL,uid);
}
#endif


/* IDENT_WALKPW - Get first (if next == 0) or next user entry out of sql
 * ident database.
 */


#ifdef HAVEIDENT_WALKPW
SQLhandle iwp_sqlh= NULL;

struct passwd *ident_walkpw(int next)
{
    if (!next)
    {
	make_sql_connection();
	if (iwp_sqlh != NULL) sql_got(iwp_sqlh);
	iwp_sqlh= qry_ident_users(0);
    }
    else if (iwp_sqlh == NULL)
	die("ident_walkpw asked for next login before first login");

    if (sql_nextrow(iwp_sqlh))
	return ident_fetchpw(iwp_sqlh,2,NULL,-1);
    else
    {
	sql_got(iwp_sqlh);
	iwp_sqlh= NULL;
	return NULL;
    }
}
#endif


/* IDENT_WALK - Get first (if next == 0) or next login id out of sql ident
 * database.
 */

SQLhandle iw_sqlh= NULL;

#ifdef HAVEIDENT_WALK
char *ident_walk(int next)
{
    static char bf[MAX_LOGIN_LEN+1];
    char *nam;

    if (!next)
    {
	make_sql_connection();
	if (iw_sqlh != NULL) sql_got(iw_sqlh);
	iw_sqlh= qry_ident_logins(0);
    }
    else if (iw_sqlh == NULL)
	die("ident_walk asked for next login before first login");

    if (sql_nextrow(iw_sqlh) && (nam= sql_fetchstr(iw_sqlh, 0)) != NULL)
    {
        strncpy(bf, nam, MAX_LOGIN_LEN);
        bf[MAX_LOGIN_LEN]= '\0';
        return bf;
    }
    else
    {
        /* No more rows */
        sql_got(iw_sqlh);
        iw_sqlh= NULL;
        return NULL;
    }
}
#endif


/* IDENT_SEEK - Set the scan to start after the named user.  Future
 * ident_walkpw(1)  calls will continue from there.
 */

#ifdef HAVEIDENT_SEEK
void ident_seek(char *login)
{
    make_sql_connection();
    if (iw_sqlh != NULL) sql_got(iw_sqlh);
    iw_sqlh= qry_ident_logins_after(login,0);
}
#endif


/* NEWFNAME - change the user's fullname to the given value.
 */

#ifdef HAVEIDENT_NEWFNAME
void ident_newfname(struct passwd *pw, char *newfname)
{
    make_sql_connection();
    qry_ident_newfname(pw->pw_name, newfname);
    flushusercache();
}
#endif /* HAVEIDENT_NEWFNAME */


/* IDENT_NEWGID - change the user's primary gid to the given value.
 */

#ifdef HAVEIDENT_NEWGID
void ident_newgid(struct passwd *pw, gid_t newgid)
{
    make_sql_connection();
    qry_ident_newgid(pw->pw_name, newgid);
    flushusercache();
}
#endif /* HAVEIDENT_NEWGID */


/* IDENT_ADD - Add an entry to the user database describing a user with
 * the given login, uid, gid, fullname, and directory.
 */

#ifdef HAVEIDENT_ADD
void ident_add(char *lid, uid_t uid, gid_t gid, char *fn, char *dir)
{
    make_sql_connection();
    qry_ident_add(lid, uid, gid, fn, dir);
}
#endif


/* IDENT_DEL - Delete the named user from the ident database.
 */

#ifdef HAVEIDENT_DEL
void ident_del(char *login)
{
    make_sql_connection();
    qry_ident_del(login);
    return;
}
#endif
