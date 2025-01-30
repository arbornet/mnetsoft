/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* GENERIC INTERFACE TO AUTH AND IDENT MODULES */

#include "common.h"
#include "set_uid.h"

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <errno.h>
extern char *crypt ();

#include "udb.h"
#include "lock.h"
#include "nextuid.h"
#include "authident.h"
#if defined(DBM_UID_FILE) || defined(UID_MAP_FILE)
#include "hashfile.h"
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef HAVEIDENT_WALK

#define any_walk(f) ident_walk(f)
#define HAVEANY_WALK
#ifdef HAVEIDENT_SEEK
#define any_seek(l) ident_seek(l)
#define HAVEANY_SEEK
#endif

#else
#ifdef HAVEAUTH_WALK

#define any_walk(f) auth_walk(f)
#define HAVEANY_WALK
#ifdef HAVEAUTH_SEEK
#define any_seek(l) auth_seek(l)
#define HAVEANY_SEEK
#endif

#endif
#endif

void killdir(char *dir);

/* Cache for the last user ident looked up */
struct passwd last_pw;
int have_last_pw= FALSE;


/* FLUSHUSERCACHE - If we do something like change our full name, we need to
 * be able to forget the previously loaded user info and reload it.  This
 * does that.
 */
 
 void flushusercache()
{
    if (!have_last_pw) return;
    if (last_pw.pw_name != NULL) free(last_pw.pw_name);
    if (last_pw.pw_passwd != NULL) free(last_pw.pw_passwd);
    if (last_pw.pw_gecos != NULL) free(last_pw.pw_gecos);
    if (last_pw.pw_dir != NULL) free(last_pw.pw_dir);
    have_last_pw= FALSE;
}


/* SETUSERCACHE - Put a user into the last user cache.
 */
 
void setusercache(struct passwd *pw)
{
    flushusercache();
    last_pw.pw_name= strdup(pw->pw_name);
    last_pw.pw_passwd= (pw->pw_passwd != NULL) ? strdup(pw->pw_passwd) : NULL;
    last_pw.pw_uid= pw->pw_uid;
    last_pw.pw_gid= pw->pw_gid;
    last_pw.pw_gecos= (pw->pw_gecos != NULL) ? strdup(pw->pw_gecos) : NULL;
    last_pw.pw_dir= (pw->pw_dir != NULL) ? strdup(pw->pw_dir) : NULL;
    have_last_pw= TRUE;
}


/* HOMEDIRPATH - Given a login name, generate the name of an appropriate
 * home directory.  
 */

#ifdef MIXED_CASE_LOGINS
#define lc(x) (isupper(x) ? tolower(x) : (x))
#else
#define lc(x) (x)
#endif

char *homedirpath(char *name)
{
    static char dir[BFSZ];

#if USER_DIR_LEVEL == 0
    sprintf(dir, USER_DIR"%s", name);
#elif USER_DIR_LEVEL == 1
    sprintf(dir, USER_DIR"%c/%s", lc(name[0]), name);
#else
    sprintf(dir, USER_DIR"%c/%c/%s",
        lc(name[0]), name[1] == '\0' ? '_' : lc(name[1]), name);
#endif
    return dir;
}


/* GETDBNAM - Find passwd structure for the named user.  The returned password
 * structure will have the pw_name, pw_uid, pw_gid, pw_gecos (full name) and
 * pw_dir fields set.  pw_passwd and pw_shell may or may not have something
 * useful in them.
 */

struct passwd *getdbnam(char *name)
{
    struct passwd *pw;

    /* Look first in the cache */
    if (have_last_pw && !strcmp(name, last_pw.pw_name))
    	return &last_pw;

    pw= ident_getpwnam(name);

    if (pw == NULL) return NULL;

    /* Fixups */
    if (pw->pw_name == NULL) pw->pw_name= name;
    if (pw->pw_passwd == NULL) pw->pw_passwd= "";
    if (pw->pw_dir == NULL) pw->pw_dir= homedirpath(name);

