/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Unix Passwd File Identification Database Interface
 *
 * This module supports using the unix password file as your identification
 * database.  It's always your authentication database too.
 */

#include "common.h"
#include "authident.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "set_uid.h"
#include "str.h"
#include "lock.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_ident_module= "ident_passwd";

#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *login)
{
    struct passwd *pw;
    pw= getpwnam(login);
    /* Clip off office/phone number */
    if (pw != NULL) *firstin(pw->pw_gecos,",")= '\0';
    return pw;
}
#endif


#ifdef HAVEIDENT_GETPWUID
struct passwd *ident_getpwuid(uid_t uid)
{
    struct passwd *pw;
    pw= getpwuid(uid);
    if (pw != NULL)
    {
	/* Clip off office/phone number */
	*firstin(pw->pw_gecos,",")= '\0';
	/* Set one character passwords to zero characters */
	if (pw->pw_passwd[1] == '\0') pw->pw_passwd[0]= '\0';
    }
    return pw;
}
#endif


#ifdef HAVEIDENT_WALKPW
struct passwd *ident_walkpw(int flag)
{
    struct passwd *pw;

    /* If flag is 0, goto begining of file
    if (!flag) setpwent();

    /* Get next */
    pw= getpwent();
    /* Clip off office/phone number */
    if (pw != NULL) *firstin(pw->pw_gecos,",")= '\0';
    return pw;
}
#endif
