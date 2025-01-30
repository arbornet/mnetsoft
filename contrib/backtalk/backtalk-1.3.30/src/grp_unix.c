/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* ROUTINES TO READ THE UNIX GROUP FILE */

#include "backtalk.h"
#include "group.h"

#include <grp.h>
#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_group_module= "grp_unix";

/* GROUPNAME - Given a gid number, return the matching group name.  Return
 * NULL if not defined.  Returned name should be treated as being in static
 * memory - future calls may overwrite it.
 *
 * Unix version.
 */

char *groupname(gid_t gid)
{
    struct group *gr;

    setgrent();
    while ((gr= getgrent()) != NULL)
    {
    	if (gr->gr_gid == gid)
	{
	    endgrent();
	    return gr->gr_name;
	}
    }
    endgrent();
    return NULL;
}


/* GROUPID - Given a group name, return the matching group id.  Return
 * -1 if not defined.
 *
 * Unix version.
 */

int groupid(char *gname)
{
    struct group *gr;

    setgrent();
    while ((gr= getgrent()) != NULL)
    {
    	if (!strcmp(gr->gr_name,gname))
	{
	    endgrent();
	    return gr->gr_gid;
	}
    }
    endgrent();
    return -1;
}


/* INGROUPNO - Given a group number return true if the given user with the
 * given primary group id is in that group.
 *
 * Unix version.
 */

int ingroupno(gid_t gid, gid_t my_gid, char *my_login)
{
    struct group *gr;
    char **m;

    if (my_login == NULL || my_login[0] == '\0') return 0;
    if (gid == my_gid) return 1;

    setgrent();
    while ((gr= getgrent()) != NULL)
    {
    	if (gr->gr_gid == gid)
    	{
	    for (m= gr->gr_mem; *m != NULL; m++)
	    {
	    	if (!strcmp(*m, my_login))
		{
		    endgrent();
		    return 2;
	        }
	    }
	    endgrent();
	    return 0;
	}
    }
    endgrent();
    return 0;
}


/* INGROUPNAME - Given a group name return true if the given user with the 
 * given group id is in that group.
 *
 * Unix version.
 */

int ingroupname(char *groupname, gid_t my_gid, char *my_login)
{
    struct group *gr;
    char **m;

    if (groupname == NULL || groupname[0] == '\0') return 0;
    if (my_login == NULL || my_login[0] == '\0') return 0;

    setgrent();
    while ((gr= getgrent()) != NULL)
    {
    	if (!strcmp(groupname, gr->gr_name))
    	{
	    if (gr->gr_gid == my_gid) return 1;
	    for (m= gr->gr_mem; *m != NULL; m++)
	    {
	    	if (!strcmp(*m, my_login))
		{
		    endgrent();
		    return 2;
	        }
	    }
	    endgrent();
	    return 0;
	}
    }
    endgrent();
    return 0;
}


/* INGROUPLIST - Given a NULL-terminated array of pointers to group names and
 * an ID, return true if that id is in any of those groups.
 *
 * Unix version.
 */

int ingrouplist(char **groups, gid_t my_gid, char *my_login)
{
    struct group *gr;
    char **m;
    int i;

    if (groups == NULL || groups[0] == NULL) return 0;

    setgrent();
    while ((gr= getgrent()) != NULL)
    {
	for (i= 0; groups[i] != NULL; i++)
	{
	    if (!strcmp(groups[i], gr->gr_name))
	    {
		if (gr->gr_gid == my_gid) return 1;
		for (m= gr->gr_mem; *m != NULL; m++)
		{
		    if (!strcmp(*m, my_login))
		    {
			endgrent();
			return 1;
		    }
		}
	    }
	}
    }
    endgrent();
    return 0;
}


/* FOR_GROUPS - Call a given function once with each currently defined group
 * name.
 *
 * Unix version.
 */

void for_groups(void (*func)(char *))
{
    struct group *gr;

    setgrent();
    while ((gr= getgrent()) != NULL)
    	(*func)(gr->gr_name);
    endgrent();
}