    setusercache(pw);
    return pw;
}


#if defined(ID_GETUID) && !defined(UID_MAP_FILE)

/* GETDBUID - Find passwd structure for user with the given uid number.  The
 * returned password structure will have the pw_name, pw_uid, pw_gid, pw_gecos
 * (full name) and pw_dir fields set.  pw_passwd and pw_shell may or may not
 * have something useful in them.
 */

struct passwd *getdbuid(uid_t uid)
{
    struct passwd *pw;

    /* Look first in the cache */
    if (have_last_pw && last_pw.pw_uid == uid)
    	return &last_pw;

    pw= ident_getpwuid(uid);

    if (pw == NULL) return NULL;

    /* Fixups */
    if (pw->pw_passwd == NULL) pw->pw_passwd= "";
    if (pw->pw_dir == NULL) pw->pw_dir= homedirpath(pw->pw_name);

    setusercache(pw);
    return pw;
}
#endif


/* WALKDB - Find the passwd structure for the "first" (if flag-=0) or "next"
 * (if flag!=0) user in the database.  The returned password structure will
 * have the pw_name, pw_uid, pw_gid, pw_gecos (full name) and pw_dir fields
 * set.  pw_passwd and pw_shell may or may not have something useful in them.
 * Returns NULL if no users are left.
 */

struct passwd *walkdb(int flag)
{
    struct passwd *pw;

#ifdef HAVEIDENT_WALKPW
    pw= ident_walkpw(flag);

    if (pw == NULL) return NULL;

    /* Fixups */
    if (pw->pw_passwd == NULL) pw->pw_passwd= "";
    if (pw->pw_dir == NULL) pw->pw_dir= homedirpath(pw->pw_name);

    setusercache(pw);
    return pw;
#else
#ifdef HAVEANY_WALK
    char *name;

    /* This loop only iterates if there is a dud user in the passwd file */
    while ((name= any_walk(flag)) != NULL)
    {
	pw= getdbnam(name);
	if (pw != NULL) return pw;
    }
    return NULL;
#else
#error Neither ident_walkpw, ident_walk, nor auth_walk are defined
#endif
#endif
}


/* GETHOMEDIR - Find a home directory given a login name.  May or may not be
 * faster than calling getdbnam.  Returned value is in static memory on
 * success, NULL otherwise.
 */

char *gethomedir(char *name)
{
#ifdef IDENT_STOREDIR
    struct passwd *pw;

    if ((pw= getdbnam(name)) == NULL)
	return NULL;
    else
	return pw->pw_dir;
#else
    return homedirpath(name);
#endif
}


/* WALKUSR - Return the login name of the "first" (if flag==0) or "next"
 * (if flag==1) user in the database.  Returned name is in static storage.
 */

char *walkusr(int flag)
{
#ifdef HAVEANY_WALK
    return any_walk(flag);
#else
    if (walkdb(flag) == NULL) return NULL;
    return last_pw.pw_name;
#endif
}


/* SEEKUSR - Arrange things so that future calls to nextusr(1) will return
 * the user after the given user in the database.
 */

void seekusr(char *name)
{
#ifdef HAVEANY_SEEK
    any_seek(name);
#else
    char *u;

    for (u= firstusr(); u != NULL && strcmp(u,name); u= nextusr())
	;
#endif
}


/* GETPASSWORD - Get the encrypted password for a user.  Returns NULL on
 * error, or an empty string if we can't access passwords.  If noshare is
 * true and the authentication and identity databases are shared, then
 * we always return "".
 */

