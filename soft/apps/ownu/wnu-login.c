#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <unistd.h>
#include "nu.h"
#include "wnu.h"

/* VERSION HISTORY:
 * 04/30/96 - last verson before I started collecting version history. [jdw]
 * 06/24/97 - Added version history. [jdw]
 *            Teach it to compile (if not necessarily work) on Linux. [jdw]
 */

#ifndef ETCPASSWD
#ifdef BSDI
char ETCPASSWD[] = _PATH_MASTERPASSWD;
#else
char ETCPASSWD[] = "/etc/passwd";
#endif
#endif


int good_login;
char cpt_pass[30];


/* CHECK_PASSWD - Sanity checking for passwords.  This will be called twice
 * once for each of the two copies of the password submitted.  Passwords must:
 *   - match each other
 *   - be between 4 and 40 characters long
 *   - be different from the login name
 *   - not be "password"
 *
 * Additionally, some more rigorous tests may be done when we get to the
 * final_login_check() routine.  We will call Marcus's obvious() routine
 * there to check that it isn't a dictionary word or part of the user's 
 * full name.  Older versions of Marcus's code don't handle this.
 */

char *check_passwd(char *val)
{
int i,len;

    if (userpass != NULL)
    {
	if (strcmp(val,userpass))
	    err(USER,"The two copies of your password are not identical.\n");
	return;
    }

    if ((len= strlen(val)) == 0)
    {
	err(USER,"You did not fill in the <STRONG>Password</STRONG> field.\n");
	return;
    }
    if (len < 4)
    {
	err(USER,"The password you selected is too short.\n");
	return;
    }
    else if (len > 40)
	err(HOSTILE|USER,"The password you selected is too long.\n");

    if (loginid != NULL && (!strcmp(val,loginid) || !strcmp(val,"password")))
	err(USER,"The password you selected is too obvious.\n");

    for (i= 0; i < len; i++)
	if (!isascii(val[i]) || !isprint(val[i]) ||
	    val[i] == '\n' || val[i] == '\r')
	    err(USER,"Only standard ASCII characters are allowed "
		     "in passwords\n");
#ifdef MKP2
    strcpy(clear_pass, val);
#else
    strcpy(cpt_pass, mkpasswd(val));
#endif
    return val;
}


char *pwskip2(p, c)
char *p;
char c;
{
	while (*p && *p != c)
		++p;
	if (*p) *p++ = 0;
	return p;
}
#define pwskip(p) pwskip2(p,':')


/* GPWENT - Read an entry from the passwd file, or some other file in the
 * same format.
 *
 * This originally read a version 7 format passwd file.  Note that in BSDI
 * unix the file /etc/passwd is in this format (but includes no passwords),
 * but the real passwd file, /etc/master.passwd, has some extra fields. If
 * the flag "newformat" is 1, this will expect the new fields, otherwise
 * it just defaults them.  We use this instead of getpwent() because we need
 * to read files other than /etc/master.passwd to support verification.
 */

struct passwd * gpwent(FILE *fp, int newformat)
{
	static struct passwd pwbuf;
	static char pwline[MAXSTRINGSIZE];
	register char *p;
	if (!(p = fgets(pwline, MAXSTRINGSIZE, fp)))
		return 0;
	pwbuf.pw_name = p;	p = pwskip(p);
	pwbuf.pw_passwd = p;	p = pwskip(p);
	pwbuf.pw_uid = atoi(p);	p = pwskip(p);
	pwbuf.pw_gid = atoi(p);	p = pwskip(p);

	/* pwbuf.pw_quota = 0; system III replaces this
	with an age field, see passwd(5), getpwent(3)... */

#ifdef BSDI
	if (newformat)
	{
		pwbuf.pw_class = p;		p = pwskip(p);
		pwbuf.pw_change = atoi(p);	p = pwskip(p);
		pwbuf.pw_expire = atoi(p);	p = pwskip(p);
	}
	else
	{
		pwbuf.pw_class = "";
		pwbuf.pw_change = 0;
		pwbuf.pw_expire = 0;
	}
#endif
#ifdef sunos4	/* probably lots of unixes don't have pw_comment */
	pwbuf.pw_comment = "";
#endif
	pwbuf.pw_gecos = p;	p = pwskip(p);
	pwbuf.pw_dir = p;	p = pwskip(p);
	pwbuf.pw_shell = p;	p = pwskip(p);
	pwskip2(p, '\n');
	return &pwbuf;
}


char *rfullname= NULL;	/* We don't use these.  They are just for au() */
char *rloginid= NULL;

