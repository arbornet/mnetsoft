/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* USER INFORMATION FUNCTIONS
 */


#include "backtalk.h"

#include <pwd.h>

#include "sysdict.h"
#include "last_log.h"
#include "stack.h"
#include "free.h"
#include "user.h"
#include "userfunc.h"
#include "udb.h"
#include "group.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* FUNC_USERINFO:  <id> userinfo <uid> <gid> <fullname> <dir> <laston> <status>
 *
 * Load info about an arbitrary user.  Most fields are 0 or () if the user
 * does not exist, however status is -1 if the user doesn't exist.
 */

void func_userinfo()
{
    char *name= pop_string();
    char *pass;
    struct passwd *pw;

    pw= getdbnam(name);
    if (pw == NULL)
    {
	push_integer(-1);
	push_integer(-1);
	push_string("",TRUE);
	push_string("",TRUE);
	push_time(0);
	/* User without ident entry could still have an auth entry */
	pass= getpassword(name,TRUE);
	if (pass == NULL || pass[0] == '\0')
	    push_integer(-1);
	else if (pass[0] == '*')
	    push_integer(2);
	else
	    push_integer(pass[0] == '?');
    }
    else
    {
	time_t laston= get_lastlog(pw->pw_uid);

	push_integer(pw->pw_uid);
	push_integer(pw->pw_gid);
	if (bdict(VAR_MAYSEEFNAME) || idict(VAR_UID) == pw->pw_uid)
	    push_string(pw->pw_gecos,TRUE);
	else
	    push_string("",TRUE);
	push_string(pw->pw_dir,TRUE);
	push_time(laston);

	pass= getpassword(name,FALSE);
	if (pass == NULL || pass[0] == '*')
	    push_integer(2);
	else
	    push_integer(pass[0] == '?');
    }
}


/* FUNC_USEREMAIL:  <id> useremail <email>
 *
 * Load email address for an arbitrary user.  Result is () if the identity
 * database does not store email addresses, which is usually.
 */

void func_useremail()
{
    char *name= pop_string();
    struct passwd *pw;

    if (!save_email() || (pw= getdbnam(name)) == NULL)
	push_string("",TRUE);
    else
	push_string(pw->pw_shell,TRUE);
    free(name);
}


/* FUNC_FIRSTUSER: - firstuser <username>
 * This returns the name of the "first" user in the user database.  It can
 * be used, together with "nextuser" to walk through the list of users.  If
 * there are no users, an empty string is pushed.
 */

void func_firstuser()
{
    char *login;

    if ((login= firstusr()) == NULL)
	push_string("",TRUE);
    else
	push_string(login,TRUE);
}


/* FUNC_NEXTUSER: - nextuser <username>
 * This returns the name of the "next" user in the user database.  It can
 * be used, together with "firstuser" to walk through the list of users.  If
 * there are no more users, an empty string is pushed.
 */

void func_nextuser()
{
    char *login;

    if ((login= nextusr()) == NULL)
	push_string("",TRUE);
    else
	push_string(login,TRUE);
}


/* FUNC_SEEKUSER: <username> seekuser -
 * This sets the user database so that the next call to "nextuser" will return
 * the user *after* the named user.  It is somewhat slow on some types of
 * databases.
 */

void func_seekuser()
{
    char *login= pop_string();
    seekusr(login);
    free(login);
}


/* FUNC_GROUPNAME: <gid> groupname <groupname>
 *
 * Return the group name associated with the given group ID number.
 */

void func_groupname()
{
    char *gn;

    gn= groupname(pop_integer());
    if (gn == NULL)
	push_string("",TRUE);
    else
	push_string(gn,TRUE);
}


/* FUNC_GROUPID: <groupname> groupid <gid>
 *
 * Return the gid number associated with the given group name.
 */

void func_groupid()
{
    char *gname;

    gname= pop_string();
    if (gname[0] >= '0' && gname[0] <= '9')
    	push_integer(atoi(gname));
    else
	push_integer(groupid(gname));
    free(gname);
}


/* FUNC_INGROUP: <group> <userid> ingroup <boolean>
 *
 * Return true if given user is in the given group.  If the user is (), then
 * the currently logged in user is checked.  The group can be given as a name
 * or number, and the number can be in integer or string format.
 */

