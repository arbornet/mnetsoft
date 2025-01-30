/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#if !NOPWEDIT

#include <ctype.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "adm.h"
#include "adm_check.h"
#include "set_uid.h"

#if defined(RESERVE_UNIX_LOGINS)
#include <pwd.h>
#endif

#ifdef AUTH_DBM
#include "hashfile.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifndef LINKCONF
#define LINKCONF "linkconf"
#endif


/* LOGIN_BAD - If the given login id is legal return zero.  If it is bad,
 * store an error message in "msg" and return 1.  This does not check if
 * the account already exists.
 */

int login_bad(char *lid, char *msg)
{
    char *c;
#ifdef RESERVE_UNIX_LOGINS
    struct passwd pw;
    uid_t ruid;
#endif

    if (lid == NULL || *lid == '\0')
    {
	strcpy(msg,"No login name given.\n");
	return 1;
    }

#ifdef MIN_LOGIN_LEN
    if (strlen(lid) < MIN_LOGIN_LEN)
    {
	sprintf(msg,"Login name must be at least %d characters long.\n",
	    MIN_LOGIN_LEN);
	return 1;
    }
#endif
    
    if (strlen(lid) > MAX_LOGIN_LEN)
    {
	sprintf(msg,"Login name exceeds %d characters.\n",MAX_LOGIN_LEN);
	return 1;
    }

    for (c= lid; *c != '\0'; c++)
#ifdef MIXED_CASE_LOGINS
	if (!isascii(*c) || !(isalpha(*c) || (c != lid && isdigit(*c))))
	{
	    strcpy(msg,"Logins may contain only letters and digits, "
		"and begin with a letter.\n");
	    return 1;
	}
#else
	if (!isascii(*c) || !(islower(*c) || (c != lid && isdigit(*c))))
	{
	    strcpy(msg,"Logins may contain only lower-case letters and digits, "
		"and begin with a letter.\n");
	    return 1;
	}
#endif /* !MIXED_CASE_LOGINS */

#ifdef RESERVE_UNIX_LOGINS
    setpwent();
    pw= getpwnam(lid);
    endpwent();
    ruid= getuid();
    if (!(pw == NULL || (pw->uid == ruid && ruid != SERVER_UID)))
    {
	sprintf(msg,"Login name %s is reserved for unix user %s.%s\n"
	    lid,
	    pw->pw_gecos,
	    ruid != SERVER_UID ? "" :
		"If that's you, login into your account and run the '"
		LINKCONF"' command.\n");
	return 1;
    }
#endif
    return 0;
}

#endif  /* !NOPWEDIT */
