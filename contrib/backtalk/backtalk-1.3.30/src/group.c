/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "group.h"
#include "sysdict.h"
#include "stack.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* USER_GROUP_NAME()
 * USER_GROUP_ID()
 * CFADM_GROUP_NAME()
 * CFADM_GROUP_ID()
 * GRADM_GROUP_NAME()
 * GRADM_GROUP_ID()
 *   Return names/IDs of interesting groups.  Sometimes these are macros
 *   defined in group.h.  Look up each thing only once, then save in a
 *   static variable.
 */

#if defined(USER_GID) && !defined(USER_GROUP)
char *user_group_name()
{
    static char *name= NULL;
    if (name == NULL) name= strdup(groupname(USER_GID));
    return name;
}
#endif

#if !defined(USER_GID) && defined(USER_GROUP)
int user_group_id()
{
    static int gid= -1;
    if (gid == -1) gid= groupid(USER_GROUP);
    return gid;
}
#endif

#if defined(CFADM_GID) && !defined(CFADM_GROUP)
char *cfadm_group_name()
{
    static char *name= NULL;
    if (name == NULL) name= strdup(groupname(CFADM_GID));
    return name;
}
#endif

#if !defined(CFADM_GID) && defined(CFADM_GROUP)
int cfadm_group_id()
{
    static int gid= -1;
    if (gid == -1) gid= groupid(CFADM_GROUP);
    return gid;
}
#endif

#if defined(GRADM_GID) && !defined(GRADM_GROUP)
char *gradm_group_name()
{
    static char *name= NULL;
    if (name == NULL) name= strdup(groupname(GRADM_GID));
    return name;
}
#endif

#if !defined(GRADM_GID) && defined(GRADM_GROUP)
int gradm_group_id()
{
    static int gid= -1;
    if (gid == -1) gid= groupid(GRADM_GROUP);
    return gid;
}
#endif
