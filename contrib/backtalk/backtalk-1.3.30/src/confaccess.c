/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "sysdict.h"

#include "confaccess.h"
#include "ps_config.h"
#include "ps_conflist.h"
#include "str.h"


/* CONF_ACCESS -- Check if we have read access to the named conference.
 * Return:
 *   -1 = have access and am fairwitness or admin.
 *    0	= have access
 *    1 = bad conference (not found, malformatted).
 *    2 = permission denied.
 */

int conf_access(char *conf)
{
    int mode, mayread;
    int have_acl= 0;
    char *dir;
    char fw[BFSZ];
    extern char curr_conf[];

    if (conf == NULL || conf[0] == '\0')
	return 1;

    if (bdict(VAR_AMADM))
	return -1;

    /* If this matches the currently active conference, return MAYREAD */
    if (!strcasecmp(conf,curr_conf))
	return bdict(VAR_MAYREAD) ? 0 : 2;

    /* Find path to conf config file */
    if ((dir= cfpath(conf)) == NULL)
	return 1;

    /* Get participation file name, fairwitness, mode and title */
    readconfig(dir, NULL, fw, &mode, NULL);

    /* Fairwitnesses always have access */
    if (inlist(sdict(VAR_ID),fw," ,"))
	return -1;

#ifndef YAPP_COMPAT
    if (mode < 0)	/* check ACL file only if mode was 'acl' */
#endif
	have_acl= read_acl(dir, &mayread, NULL, NULL);

    /* ACL file says we have read access */
    if (have_acl && mayread)
	return 0;

    /* Mode is 'acl' but there is no acl file */
    if (mode < 0)
	return 1;

    /* Can aways read fishbowls */
    if ((mode&0x10) != 0)
	return 0;

    mode&= 0x0F;

    /* If conference has password, and it hasn't been given */
    if ((mode == 5 || mode == 6) && !cpass_ok(sdict(VAR_CPASS)))
	return 2;

    /* If conf has ulist/glist, check if we are in them */
    if ((mode == 4 || mode == 6) &&
	    !in_ulist(sdict(VAR_ID),"ulist") &&
	    !in_glist(sdict(VAR_ID),idict(VAR_GID),"glist"))
	return 2;

    /* Otherwise - OK*/
    return 0;
}
