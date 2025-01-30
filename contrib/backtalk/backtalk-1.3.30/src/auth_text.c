/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Flat Textfile Authentication Database Interface
 *
 * This module supports an authentication database formatted as required by
 * Apache mod_auth.  Such files are typically called .htpasswd files.  Each
 * line looks like:
 *
 *     <login>:<passwd>
 *
 * where <login> is a login ID and <passwd> is a crypt() incrypted password.
 *
 * If SHARE_AUTH_IDENT is defined, the same file also contains ident
 * information, and is formatted more like /etc/passwd, with each line
 * containing
 *
 *     <login>:<passwd>:<uid>:<gid>:<fullname>:<homedir>:
 *
 * Here <uid> is the user's uid number, <gid> is his primary group ID,
 * <fullname> is the user's full name, and <homedir> is the full path of the
 * directory where his configuration and participation files are saved.
 */


#include "common.h"
#include "authident.h"

#include <ctype.h>

#include "set_uid.h"
#include "udb.h"
#include "str.h"
#include "lock.h"

#ifdef CFADM_OWNS_AUTH
#define set_httpd_if_httpd_auth()
#define set_cfadm_if_httpd_auth()
#else
#define set_httpd_if_httpd_auth() set_httpd()
#define set_cfadm_if_httpd_auth() set_cfadm()
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_auth_module= "auth_text (file="AUTH_FILE")";

/* AUTH_GETPASS - Get the encrypted password for a user.  Returns NULL if there
 * is no such user.  Works for either standard .htpasswd files, or for more
 * /etc/passwd like files.
 */

#ifdef HAVEAUTH_GETPASS
char *auth_getpass(char *nam)
{
    char bf[BFSZ+1];
    static char pass[BFSZ+1];
    char *a,*b;
    int n;
    FILE *fp;

    if (nam == NULL) return NULL;

    set_httpd_if_httpd_auth();
    fp= fopen(AUTH_FILE,"r");
    set_cfadm_if_httpd_auth();
    if (fp == NULL)
    	die("auth_getpass cannot open file "AUTH_FILE" to read");

    n= strlen(nam);

    while (fgets(bf,BFSZ,fp) != NULL)
    {
	if (bf[n] == ':' && !strncmp(bf,nam,n))
	{
	    a= pass; b= bf+n+1;
	    while (*b != ':' && *b != '\n' && *b != '\0' && a < bf+BFSZ)
		 *(a++)= *(b++);
	    *a= '\0';
	    fclose(fp);
	    return pass;
	}
    }
    fclose(fp);
    return NULL;
}
#endif


/* AUTH_WALK() - Get the first or next name out of the htpasswd file.
 */

#ifdef HAVEAUTH_WALK
FILE *htpfp= NULL;
char *auth_walk(int next)
{
    static char bf[BFSZ+1];

    if (!next)
    {
	/* Open or rewind the file */
	if (htpfp == NULL)
	{
	    set_httpd_if_httpd_auth();
	    htpfp= fopen(AUTH_FILE,"r");
	    set_cfadm_if_httpd_auth();
	    if (htpfp == NULL)
		die("auth_walk cannot open pwfile "AUTH_FILE" to read");
	}
	else
	    rewind(htpfp);
    }

    /* Get next entry */
    if (htpfp == NULL || fgets(bf,BFSZ,htpfp) == NULL)
	return NULL;
    
    *firstin(bf,":\n")= '\0';
    return bf;
}
#endif


/* AUTH_NEWPASS - change the user's password to the given value.  Pass in
 * the encrypted password.
 */

#ifdef HAVEAUTH_NEWPASS
void auth_newpass(struct passwd *pw, char *encpw)
{
    char bf[BFSZ];

#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%s:%d:%d:%s:%s:\n",
	    pw->pw_name, encpw,
	    pw->pw_uid, pw->pw_gid,
	    pw->pw_gecos, pw->pw_dir);
#else
    sprintf(bf,"%s:%s\n",pw->pw_name, encpw);
#endif

    set_httpd_if_httpd_auth();
    update_pw(AUTH_FILE, pw->pw_name, bf);
    set_cfadm_if_httpd_auth();
}
#endif


/* AUTH_ADD - Add an account to the authentication dtabase.
 */

#ifdef HAVEAUTH_ADD
void auth_add(char *lid, char *pw, int status,
    uid_t uid, gid_t gid, char *fn, char *dir)
{
    char bf[BFSZ];
    int fd;
    size_t len;
    char *encpw= mkpasswd(pw,status);

    /* Construct the line */
#ifdef SHARE_AUTH_IDENT
    sprintf(bf,"%s:%s:%d:%d:%s:%s:\n", lid, encpw, uid, gid, fn, dir);
#else
    sprintf(bf,"%s:%s\n", lid, encpw);
#endif
    len= strlen(bf);
    
    /* Open and lock the file */
    set_httpd_if_httpd_auth();
    if ((fd= open(AUTH_FILE, O_WRONLY|O_APPEND)) < 0)
    {
	int uid= getuid();
	set_cfadm_if_httpd_auth();
	die("auth_add cannot open "AUTH_FILE" to write (running as uid %d)",
	    uid);
    }
    lock_exclusive(fd,AUTH_FILE);

    /* Append the line */
    if (write(fd, bf, len) != len)
    {
	set_cfadm_if_httpd_auth();
	die("write to "AUTH_FILE" failed");
    }
    
    /* Close and unlock the file */
    close(fd);
    unlock(AUTH_FILE);
    set_cfadm_if_httpd_auth();
}
#endif


/* AUTH_DEL - Delete the named user */

#ifdef HAVEAUTH_DEL
int auth_del(char *login)
{
    int rc;

    set_httpd_if_httpd_auth();
    rc= update_pw(AUTH_FILE, login, NULL);
    set_cfadm_if_httpd_auth();

    return rc;
}
#endif
