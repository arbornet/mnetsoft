/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "set_uid.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* True if our SAVED UID is 0, false otherwise */
int saved_root= 0;


/* SET_HTTPD - SET_CFADM - set our effective uid to be whatever httpd runs
 * as or whatever the conferences are owned by.  This is needed because when
 * the user database is shared with the password file, it will normally only
 * be readable by the httpd's uid.  Unless we are accessing such a file,
 * we should always be running as the conferencing system.
 *
 * We need a way to toggle our effective uid back and forth between two
 * different uids.  If we have saved-uids, this can be done with seteuid(),
 * but if not, we need to use setreuid() (even though it is depreciated).
 */


#if SERVER_UID != CFADM_UID
#if defined(_POSIX_SAVED_IDS) || defined(SAVED_UID)

/* On systems where we always have saved uids, use seteuid() */

/* Note that if our saved uid is root instead of one of SERVER_UID/CFADM_UID,
 * then we need to become effectively root before we can change our effective
 * uid to something else.  Weird.
 */

void set_httpd()
{
    if (saved_root) seteuid(0);
    seteuid(SERVER_UID);
}

void set_cfadm()
{
    if (saved_root) seteuid(0);
    seteuid(CFADM_UID);
}

#else
#ifdef _POSIX_VERSION

/* On systems where we sometimes have saved uids, check at runtime */

void set_httpd() {
    if (sysconf(_SC_SAVED_IDS))
    {
	if (saved_root) seteuid(0);
	seteuid(SERVER_UID);
    }
    else
    {
	/* Compiler may warn that setreuid() is depreciated.  That's OK.
	 * We probably never call it anyway.
	 */
	setreuid(CFADM_UID, SERVER_UID);
    }
}

void set_cfadm() {
    if (sysconf(_SC_SAVED_IDS))
    {
	if (saved_root) seteuid(0);
	seteuid(CFADM_UID);
    }
    else
    {
	/* Compiler may warn that setreuid() is depreciated.  That's OK.
	 * We probably never call it anyway.
	 */
	setreuid(SERVER_UID, CFADM_UID);
    }
}

#else

/* On systems where we don't have saved uids, use setreuid() */

void set_httpd()
{
    setreuid(CFADM_UID, SERVER_UID);
}

void set_cfadm()
{
    setreuid(SERVER_UID, CFADM_UID);
}

#endif /* _POSIX_VERSION */
#endif /* _POSIX_SAVED_IDS */
#endif /* SERVER_UID != CFADM_UID */


/* SET_ROOT_UID - Nope, this doesn't make you root, but if your REAL UID is
 * root, you should call this once to set things up so that the set_httpd()
 * and set_cfadm() functions will work properly.  It also does a set_httpd().
 */

void set_root_uid()
{
    /* Change SAVED UID and EFFECTIVE UID to root */
    setuid(0);

    /* Set a flag saying that our SAVED UID is root, not one of cfadm/httpd */
    saved_root= 1;

    /* This could do either a setreuid() call or two seteuid() calls:
     *
     * - setreuid(SERVER_UID,CFADM_UID) works because our EFFECTIVE UID is root
     *
     * - seteuid(0);seteuid(SERVER_UID) works because our SAVED UID is root
     *   for the first call, and our EFFECTIVE UID is root for the second.
     */
    set_httpd();
}


/* AS_USER, AS_CFADM - These are used when running in direct_exec mode to
 * toggle between being the user and being cfadm.  None of these functions
 * do anything if direct_exec is not set.
 */

#ifdef ID_GETUID
extern int direct_exec;

#if defined(_POSIX_SAVED_IDS) || defined(SAVED_UID)

void as_user()
{
    if (direct_exec) seteuid(getuid());
}

void as_cfadm()
{
    if (direct_exec) seteuid(CFADM_UID);
}

#else
#ifdef _POSIX_VERSION

/* On systems where we sometimes have saved uids, check at runtime */

void as_user() {
    if (!direct_exec) return;
    if (sysconf(_SC_SAVED_IDS))
	seteuid(getuid());
    else
    {
	/* Compiler may warn that setreuid() is depreciated.  That's OK.
	 * We probably never call it anyway.
	 */
	setreuid(CFADM_UID, getuid());
    }
}

void as_cfadm() {
    if (!direct_exec) return;
    if (sysconf(_SC_SAVED_IDS))
	seteuid(CFADM_UID);
    else
    {
	/* Compiler may warn that setreuid() is depreciated.  That's OK.
	 * We probably never call it anyway.
	 */
	setreuid(geteuid(), CFADM_UID);
    }
}

#else

/* On systems where we don't have saved uids, use setreuid() */

void as_cfadm()
{
    if (direct_exec) setreuid(CFADM_UID, getuid());
}

void set_cfadm()
{
    if (direct_exec) setreuid(geteuid(), CFADM_UID);
}

#endif /* _POSIX_VERSION */
#endif /* _POSIX_SAVED_IDS */

#endif /* ID_GETUID */