/* CHECK_RESERVED - Check if the login is in the list of reserved logins.
 * If it is, then the userpass must match the passwd.  Returns a 1 iff the
 * name exists & the user didn't supply the right password.  This is based
 * on Marcus's is_reserved(), but Marcus asks the user for the old password
 * instead of requiring that the new password match the old one.  Marcus's
 * way is much better, but it would be a pain in the ass to do over the web.
 *
 * BSDI note:  the reserved file should be in Version 7 format, but it should
 * give passwords for the logins.  Strange huh?
 */

char *check_reserved(char *lid)
{
FILE *fp;
struct passwd *pwd;
int s;
char *clear;

    /* Scan reserved file */

    if ((fp= fopen(rpwd, "r")) == NULL)
	err(SYSTEM,"Cannot open the reserved login file (%S).\n");

    while ((pwd= gpwent(fp,0)) != NULL)
	if (!strcmp(pwd->pw_name, lid))
	    break;

    fclose(fp);

    /* If we found that login name, check userpass against it */

    if (pwd)
    {
	if (strcmp(crypt(userpass, pwd->pw_passwd), pwd->pw_passwd))
	    err(USER,"The <STRONG>Login ID</STRONG> <KBD>%S</KBD> has been "
		"reserved for %S,\nbut the <STRONG>Password</STRONG> that "
		"you selected is not the correct one.\nEither give the "
		"correct password, or choose another login ID.\n",
		loginid, pwd->pw_gecos);
    }
    return lid;
}


/* CHECK_LOGIN - sanity checking for login ids.  They should be:
 *   - 1 to 8 characters long
 *   - first character a lower-case letter
 *   - other characters lower-case letters or digits
 *   - different from the user's passwd
 *   - not "root"
 *   - not a mail alias
 *   - if it is in the list of reserved logins, the userpass matchs
 * People who give the login name "root" or put a ":" in their logins are
 * considered hostile (which basically just means we log the attempt).
 */

char *check_login(char *val)
{
char *p;
int len,f;

    if (isascii(val[0]) && isdigit(val[0]))
	err((val[0]==':')?HOSTILE|USER:USER,
	    "The first letter of your <STRONG>Login ID</STRONG> "
	    "(<KBD>%#S</KBD>) must be a lower-case letter.\n",val);

    for (p= val, f=1; *p != '\0'; p++)
    {
	if (f && (!isascii(*p) || (!isdigit(*p) && !islower(*p))))
	{
	    err((*p == ':')?HOSTILE|USER:USER,
		"Your <STRONG>Login ID</STRONG> (<KBD>%#S</KBD>) "
		"contains the character '<KBD>%#C</KBD>'.\n"
		"Only digits and lower-case letters are allowed.\n",val,*p);
	    f= 0;
	}
    }

    /* Check login length */
    len= p - val;
    if (len == 0)
	err(USER,"You did not fill in the <STRONG>Login ID</STRONG> field.\n");
    else if (len > 8)
	err(USER,"The <STRONG>Login ID</STRONG> you gave (<KBD>%.20S</KBD>) "
		"contains more than 8 characters.\n",val);
    
    /* If it isn't valid, don't even bother checking if it is in use */
    good_login= !reject;
    if (reject) return val;

    if (userpass != NULL && strcmp(val,userpass))
	err(USER,"The password you selected is too obvious.\n");

    /* Not that we're really worried, but... */
    if (!strcmp(val,"root"))
    {
	err(USER|HOSTILE, "The <STRONG>Login ID</STRONG> "
	    "<KBD>root</KBD> is not for the use of small children.\n");
	good_login= 0;
	return val;
    }
    
    /* Check for aliases */
    if (!isalias(val))
    {
	err(USER,"The <STRONG>Login ID</STRONG> <KBD>%S</KBD> is in use "
	    "as a mail alias.  Try something else.\n",val);
	good_login= 0;
	return val;
    }

    if (*rpwd)
	check_reserved(val);

    /* We check if the login name is in use later, when we have a lock on */
    return val;
}


/* FINAL_LOGIN_CHECK - The last few login sanity checks need to be done under
 * a file lock.  If all seems well, this file lock is left on when this
 * routine is done, presuming that we are about to run adduser().  This is
 * adapted from Marcus's lid_free().
 */