void func_ingroup()
{
    Token tk;
    char *ulogin= pop_string();
    int fu= 1;
    struct passwd *pw;
    gid_t ugid;

    pop_any(&tk);

    if (*ulogin == '\0')
    {
	if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	{
	    /* No current user */
	    push_integer(0);
	    free(ulogin);
	    free_val(&tk);
	    return;
	}
        else
	{
	    /* Check current user */
	    free(ulogin);  fu= 0;
	    ulogin= user_id();
	    ugid= user_gid();
	}
    }
    else
    {
    	/* Check given user */
	pw= getdbnam(ulogin);
	if (pw == NULL)
	{
	    /* No such user */
	    push_integer(0);
	    if (fu) free(ulogin);
	    free_val(&tk);
	    return;
	}
	ugid= pw->pw_gid;
    }

    if (type(tk) == TK_INTEGER)
	push_integer(ingroupno(ival(tk),ugid,ulogin));
    else if (type(tk) == TK_STRING)
    {
	char *s= sval(tk);
	if (s[0] >= '0' && s[0] <= '9')
	    push_integer(ingroupno(atoi(s),ugid,ulogin));
	else
	    push_integer(ingroupname(s,ugid,ulogin));
    }
    else
    	die("expected string or integer argument");

    if (fu) free(ulogin);
    free_val(&tk);
}


/* FUNC_GROUPS: - groups <groupname1> <groupname2> ...
 *
 * Push all known group names on the stack.
 */

void push_str_copy(char *name) { push_string(name,TRUE); }

void func_groups()
{
    for_groups(&push_str_copy);
}


/* FUNC_DFLTGROUP: - dfltgroup <groupname>
 *
 * Push the name of the default user group.  Pushs () if there is none.
 */

void func_dfltgroup()
{
    char *g= user_group_name();

    push_string(g ? g : "",TRUE);
}


/* FUNC_NEWGROUP:   <groupname> newgroup <flag>
 *   Creates a new group with the given name.  Returns 0 on success, 1 if
 *   the group already exists.
 */

void func_newgroup()
{
#if NOPWEDIT
    die("Group creation not enabled in this installation");
#else
    char *gname= pop_string();

    if (!bdict(VAR_AMADM))
    	die("Only conference administrator may create new groups");

    push_integer(newgroup(gname));

    free(gname);
#endif /*NOPWEDIT*/
}


/* FUNC_NEWGROUPMEM:   <gname> <member> newgroupmem <flag>
 *   Add a user to an existing group.  Returns 0 on success, 1 if the group
 *   does not exists.
 */

void func_newgroupmem()
{
#if NOPWEDIT
    die("Changing group membership not enabled in this installation");
#else
    char *mem= pop_string();
    char *gname= pop_string();

    if (!bdict(VAR_AMADM))
    	die("Only conference administrator may add users to groups");

    push_integer(newgroupmem(gname,mem));

    free(mem);
    free(gname);
#endif /*NOPWEDIT*/
}


/* FUNC_DELGROUP:   <groupname> delgroup <flag>
 *   Delete a group and all its members.
 */

void func_delgroup()
{
#if NOPWEDIT
    die("Group deletion not enabled in this installation");
#else
    char *gname= pop_string();
    struct grp *groups;

    if (!bdict(VAR_AMADM))
    	die("Only conference administrator may remove groups");

    push_integer(delgroup(gname));

    free(gname);
#endif /*NOPWEDIT*/
}


/* FUNC_DELGROUPMEM:   <gname> <member> delgroupmem <flag>
 *   Remove a user from a group.  Returns 0 on success, 1 if the group
 *   does not exists or the user was not a member of it.
 */

void func_delgroupmem()
{
#if NOPWEDIT
    die("Changing group membership not enabled in this installation");
#else
    char *mem= pop_string();
    char *gname= pop_string();

    if (!bdict(VAR_AMADM))
    	die("Only conference administrator may remove users from groups");

    push_integer(delgroupmem(gname,mem));
    free(mem);
    free(gname);
#endif /*NOPWEDIT*/
}
