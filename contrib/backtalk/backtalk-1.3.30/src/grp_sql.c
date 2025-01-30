/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* ROUTINES TO READ THE UNIX GROUP FILE */

#include "common.h"
#include "group.h"
#include "sql.h"
#include "sqlqry.h"
#include "sqlutil.h"
#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char *showopt_group_module= "grp_sql";

/* GROUPNAME - Given a gid number, return the matching group name.  Return
 * NULL if not defined.  Returned name should be treated as being in static
 * memory - future calls may overwrite it.
 *
 * SQL version.
 */

char *groupname(gid_t gid)
{
    static char bf[MAX_LOGIN_LEN+1];
    SQLhandle sqlh;
    int nrow;
    char *p;

    make_sql_connection();

    /* Construct and execute the query */
    sqlh= qry_group_gid_to_name(gid);

    /* Check number of result rows */
    if ((nrow= sql_nrows(sqlh)) == 0)
    {
	sql_got(sqlh);
	return NULL;
    }
    if (nrow > 1)
        die("groupname found more than one group with gid %d",gid);

    /* Get password from only result row */
    p= sql_fetchstr(sqlh, 0);
    if (p == NULL)
    {
	sql_got(sqlh);
	return NULL;
    }
    strncpy(bf,p,MAX_LOGIN_LEN);
    bf[MAX_LOGIN_LEN]= '\0';

    sql_got(sqlh);
    return bf;
}


/* GROUPID - Given a group name, return the matching group id.  Return
 * -1 if not defined.
 *
 * SQL version.
 */

int groupid(char *gname)
{
    SQLhandle sqlh;
    int nrow;
    gid_t gid;
    char *p;

    make_sql_connection();

    /* Construct and execute the query */
    sqlh= qry_group_name_to_gid(gname);

    /* Check number of result rows */
    if ((nrow= sql_nrows(sqlh)) == 0)
    {
	sql_got(sqlh);
	return -1;
    }
    if (nrow > 1)
        die("groupid found more than one group with name %s",gname);

    /* Get password from only result row */
    p= sql_fetchstr(sqlh, 0);
    if (p == NULL)
    {
	sql_got(sqlh);
	return -1;
    }
    gid= atoi(p);
    sql_got(sqlh);
    return gid;
}


/* INGROUPNO - Given a group number return true if the given user with the
 * given primary group id is in that group.
 *
 * SQL version.
 */

int ingroupno(gid_t gid, gid_t my_gid, char *my_login)
{
    SQLhandle sqlh;
    int rc;

    if (my_login == NULL || my_login[0] == '\0') return 0;
    if (gid == my_gid) return 1;

    make_sql_connection();

    /* Construct and execute the query */
    sqlh= qry_group_in_gid(my_login,gid);

    /* If any rows are returned, then we are in the group */
    rc= (sql_nrows(sqlh) > 0) ? 2 : 0;
    sql_got(sqlh);
    return rc;
}


/* INGROUPNAME - Given a group name return true if the given user with the 
 * given group id is in that group.  Actually ignores the gid passed in.
 *
 * SQL version.
 */

int ingroupname(char *groupname, gid_t my_gid, char *my_login)
{
    SQLhandle sqlh;
    int rc;

    if (groupname == NULL || groupname[0] == '\0') return 0;
    if (my_login == NULL || my_login[0] == '\0') return 0;

    make_sql_connection();

    /* Construct and execute the query */
    sqlh= qry_group_in_name(my_login,groupname);

    /* If any rows are returned, then we are in the group */
    rc= (sql_nrows(sqlh) > 0);
    sql_got(sqlh);
    return rc;
}


/* INGROUPLIST - Given a NULL-terminated array of pointers to group names and
 * an ID, return true if that id is in any of those groups.
 *
 * SQL version.
 */

int ingrouplist(char **groups, gid_t my_gid, char *my_login)
{
    SQLhandle sqlh;
    char *grp;
    int i;

    if (groups == NULL || groups[0] == NULL) return 0;

    make_sql_connection();

    sqlh= qry_group_mine(my_login,0);

    while (sql_nextrow(sqlh))
    {
	grp= sql_fetchstr(sqlh, 0);
	if (grp == NULL) continue;

	for (i= 0; groups[i] != NULL; i++)
	{
	    if (!strcmp(groups[i], grp))
	    {
	    	sql_got(sqlh);
		return 1;
	    }
	}
    }
    sql_got(sqlh);
    return 0;
}


/* FOR_GROUPS - Call a given function once with each currently defined group
 * name.
 *
 * SQL version.
 */

void for_groups(void (*func)(char *))
{
    SQLhandle sqlh;
    char *grp;
    int i;

    make_sql_connection();

    sqlh= qry_group_all(0);

    while (sql_nextrow(sqlh))
    {
	grp= sql_fetchstr(sqlh, 0);
	if (grp == NULL) continue;
	(*func)(grp);
    }
    sql_got(sqlh);
}


#if !NOPWEDIT

/* NEWGROUP(gname)
 *   Creates a new group with the given name.  Returns 0 on success, 1 if
 *   the group already exists.  Group ID is automatically generated.
 */

int newgroup(char *gname)
{
    SQLhandle sqlh;
    gid_t gid, my_gid= 100;
    char *grp;

    make_sql_connection();

    /* Look for an unused group id */
    sqlh= qry_group_all(0);
    while (sql_nextrow(sqlh))
    {
    	if (!strcmp(gname, sql_fetchstr(sqlh, 0)))
	{
	    sql_got(sqlh);
	    return 1;
	}
	gid= atoi(sql_fetchstr(sqlh, 1));
	if (gid > my_gid) my_gid= gid;
    }
    sql_got(sqlh);
    my_gid++;

    /* Create the group */
    qry_group_add(my_gid, gname);

    return 0;
}


/* NEWGROUPMEM(gname,member)
 *   Add a user to an existing group.  Returns 0 on success, 1 if the group
 *   does not exists.
 */

int newgroupmem(char *gname, char *mem)
{
    SQLhandle sqlh;
    gid_t gid;

    make_sql_connection();

    sqlh= qry_group_name_to_gid(gname);
    if (sql_nrows(sqlh) <= 0)
    {
    	sql_got(sqlh);
	return 1;
    }
    gid= atoi(sql_fetchstr(sqlh, 0));
    sql_got(sqlh);

    qry_group_memadd(gid, mem);
    return 0;
}


/* DELGROUP(gname)
 *   Delete a group and all its members.  Returns 0 on success.
 */

int delgroup(char *gname)
{
    SQLhandle sqlh;

    make_sql_connection();

    sqlh= qry_group_name_to_gid(gname);
    if (sql_nrows(sqlh) <= 0)
    {
    	sql_got(sqlh);
	return 1;
    }
    sql_got(sqlh);

    qry_group_del(gname);
    return 0;
}


/* DELGROUPMEM(gname, member)
 *   Remove a user from a group.  Returns 0 on success, 1 if the group
 *   does not exists or the user was not a member of it.
 */

int delgroupmem(char *gname, char *member)
{
    if (!ingroupname(gname, 0, member))
    	return 1;

    qry_group_memdel(gname, member);
    return 0;
}

#endif /* !NOPWEDIT */