char *getpassword(char *login, int noshare)
{
#ifdef SHARE_AUTH_IDENT
#ifndef HAVEAUTH_GETPASS
    struct passwd *pw;
#endif
    if (noshare) return "";

    /* If it is in cache, use that */
    if (have_last_pw && !strcmp(login, last_pw.pw_name))
	return last_pw.pw_passwd;
#endif

    /* If we have a getpass routine, use that */
#ifdef HAVEAUTH_GETPASS
    return auth_getpass(login);
#else
#ifdef SHARE_AUTH_IDENT
    /* If it's in the a shared auth/ident database, get it from that */
    if ((pw= getdbnam(login)) != NULL)
    {
	static char pass[40];
	strcpy(pass,pw->pw_passwd);
	return pass;
    }
#endif
    return "";
#endif
}


#ifdef ID_SESSION
/* CHECKPASSWORD - Check if an plain text password is correct for a user.
 */

int checkpassword(char *login, char *passwd)
{
#ifdef HAVEAUTH_CHECKPASS
    return auth_checkpass(login,passwd);
#else
    char *encpass;

    if ((encpass= getpassword(login,FALSE)) != NULL)
	return !strcmp(encpass,crypt(passwd,encpass));
    else
	return 0;
#endif
}
#endif


/* CHANGE_FNAME - Change the user's full name to the given value.  The full
 * set of old ident information must be given in pw.  This is a no-op if we
 * can't do it.
 */

void change_fname(struct passwd *pw, char *newfname)
{
#ifdef HAVEIDENT_NEWFNAME
    ident_newfname(pw,newfname);
#endif
}

/* CHANGE_EMAIL - Change the user's email address if it is stored and editable.
 * set of old ident information must be given in pw.  This is a no-op if we
 * can't do it (which is usually).
 */

void change_email(struct passwd *pw, char *newemail)
{
#ifdef HAVEIDENT_NEWEMAIL
    ident_newemail(pw,newemail);
#endif
}


/* CHANGE_GID - Change the user's primary group id to the given value.  The
 * full set of old ident information must be given in pw.  This is a no-op if
 * we can't do it.
 */

void change_gid(struct passwd *pw, gid_t gid)
{
#ifdef HAVEIDENT_NEWGID
    ident_newgid(pw,gid);
#endif
}


/* CHANGE_PASSWORD - Change the user's password to the given value.  The
 * full set of old ident information must be given in pw.  This is a no-op if
 * we can't do it.
 */

void change_passwd(struct passwd *pw, char *newpass)
{
#ifdef HAVEAUTH_NEWPASS
    auth_newpass(pw,newpass);
#endif
}


/* MK_DIRECTORY - Create a directory for the user
 */

void mk_directory(char *dir, char *lid)
{
    extern int errno;
    char *p;

    strcpy(dir, USER_DIR);
    p= dir + strlen(USER_DIR);

    set_httpd_if_httpd_home();

#if USER_DIR_LEVEL > 0

    *p++= lid[0];
    *p= 0;
    if (mkdir(dir,0755) && errno != EEXIST)
	die("Could not create directory %s: %s", dir, strerror(errno));
    *p++= '/';
#if USER_DIR_LEVEL > 1

    *p++= lid[1] == '\0' ? '_' : lid[1];
    *p= 0;
    if (mkdir(dir,0755) && errno != EEXIST)
	die("Could not create directory %s: %s", dir, strerror(errno));
    *p++= '/';
#endif
#endif

    strcpy(p,lid);
    if (mkdir(dir,0755))
	die("Could not create directory %s: %s", dir, strerror(errno));

    set_cfadm_if_httpd_home();
}


/* ADDUSER - create an account for a user.  "dir" is an output, the rest are
 * inputs.  Dir should point at BFSZ worth of storage.  Status is the initial
 * status, 0=validated, 1=unvalidated, 2=invalidated (though doing 2 would be
 * wierd).
 */