void final_login_check()
{
int lockset;
char *by_whom;
int obv;

    /* If we already know the login is bad, never mind checking it again */
    if (!good_login)
	return;

    /* Lock password file unless we already know we aren't accepting this
     * application (so we won't be doing adduser()) or we are are TESTING
     * as non-root */
#ifdef TESTING
    if (getuid() == 0)
#endif
	if (lockset= !reject)
	    set_lock();

    /* Check against both /etc/passwd and the file of unverified users */
    good_login= check_pwd(passwd,&by_whom);
    if (good_login && strcmp(passwd, ETCPASSWD))
	good_login= check_pwd(ETCPASSWD,&by_whom);
    if (!good_login)
	err(USER,"The <STRONG>Login ID</STRONG> <KBD>%S</KBD> is already "
	    "in use by %#S.\n",loginid,by_whom);

    /* Check if the directory for the login is available */
    if (good_login)
    {
#ifdef GREX
	setuserdir();
#endif
	if (checkdirs)
	    good_login= check_usr();
	else
	    good_login= quick_check_dir();

	if (!good_login)
	    err(USER,"The directory for the <STRONG>Login ID</STRONG> "
		"<KBD>%S</KBD> is already in use.\nTry another ID.\n",loginid);
    }

    /* Release the lock if we have one and we rejected this login */
    if (lockset && !good_login)
	un_lock();
}


/* CHECK_USR - routine check to see if:
 *   (a) the proposed directory name already exists
 *   (b) there is any directory already owned by the proposed uid number
 * If (a) occurs, the login name is rejected.  If (b) occurs, the uid is
 * incremented.	 Part (b) is rather slow so you should avoid using this.
 * This is adapted from Marcus's check_usr() routine.
 */

int check_usr()
{
char nbuf[80];

struct dirent *dp;
#define dir	(*dp)
DIR *fp;
struct stat stbuf;

    if ((fp= opendir(usr)) == NULL)
	err(SYSTEM,"Cannot open %s to check directories.\n",usr);

    for (;;)
    {
	dp = readdir(fp);
	if (!dp) break;
	if (dp->d_ino == 0)	/* fix for jcb's bug */
	    continue;
	sprintf (nbuf, "%s/%s", usr, dp->d_name);
	if (stat(nbuf, &stbuf) == 0)
	    notthisone(stbuf.st_uid, stbuf.st_gid);
	if (strcmp(dp->d_name, loginid) == 0)
	{
	    closedir(fp);
	    return 0;
	}
    }
    closedir(fp);
    return 1;
}


/* QUICK_CHECK_DIR - This is a faster alternative to check_usr() used when
 * the checkdir flag is not set.  It only checks that the directory doesn't
 * exist.
 */

int quick_check_dir()
{
char nbuf[80];

    sprintf(nbuf,"%s/%s",usr,loginid);
    if(!access(nbuf,0))
    {
	err(USER,"The directory for the <STRONG>Login ID</STRONG> "
	    "<KBD>%S</KBD> is already in use.\nTry another ID.\n",loginid);
	return 0;
    }
    return 1;
}


/* CHECK_PWD - Scan through the passwd file, to make sure that
 *  (a) the login name is not already taken.
 *  (b) the uid number is not in use.
 * Note that the passwd file may not really be /etc/passwd file.
 *
 * BSDI note:  we check against /etc/master.passwd, which is the real passwd
 * file.  This is because we want to use this same routine to check against
 * the file of unverified users, and we want to be able to append the
 * unverified users easily to the real passwd file with vipw.  And that means
 * it has to be in the new format.
 */

int check_pwd(char *passfile, char **by_whom)
{
register FILE *fp;
struct passwd *pwd;
struct stat stbuf;
int quick_scan;

    /* Do a quick check that the account name is free.  This can be trusted
     * to work only on the actual passwd file.
     */

    if (quick_scan= (reject || !strcmp(passfile,ETCPASSWD)))
    {

	if ((pwd= getpwnam(loginid)) != NULL)
	{
	    *by_whom= pwd->pw_gecos;
	    return 0;
	}
    }

    /* Scan the uid numbers, and, for non-passwd files, check that login is
     * not in use while we're at it.
     */

    if (!reject)
    {
	if ((fp= fopen(passfile, "r")) == NULL)
	    return 1;
	fstat(fileno(fp), &stbuf);
	while ((pwd= gpwent(fp,1)) != NULL)
	{
	    notthisone(pwd->pw_uid, pwd->pw_gid);
	    if (!quick_scan && strcmp(pwd->pw_name, loginid) == 0)
	    {
		fclose(fp);
		*by_whom= pwd->pw_gecos;
		return 0;
	    }
	}
	fclose(fp);
    }
    return 1;
}