void adduser(char *lid, gid_t gid, char *pw, char *fn, int status, char *dir,
    char *email)
{
    uid_t uid;

#ifndef HAVEAUTH_ADD
    die("Installed authentication module does not define auth_add()");
#else
#if !defined(SHARE_AUTH_IDENT) && !defined(HAVEIDENT_ADD)
    die("Installed identity module does not define ident_add()");
#else

    /* Get the next available uid number */
    uid= next_uid(1);

    /* Create user's directory and store name in dir */
    mk_directory(dir,lid);

    /* Create the passwd file entries */
#ifdef SHARE_AUTH_IDENT
    auth_add(lid,pw,status,uid,gid,fn,dir);
#else
#ifdef IDENT_STOREEMAIL
    ident_add(lid,uid,gid,fn,dir,email);
#else
    ident_add(lid,uid,gid,fn,dir);
#endif /* !IDENT_STOREEMAIL */
    auth_add(lid,pw,status,0,0,NULL,NULL);
#endif /* !SHARE_AUTH_IDENT */
#endif /* SHARE_AUTH_IDENT || HAVEIDENT_ADD */
#endif /* HAVEAUTH_ADD */
}


/* KILL_USER - This deletes the user from whatever database he is in and
 * removes his home directory and all his files.  Returns nonzero if there
 * was no such user, 0 on success.
 */

int kill_user(struct passwd *pw)
{
#ifndef HAVEAUTH_DEL
    die("Installed authentication module does not define delauth()");
#else
#if !defined(SHARE_AUTH_IDENT) && !defined(HAVEIDENT_DEL)
    die("Installed identity module does not define delident()");
#else
    int rc;

    rc= auth_del(pw->pw_name);
    if (rc) return rc;

#ifndef SHARE_AUTH_IDENT
    ident_del(pw->pw_name);
#endif

#ifdef DBM_UID_FILE
    /* Delete an entry from a by-uid dbm password file */
    deletedbm(DBM_UID_FILE, (char *)&pw->pw_uid, sizeof(pw->pw_uid), 0);
#endif /* DBM_UID_FILE */

#ifdef UID_MAP_FILE
    /* Delete an entry from the mapuid file
     * Note that this file is indexed by the user's realuid, which is
     * different from the one in pw->pw_uid, so we can't use that to
     * find the key to delete.  So we have to do a search through the
     * whole database for one whose content matches our name key.
     */
    deletedbm(UID_MAP_FILE, pw->pw_name, namelen+1, 1);
#endif /* UID_MAP_FILE */

    killdir(pw->pw_dir);

    return rc;
#endif /* SHARE_AUTH_IDENT || HAVEIDENT_DEL */
#endif /* !HAVEAUTH_DEL */
}


/* KILLDIR - Remove a directory and all files below it. */

void killdir(char *dir)
{
    pid_t cpid, wpid;

    if ((cpid= fork()) == 0)
    {
	set_httpd_if_httpd_home();
	execl("/bin/rm","rm","-rf",dir,(char *)NULL);
	exit(1);
    }

    while ((wpid= wait((int *)0)) != cpid && wpid != -1)
	;
}


/* helper routine for mkpasswd() - convert number from 0 to 61 to character
 * in 0-9, A-Z or a-z */

int fc(int c)
{
    c += '0' + 1;
    if (c > '9') c += 7;
    if (c > 'Z') c += 6;
    return c;
}


/* MKPASSWD - encrypt a password using reasonably random salt.
 */

char *mkpasswd(char *clear, int status)
{
    unsigned long int salt;
    static char bf[33], *enc;

    /* Get a reasonable random seed */
#ifdef HAVE_CLOCK_GETTIME
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    salt= tv.tv_sec + tv.tv_nsec;
#else
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;
    gettimeofday(&tv,NULL);
    salt= tv.tv_sec + tv.tv_usec;
#else
    salt= time(NULL);
#endif
#endif
    salt += getpid();

    /* Use a random number generator to smear the randomness around so it
     * spreads out of the low bits, and generate two salt characters */
#ifdef HAVE_RANDOM
    srandom(salt); salt= random();
    bf[0]= fc(salt % 62);
    bf[1]= fc((salt>>6) % 62);
#else
    srand(salt);
    bf[0]= (int) (62.0*rand()/(RAND_MAX+1.0));
    bf[1]= (int) (62.0*rand()/(RAND_MAX+1.0));
#endif

    /* Encrypt the password */
    enc= crypt(clear, bf);
    if (status == 0) return enc;
    bf[0]= (status == 1) ? '?' : '*';
    strcpy(bf+1,enc);
    return bf;
}


/* SAVE_EMAIL - Returns true if we are saving email addresses.  Higher level
 * code doesn't *really* need to know this, but we can save a lot of
 * computation if it does.
 */

int save_email()
{
#ifdef IDENT_STOREEMAIL
    return 1;
#else
    return 0;
#endif
}


#ifdef NEED_UPDATE_PW
/* A couple modules need this.  Putting it here doesn't make a lot of sense,
 * but I haven't figured out a better way to handle it.
 */

/* UPDATE_PW - This updates flat passwd-type files.  It opens the given file,
 * and searches it for a line starting with the given key and a colon.  If
 * replace is not NULL, it replaces that line with the one given, otherwise
 * it deletes that line.  We check for full disks and such problems.  It is
 * the responsibility of the caller to set our uid to something that has write
 * access to the file and the directory containing the file.  The replace line
 * should include a trailing newline.  Returns 1 if there was no such user.
 */

int update_pw(char *file, char *key, char *replace)
{
    char tmp[BFSZ], bf[BFSZ+1];
    FILE *tmpfp, *fp;
    struct stat st;
    int tmpfd, keylen;
    int did_change= 0;
    off_t size;
    int oldumask;

    /* Open original password file */

    if ((fp= fopen(file,"r")) == NULL)
	die ("Cannot open %s",file);
    lock_exclusive(fileno(fp),file);
    fstat(fileno(fp), &st);
    size= st.st_size;

    /* Open temporary file */

    sprintf(tmp,"%s.tmp",file);
    oldumask= umask(0);
    tmpfd= open(tmp, O_WRONLY|O_CREAT|O_EXCL, st.st_mode);
    umask(oldumask);
    if (tmpfd < 0)
    {
	fclose(fp);
	unlock(file);
	if (errno == EEXIST)
	    die("%s already exists -- I won't overwrite it",tmp);
	else
	    die("cannot create %s",tmp);
    }
    tmpfp= fdopen(tmpfd, "w");

    /* Copy the original over into the temporary, looking for update line */

    keylen= strlen(key);
    while (fgets(bf,BFSZ,fp) != NULL)
    {
	if (bf[keylen] == ':' && !strncmp(bf,key,keylen))
	{
	    did_change++;
	    size-= strlen(bf);
	    if (replace != NULL)
	    {
		fputs(replace,tmpfp);
		size+= strlen(replace);
	    }
	}
	else
	    fputs(bf,tmpfp);
    }

    if (!did_change)
    {
	unlink(tmp);
	fclose(fp);
	unlock(file);
	return 1;
    }

    /* Check to make sure that the writes all succeeded */
    fflush(tmpfp);
    fstat(fileno(tmpfp), &st);
    if (st.st_size != size)
    {
	unlink(tmp);
	fclose(fp);
	unlock(file);
	die("Error updating %s.  Disk full?",file);
    }

    /* Replace the old file with the temp file */
    unlink(file);
    if (!link(tmp,file))
	unlink(tmp);

    fclose(fp);
    unlock(file);
    return 0;
}
#endif

#ifdef UID_MAP_FILE

/* LINKUSER - create a link from the given real uid to given conference lid.
 */

void linkuser(uid_t uid, char *lid)
{
    set_cfadm();

    if (storedbm(UID_MAP_FILE,
    		 (char *)&uid, sizeof(uid),
    		 lid, strlen(lid)+1,
    		 1))
	die("unable to open map file "UID_MAP_FILE" (%s)",errdbm());
}

#endif /* UID_MAP_FILE */
