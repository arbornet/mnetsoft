/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
typedef struct dirent direntry;
#else
typedef struct direct direntry;
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#include "dict.h"
#include "partdir.h"
#include "sysdict.h"
#include "file.h"
#include "readfile.h"
#include "udb.h"
#include "str.h"
#include "stack.h"
#include "free.h"
#include "ps_part.h"
#include "set_uid.h"
#include "tag.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char *effective_user= NULL;	/* set by func_selectuser() */


/* FILE HANDLE LIST: Any file that any user is supposed to be able to edit
 * should be in the list below.  This tells where it is, who can edit it
 * and what type of file it is.  Only files on this list can be written,
 * but most any publically readable file can be read.
 */

#define TYP_TEXT   0	/* Regular Text File */
#define TYP_TAG    1	/* Tag File */

#define HD_HOME 0	/* in user's home directory */
#define HD_CFDR 1	/* in user's .cfdir or home directory */
#define HD_PART 2	/* in bbs/part directory */
#define HD_CONF 3	/* in current conference directory */
#define HD_SYST 4	/* in main bbs directory */

#if 0			/* Not using this */
#define HD_GROU 5	/* File lives in main bbs directory with group suffix */
#endif

#define AC_USER 0	/* Only the user can access this file */
#define AC_CONF 1	/* Only the fairwitness can access this file */
#define AC_GADM 2	/* Only group admin and sysadm can access this file */
#define AC_SYST 3	/* Only the sysadm can access this file */

/* These are indexes into the table below.  You must edit them when you edit
 * the table.  This stinks.
 */

#define FH_NONE			-1
#define FH_BACKTALK		 0
#define FH_CFLIST		 1
#define FH_CFONCE		 2
#define FH_CFRC			 3
#define FH_PLAN			 4
#define FH_ACL			 5
#define FH_BBSRC		 6
#define FH_BULL			 7
#define FH_CONFIG		 8
#define FH_CONFLIST		 9
#define FH_CONFMENU		10
#define FH_CONFRC		11
#define FH_DFLT_BACKTALK	12
#define FH_DFLT_CFLIST		13
#define FH_GLIST		14
#define FH_INDEX		15
#define FH_LOGIN		16
#define FH_LOGOUT		17
#define FH_MOTD_HTML		18
#define FH_PUBLICTXT		19
#define FH_SECRET		20
#define FH_SETTINGS		21
#define FH_ULIST		22
#define FH_WELCOME		23

struct fnl {
	char *handle;	/* Name of file */
	int type;	/* Type of file */
	int loc;	/* Location of file */
	int access;	/* Who can write file */
	char *pu_flag;	/* Flag passed to partutil to permit file */
	int dflt;	/* File to read for default value */
	} fhand[]= {

	/* HANDLE          TYPE      LOCATION WRITER   PU_FLAG */
	/* -------------------------------------------------------- */
#ifdef PART_DIR
	{ ".backtalk",	   TYP_TAG,  HD_PART, AC_USER, NULL, FH_DFLT_BACKTALK },
	{ ".cflist",	   TYP_TEXT, HD_PART, AC_USER, NULL, FH_DFLT_CFLIST },
#else
	{ ".backtalk",	   TYP_TAG,  HD_CFDR, AC_USER, "-b", FH_DFLT_BACKTALK },
	{ ".cflist",	   TYP_TEXT, HD_CFDR, AC_USER, "-l", FH_DFLT_CFLIST },
#endif
	{ ".cfonce",	   TYP_TEXT, HD_CFDR, AC_USER, NULL, FH_NONE },
	{ ".cfrc",	   TYP_TEXT, HD_CFDR, AC_USER, NULL, FH_NONE },
	{ ".plan",	   TYP_TEXT, HD_HOME, AC_USER, "-n", FH_NONE },
	{ "acl",	   TYP_TEXT, HD_CONF, AC_SYST, NULL, FH_NONE },
	{ "bbsrc",	   TYP_TEXT, HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "bull",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "config",	   TYP_TEXT, HD_CONF, AC_SYST, NULL, FH_NONE },
	{ "conflist",      TYP_TEXT, HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "confmenu",      TYP_TEXT, HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "confrc",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "dflt.backtalk", TYP_TAG,  HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "dflt.cflist",   TYP_TEXT, HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "glist",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "index",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "login",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "logout",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "motd.html",	   TYP_TEXT, HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "public.txt",	   TYP_TEXT, HD_SYST, AC_SYST, NULL, FH_NONE },
	{ "secret",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "settings",	   TYP_TAG,  HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "ulist",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },
	{ "welcome",	   TYP_TEXT, HD_CONF, AC_CONF, NULL, FH_NONE },

	{ NULL,		   0,	       0,       0,       NULL }};


/* CHECK_MODE - Check if file whose stats are in st is accessible to the
 * give uid and gid.  If uid or gid are -1, that indicates "no user".
 * Mode is some bitwise-or of 4 for read, 2 for write, 1 for execute.
 * We require all the types of access flagged.
 */

int check_mode(struct stat *st, long uid, long gid, int mode)
{
    if (st->st_uid == uid && uid != -1)
	return ((st->st_mode >> 6) & mode) == mode;
    else if (st->st_gid == gid && gid != -1)
	return ((st->st_mode >> 3) & mode) == mode;
    else
	return (st->st_mode & mode) == mode;
}


/* USER_ACCESS - Check if the user has read (mode=4), write (mode=2), or
 * execute (mode=1) access to the named file.  If mode is a bitwise-OR of
 * these, all the flagged access is required.  If dir is true, it must be
 * a directory.  Like access(), this returns zero if he has the required
 * access.  Files that don't exist are not readable or executable, but
 * they may be writable if the parent directory is writable.  If "sys" is
 * true, then we only check if backtalk has the desired access, not if the
 * current user has it.  Return codes are:
 *     0 = we have the required access.
 *     1 = we don't have the required access, but the file exists.
 *     2 = we don't have the required access, and the file does not exist.
 */

#ifdef REAL_UID
#define UA_UID    idict(VAR_UID)
#else
#define UA_UID    -1
#endif
#ifdef REAL_GID
#define UA_GID   idict(VAR_GID)
#else
#define UA_GID   -1
#endif

int user_access(char *fname, int mode, int dir, int sys)
{
    struct stat st;
    char *c;
    int rc;

    if (stat(fname,&st))
    {
	if ((mode != 2 && mode != 6) || dir) return 2;
	/* Writing a file that doesn't exist -- check parent directory */
	if ((c= strrchr(fname,'/')) == NULL)
	    rc= stat(".",&st);
	else
	{
	    *c= '\0';
	    rc= stat(fname,&st);
	    *c= '/';
	}
	if (rc) return 2;
	/* If we are going to create a file, need write/execute access */
	mode= 3;
    }
    else if (mode == 0)
	return 0;

    if (dir && !(st.st_mode & S_IFDIR)) return 1;

    return (((sys || check_mode(&st, UA_UID, UA_GID, mode)) &&
	    check_mode(&st, geteuid(), getegid(), mode)) ? 0 : 1 );
}


/* EFFECTIVE_DIR - Return the home directory of the given user.  If the
 * user is null, then return the currently logged in user.  Often the
 * user is the effective_user.
 */

char *effective_dir(char *user)
{
    if (user == NULL)
	return sdict(VAR_HOMEDIR);
    else
    {
	/* We don't call gethomedir here because we already called getdbname
	 * when we did "selectuser" so this data is in cache and this is
	 * actually faster than gethomedir().
	 */
	struct passwd *pw= getdbnam(user);
	return (pw == NULL) ? NULL : pw->pw_dir;
    }
}


/* HAND_PATH - Write the full pathname of the i-th handle into the buffer
 * pointed to by path.  User is NULL for the current user.  Return true on
 * success.
 */

int hand_path(int i, char *user, char *path)
{
    char *fn= (i == FH_BBSRC || i == FH_CONFRC) ? "rc" : fhand[i].handle;
    char *e;

    switch(fhand[i].loc)
    {
    case HD_CONF:
	sprintf(path,"%s/%s", sdict(VAR_CONFDIR), fn);
	break;

#ifdef HD_GROU
    case HD_GROU:
        e= user_gname();
	if (e == NULL) return 0;
	sprintf(path,"%s/%s.%s", sdict(VAR_BBSDIR), fn, e);
	break;
#endif

    case HD_SYST:
	sprintf(path,"%s/%s", sdict(VAR_BBSDIR), fn);
	break;

    case HD_HOME:
	if ((e= effective_dir(user)) == NULL || e[0] == '\0')
	    return 0;
	sprintf(path,"%s/%s", e, fn);
	break;

    case HD_CFDR:
	if ((e= effective_dir(user)) == NULL || e[0] == '\0')
	    return 0;
	homepartpath(path, e, fn);
	break;

#ifdef PART_DIR
    case HD_PART:
	if ((e= (user ? user : sdict(VAR_ID))) == NULL || e[0] == '\0')
	    return 0;
	bbspartpath(path, e, fn);
	break;
#endif

    default:
	die("Unknown location code %d",fhand[i].loc);
    }
    return 1;
}

#if defined(HTTPD_OWNS_HOMEDIR) && SERVER_UID != CFADM_UID
/* RESTORE_CFADM_STATUS - In rare installations where home directories are
 * owned by httpd, the 'check_handle()' routine will do set_httpd().  This
 * routine should be called after check_handle() to restore things to normal.
 *
 * In less rare instances, when we are running in direct exec mode, the
 * 'check_handle()' routine does as_user() to access files in the user's home
 * directory.  Again, this routine restores things to normal.
 */

static int temp_am_httpd= 0;

void restore_cfadm_status()
{
    if (temp_am_httpd)
    {
	set_cfadm();
	temp_am_httpd= 0;
    }
}
#else
#ifdef ID_GETUID
static int temp_as_user= 0;

void restore_cfadm_status()
{
    if (temp_as_user)
    {
	as_cfadm();
	temp_as_user= 0;
    }
}
#else
#define restore_cfadm_status()
#endif
#endif

/* CHECK_HANDLE - Expand a handle or filename into a regular filename.  If it
 * starts with a '*' it is a handle and is looked up in the table. If it isn't
 * there, or the type doesn't match the expected type, we die with an error
 * message.  If we are reading a text file, and it isn't accessible, it may
 * return the pathname of a readable default file instead.  With tag files,
 * you only get the dflt file if you request it.  Otherwise store the full path
 * name in the given buffer.  If the handle doesn't start with a '*', we just
 * copy it, with no checking, so long as we don't intend to do a write.
 * "Mode" is a boolean-or of:
 *		1 = check execute access
 *		2 = check write access
 *		4 = check read access
 *		8 = don't check file type
 *             16 = force open of dflt file, instead of file itself.
 *	       32 = check if it is a directory
 * It bombs on illegal requests, and returns 0 for success, and 1 for legal
 * requests for files that aren't accessible (eg, don't exist).
 * restore_cfadm_status() should always be called after this.
 */

int check_handle(char *handle, int exptype, int mode, char *path)
{
    static int last= -1;
    int i, rc;
    char *e;

    if (handle == NULL || handle[0] == '*')
    {
	if (mode&32) return 1;	/* No handles are for directories */
	if (handle == NULL)
	{
	    /* Reuse previous handle */
	    if (last == -1) die("no previous check_handle");
	    i= last;
	}
	else
	{
	    /* Search for the handle */
	    for (i= 0; fhand[i].handle != NULL; i++)
		if (!strcmp(handle+1,fhand[i].handle))
		{
		    last= i;
		    goto found;
		}

	    if (mode&8) return 1;
	    die("Illegal file handle: %s",handle);
found:;
	}

	/* Do default file instead */
	if (mode&16)
	{
	    if (fhand[i].dflt == FH_NONE)
		return 1;
	    i= fhand[i].dflt;
	}

	/* Check type correctness */
	if (!(mode&8))
	{
	    if (exptype == TYP_TEXT && fhand[i].type == TYP_TAG)
		die("%s is not a text file",handle);
	    if (exptype == TYP_TAG && fhand[i].type != TYP_TAG)
		die("%s is not a tag file",handle);
	}

	/* Check if we have access */
	switch(fhand[i].access)
	{
	case AC_CONF:
	    if ((mode&2) && !bdict(VAR_AMFW))
	    {
		if (mode&8) return 1;
		die("Only fairwitnesses may write %s",handle);
	    }
	    break;

	case AC_SYST:
	    if ((mode&2) && !bdict(VAR_AMADM))
	    {
		if (mode&8) return 1;
		die("Only conference adminstrator may write %s",handle);
	    }
	    break;

	case AC_USER:
	    break;

	default:
	    die("Unknown access code %d",fhand[i].loc);
	}

	/* Construct full path name */
	if (hand_path(i,effective_user,path))
	{
	    /* If file is in homedir and homedir is owned by httpd, switch
	     * id's to httpd, and leave it that way. */
#if defined(HTTPD_OWNS_HOMEDIR) && SERVER_UID != CFADM_UID
	    if (fhand[i].loc == HD_HOME || fhand[i].loc == HD_CFDR)
	    {
		temp_am_httpd= 1;
		set_httpd();
	    }
#endif

	    /* If file is in homedir and we are running in direct_exec
	     * mode, run as user
	     */

#ifdef ID_GETUID
	    if (direct_exec &&
		    (fhand[i].loc == HD_HOME || fhand[i].loc == HD_CFDR))
	    {
		temp_as_user= 1;
		as_user();
	    }
#endif

	    /* If it exists and we have access, we are done */
	    rc= user_access(path, mode&7, 0, 1);
	    if (!rc) return 0;

	    /* File doesn't exist, or we don't have access */
#ifdef PART_DIR
	    /* If the parent directory of a file in the participation
	     * file directory doesn't exist, create it */
	    if (fhand[i].loc == HD_PART && (mode&2) && errno == ENOENT)
	    {
		mkpartdir(path);
		if (!user_access(path, mode&7, 0, 1)) return 0;
	    }
#else
#ifdef PARTUTIL
	    /* See if partutil can be used to permit this file to us
	     * This is only worth doing if the file exists or we are
	     * planning to write to it.  Rather than read a non-existant
	     * file, we'd prefer to use the default.
	     */
	    if ((rc == 1 || (mode&2)) && fhand[i].access == AC_USER &&
		fhand[i].pu_flag != NULL)
	    {
		partutil(fhand[i].pu_flag,NULL,sdict(VAR_ID));
		if (!user_access(path, mode&7, 0, 1)) return 0;
	    }
#endif /*PARTUTIL*/
#endif /*!PART_DIR*/
	}

	/* See if there is a default file to read instead */
	while (fhand[i].dflt != FH_NONE && exptype == TYP_TEXT && !(mode&2))
	{
	    i= fhand[i].dflt;
	    if (hand_path(i,effective_user,path) &&
	        !user_access(path, mode&7, 0, 1)) return 0;
	}
	return 1;
    }
    else
    {
	/* No default file */
	if (mode&16) return 1;

	/* Normal filenames */
	if (mode&2)
	{
	    if (mode&8) return 1;
	    die("Filenames must be given as handles when writing");
	}
	strcpy(path,handle);

	if (user_access(path, mode&7, mode&32, 0))
	    return 1;
    }
    return 0;
}


/* FUNC_READ:  <filehandle> read <string>
 * Push the entire contents of the named file as a single string onto the
 * stack.  The name can be a normal file path, or a handle starting with
 * a star.  If the file cannot be read, returns a {} string.
 */

void func_read()
{
    char *data, *hand;
    char fname[BFSZ];
    int err;

    err= check_handle(hand= peek_string(), TYP_TEXT, 4, fname);
    free(hand);

    if (err || (data= read_file(fname,FALSE)) == NULL)
	data= strdup("");

    restore_cfadm_status();

    repl_top((void *)data);
}


/* FUNC_WRITE:  <string> <filehandle> write -
 * Overwrite or create the named file with the given string as it's contents.
 * The name must be a handle.
 */

void func_write()
{
    struct stat st;
    char *fhandle=pop_string();
    char fname[BFSZ];
    char *data, *d;
    FILE *fp;
    int err;
    int is_secret;

    err= check_handle(fhandle, TYP_TEXT, 2, fname);
    is_secret= !strcmp(fhandle,"*secret");
    free(fhandle);

    if (err || (fp= fopen(fname,"w")) == NULL || fstat(fileno(fp),&st))
	    die("Cannot open file %s to write",fname);
    if (is_secret) fchmod(fileno(fp),0600);

    restore_cfadm_status();

    data= pop_string();

    for (d= data ; d[0] != '\0'; d++ )
	if (d[0] != '\r' || d[1] != '\n')
	    fputc(*d,fp);
    if (d > data && d[-1] != '\n')
	fputc('\n',fp);

    fclose(fp);
    free(data);
}


/* FUNC_LINE:	<filehandle> <number> line <string>
 * Read the given line number from the given file.  The first line is number
 * zero.  Returns null strings if the file does not exist, is not readable,
 * or is not that long.  May not work right with files with extremely long
 * lines.  The name may be either a regular filename or a handle starting with
 * a star.
 */

void func_line()
{
    int n= pop_integer();
    char *fhandle= peek_string();
    char fname[BFSZ];
    char bf[BFSZ+1];

    if (check_handle(fhandle, TYP_TEXT, 4, fname))
	die("Cannot read %s",fhandle);
    free(fhandle);

    nth_line(fname,n,bf,FALSE);
    restore_cfadm_status();
    push_string(bf,TRUE);
}


/* FUNC_PATHNAME:  <filehandle> <user> pathname <path>
 * Construct the full pathname for a file handle would expand to for a given
 * user.  If user is an empty string, the current user is used.
 */

void func_pathname()
{
    char *user= pop_string();
    char *fhandle= pop_string();
    char bf[BFSZ+1];
    int i;

    if (user[0] == '\0') user= NULL;

    /* Search for the handle */
    for (i= 0; fhand[i].handle != NULL; i++)
	if (!strcmp(fhandle+1,fhand[i].handle))
	    break;
    if (fhand[i].handle == NULL)
	die("Unknown file handle %s",fhandle);

    if (hand_path(i,user,bf))
	push_string(bf,TRUE);
    else
	push_string("",TRUE);
}


/* F_NTH_LINE - this is like nth_line() but works on an already open file */

int f_nth_line(FILE *fp, int n, char *bf, int skip)
{
    int i;

    for (i= 0; i <= n; i++)
    {
	if (fgets(bf,BFSZ,fp) == NULL)
	{
	    bf[0]= '\0';
	    return 0;
	}
	if (skip && (bf[0] == '#' || bf[0] == '\n'))
		i--;
    }
    *firstin(bf,"\n")= '\0';
    bf[BFSZ]= '\0';
    return 0;
}
    

/* NTH_LINE - read the nth line from the named file, returning it in the
 * named size BFSZ+1 buffer.  If skip is true, lines with #'s in front and
 * blank lines are not counted.  First line is number 0.  If the line or the
 * file does not exist, a null string is returned.  Returns 1 if the file
 * does not exist, 0 if it does.  If the line number is too high, a null
 * string is returned.
 */

int nth_line(char *fname, int n, char *bf, int skip)
{
    int i,rc;
    FILE *fp;

    if ((fp= fopen(fname,"r")) == NULL)
    {
	bf[0]= '\0';
	return 1;
    }
    rc= f_nth_line(fp, n, bf, skip);
    
    fclose(fp);
    return rc;
}

/* TAIL - return the last n lines of an open file as a freshly malloced string.
 */

void prebuf(char **ptext, int *ptextlen, char *new, int newlen)
{
    if (*ptext == NULL)
    {
	*ptextlen= newlen+1;
	*ptext= (char *)malloc(*ptextlen);
	memmove(*ptext, new, newlen);
	(*ptext)[newlen]= '\0';
    }
    else
    {
	(*ptextlen)+= newlen;
	*ptext= (char *)realloc(*ptext,*ptextlen);
	memmove(*ptext + newlen, *ptext, *ptextlen - newlen);
	memmove(*ptext, new, newlen);
    }
}

char *tail(int fd, int n)
{
    char buf[BFSZ];
    int i;
    long offset, sz;
    struct stat st;
    char *txt= NULL;
    int txtlen= 0;

    if (fstat(fd, &st) || st.st_size == 0L)
	return NULL;
    sz= (st.st_size < BFSZ) ? st.st_size : BFSZ;
    offset= st.st_size - sz;

    n++;

    while (1)
    {
	lseek(fd, offset, SEEK_SET);
	read(fd, buf, sz);
	if (txt == 0 && buf[sz-1] != '\n') n--;
	for (i= sz - 1; i >= 0; i--)
	    if (buf[i] == '\n' && --n == 0)
	    {
		prebuf(&txt, &txtlen, buf + i + 1, sz - i - 1);
		return txt;
	    }
	prebuf(&txt, &txtlen, buf, sz);
	if (offset == 0L) break;
	sz= (offset < BFSZ) ? offset : BFSZ;
	offset-= sz;
    }
    return txt;
}

/* FUNC_TAIL:  <filehandle> <n> tail <string>
 * Return a string containing the last n lines of a file.
 */

void func_tail()
{
    int n= pop_integer();
    char *hand= pop_string();
    char fname[BFSZ], *t;
    int err, fd;

    err= check_handle(hand, TYP_TEXT, 4, fname);
    free(hand);
    if (err ||
	(fd= open(fname,O_RDONLY)) < 0 ||
	(t= tail(fd,n)) == NULL)
	push_string("",TRUE);
    else
	push_string(t,FALSE);
}

/* NEXTDIR - Return the next filename in curr_direct.  Data is in static
 * storage that will be overwritten by the next call.  It closes curr_direct
 * when you hit the end.
 */

DIR *curr_direct= NULL;

char *nextdir()
{
    direntry *dir;
    static char none[]= "";

    if ((dir= readdir(curr_direct)) == NULL)
    {
	closedir(curr_direct);
	curr_direct= NULL;
    	return none;
    }
    else
    	return dir->d_name;
}


/* FUNC_FIRSTDIR:  <directory> firstdir <filename>
 * Return the first filename in a directory.  If the directory name is an
 * empty string, just close any previously open directory.  Bomb if the
 * directory doesn't exist or isn't readable.  Returns an empty string
 * in the nearly impossible case of the directory being empty.
 */

void func_firstdir()
{
    char *dhandle= pop_string();
    char dname[BFSZ];

    if (curr_direct != NULL)
    {
    	closedir(curr_direct);
	curr_direct= NULL;
    }

    if (*dhandle == '\0')
    	return;

    if (check_handle(dhandle,0,32|8|4|1,dname))
    	die("handle '%s' does not refer to an accessable directory", dhandle);
    free(dhandle);

    if ((curr_direct= opendir(dname)) == NULL)
    	die("cannot open directory %s", dname);

    restore_cfadm_status();

    push_string(nextdir(), TRUE);
}


/* FUNC_NEXTDIR:  - nextdir <filename>
 * Return the next filename the directory selected by the last firstdir call.
 * Return an empty string if we hit the end of the directory.  Bomb if there
 * wasn't a previous call to firstdir, or if a previous call to nextdir
 * already hit the end.
 */

void func_nextdir()
{
    if (curr_direct == NULL)
    	die("no directory scan in progress");
    push_string(nextdir(), TRUE);
}


/* FUNC_DIRECTORY:  <filehandle> directory <bool>
 * Check if a file is a directory with read/execute access.
 */

void func_directory()
{
    char *dhandle= pop_string();
    char dname[BFSZ];

    push_integer(!check_handle(dhandle,0,32|8|4|1,dname));
    restore_cfadm_status();
    free(dhandle);
}


/* FUNC_READABLE:  <filehandle> readable <bool>
 * Check if a file is readable.
 */

void func_readable()
{
    char *fhandle= pop_string();
    char fname[BFSZ];

    push_integer(!check_handle(fhandle,0,8|4,fname));
    restore_cfadm_status();
    free(fhandle);
}


/* FUNC_WRITABLE:  <filehandle> writable <bool>
 * Check if a file is writable.  Only handles are ever writable.
 */

void func_writable()
{
    char *fhandle= pop_string();
    char fname[BFSZ];

    push_integer(!check_handle(fhandle,0,8|2,fname));
    restore_cfadm_status();
    free(fhandle);
}


/* FUNC_EXISTS:  <filehandle> exists <bool>
 * Check if a file exists.
 */

void func_exists()
{
    char *fhandle= pop_string();
    char fname[BFSZ];

    push_integer(!check_handle(fhandle,0,8|0,fname));
    restore_cfadm_status();
    free(fhandle);
}


/* FUNC_FILEDATE: <filehandle> filedate <time>
 * Returns time that the named file was last modified, or 0 if it does not
 * exist.
 */

void func_filedate()
{
    struct stat st;
    char *fhandle= pop_string();
    char fname[BFSZ];

    if (check_handle(fhandle, 0, 8|0, fname) || stat(fname,&st))
	push_time(0);
    else
	push_time(st.st_mtime);
    restore_cfadm_status();
    free(fhandle);
}


#if 0
/* FUNC_CHDIR: <dirname> chdir -
 * Change the current directory to the one named.
 */

void func_chdir()
{
    char *dir= pop_string();

    if (chdir(dir))
	die("could not change directory to %s",dir);
    free(dir);
}
#endif


/* POP_LITARRAY - Get an array of literals off the stack.  If the value on the
 * stack is just a single literal, form it into an array of one literal and
 * return that array.
 *
 * What is this doing in the file.c source module?
 */

void pop_litarray(Token *t)
{
    int i;

    pop_any(t);
    if (type(*t) == TK_UNBOUND_LITERAL || type(*t) == TK_BOUND_LITERAL)
    {
	/* Turn literal into an array of size one containing it */
	Array *a= (Array *)malloc(sizeof(Array));
	a->a= (Token *)malloc(sizeof(Token));
	a->a[0]= *t;
	a->sz= a->lk= 1;
	t->flag= TK_ARRAY;
	t->val= (void *)a;
    }
    else if (type(*t) == TK_ARRAY)
    {
	Token *ta= aval(*t)->a;

	/* Check each element to make sure it is a string */
	for (i= 0; i < aval(*t)->sz; i++)
	    if (type(ta[i]) != TK_UNBOUND_LITERAL &&
	        type(ta[i]) != TK_BOUND_LITERAL)
		die ("expected array containing only literals");
    }
    else
	die("Expected literal or array");
}


/* VARCMP - check if the given pattern matches the given variable name.
 * Any pattern ending with a dot matches all variables that start with the
 * same characters.  Patterns not ending with a dot need to exactly match
 * the variable name.
 *
 * Return values are *not* like strcmp().  We return 1 if they match, zero
 * otherwise.
 */

int varcmp(char *pat, char *var)
{
    char *p;

    for (p= pat; *p == *var; p++, var++)
	if (*p == '\0') return 1;

    return (p != pat && *p == '\0' && p[-1] == '.');
}


/* DO_LOADVAR - given an array and a filename, do the loadvar of those
 * variables from that file.  If varray is NULL, load all variables.
 */

int do_loadvar(Array *varray, char *fname)
{
    char *var, *val;
    Token st;
    rtag *rt;
    int i, rc;

    if ((rt= open_tag(fname,TRUE)) == NULL)
    {
	/* Failure - do nothing */
	return 1;
    }

    while ((rc= get_tag(rt, &var, &val)) > 0)
    {
	/* Scan through array, to see if we should load this variable */
	if (varray != NULL)
	{
	    for (i= 0; i < varray->sz; i++)
	    {
		if (type(varray->a[i]) == TK_UNBOUND_LITERAL)
		{
		    if (varcmp(sval(varray->a[i]), var))
			break;
		}
		else
		{
		    if (varcmp(sysdict[ival(varray->a[i])].key, var))
		    	break;
		}
	    }
	}

	/* Load the variable */
	if (varray == NULL || i < varray->sz)
	{
	    /* Set variable to value from file */
	    if (rc == 1 || rc == 3)
	    {
		/* Save string value */
		st.flag= TK_STRING | TKF_FREE;
		st.val= (void *)strdup(val);
	    }
	    else if (isascii(*val) && isdigit(*val))
	    {
		st.flag= TK_INTEGER;
		st.val= (void *)atol(val);
	    }
	    else
		die("%s tag has bad integer value",var);

	    if (type(varray->a[i]) == TK_UNBOUND_LITERAL)
		store_dict(var,&st,FALSE);
	    else
		set_dict(sysdict+ival(varray->a[i]),&st,0);
	}
    }

    close_tag(rt);

    if (rc < 0)
	die("Syntax error in tagfile");

    return 0;
}


/* FUNC_LOADVAR:  <filehandle> <vars> loadvar <error>
 * Read varible settings for the listed variables from a file.  <vars> can be
 * either a single variable name or an array of variable names.  Pushs a
 * boolean which is true if the file was not readable.
 */

void func_loadvar()
{
    Token vt;
    char fname[BFSZ], *hand;
    int err,rc1= 1, rc2= 1;

    /* Get the variable name array */
    pop_litarray(&vt);

    /* If the array empty, return fast */
    if (aval(vt)->sz == 0)
    {
	free_val(&vt);
	/* Discard handle */
	free(pop_string());
	push_integer(1);
	return;
    }

    /* Do the default version of the file first, if there is one */
    err= check_handle(hand= pop_string(), TYP_TAG, 16|4, fname);
    if (!err) rc1= do_loadvar(aval(vt),fname);

    /* Do the file itself, if it exists */
    if (!check_handle((hand[0]=='*') ? NULL : hand, TYP_TAG, 4, fname))
	rc2= do_loadvar(aval(vt),fname);

    restore_cfadm_status();

    free_val(&vt);
    free(hand);

    push_integer(rc1&rc2);
}


/* WRTTAG - Write the token as a tag line */

void wrttag(FILE *fp, char *name, Token *t)
{
    if (type(*t) == TK_STRING)
	wrtstrtag(fp,name,sval(*t),FALSE);
    else if (type(*t) == TK_INTEGER)
	wrtinttag(fp,name,ival(*t),FALSE);
    else
	die("%s is not a string or integer variable", name);
}


/* DO_SAVEVAR - this does most of the work for func_savevar below.  fname is
 * the filename to save in (all security checks are assumed to have been made
 * already), and vt is a pointer to an array of literal names.
 */

void do_savevar(char *fname, Token *vt)
{
    char *var;
    Token  *val, *a;
    taglist *tagl= NULL;
    int i;

    /* Handy pointer into array */
    a= aval(*vt)->a;

    /* Set up linked list of new values to set with names from array and
     * values from the dictionary */
    for (i= 0; i < aval(*vt)->sz; i++)
    {
	/* Get name and value */
	if (type(a[i]) == TK_UNBOUND_LITERAL)
	{
	    var= sval(a[i]);
	    if ((val= get_dict(var)) == NULL)
		die("variable %s not defined",var);
	}
	else
	{
	    var= sysdict[ival(a[i])].key;
	    val= &(sysdict[ival(a[i])].t);
	}

	/* Add to list */
	if (type(*val) == TK_STRING)
	    add_str_tag(&tagl, var, sval(*val));
	else if (type(*val) == TK_INTEGER)
	    add_int_tag(&tagl, var, ival(*val));
	else
	    die("%s is not a string or integer variable", var);
    }

    /* Update the tag file */
    update_tag(fname, &tagl, bdict(VAR_AMADM));
}


/* FUNC_SAVEVAR:  <filehandle> <vars> savevar -
 * Save the current values of the listed variables into a file.  <vars> can be
 * either a single variable name or an array of variable names.  If the file
 * already contains data, old values for these variables will be replaced,
 * and new ones will be appended.
 */

void func_savevar()
{
    Token vt;
    char fname[BFSZ],*hand;

    /* Get the variable name array */
    pop_litarray(&vt);

    /* If the array empty, return fast */
    if (aval(vt)->sz == 0)
    {
	free_val(&vt);
	return;
    }

    /* Get the file name */
    if (check_handle(hand= pop_string(), TYP_TAG, 2, fname))
	die("Cannot write to %s",hand);
    free(hand);

    do_savevar(fname, &vt);

    restore_cfadm_status();
    free_val(&vt);
}


/* OPEN_SCRIPT - Open a script given the filename of either the compiled or
 * uncompiled script.  In the future, this should check if the script needs
 * recompilation, and do it if necessary.  It returns and file descriptor on
 * success, or -1 on failure.  If bname is non-null, it points to a BFSZ
 * storage area in which to return a copy of the binary file name.
 */

int open_script(char *fname, char *bname)
{
    char *sfile, *bfile, *x;
    int fd;
    int len= strlen(fname);

    if (len < 4 || strcmp(fname+len-3,".bt") != 0)
    {
	/* Name given had no .bt extension */
	sfile= x= (char *)malloc(len+4);
	strcpy(sfile,fname);
	strcpy(sfile+len,".bt");
	bfile= fname;
    }
    else
    {
	/* Name given had a .bt extension */
	bfile= x= (char *)malloc(len-2);
	strncpy(bfile,fname,len-3);
	bfile[len-3]= '\0';
	sfile= fname;
    }

    /* Future checks go here */

    /* Open the file */
    fd= open(bfile,0);

    /* Return a copy of the binary name */
    if (bname != NULL)
    {
	strncpy(bname,bfile,BFSZ-1);
	bname[BFSZ-1]= '\0';
    }

    /* Deallocate whichever name we had in allocated memory */
    free(x);

    return fd;
}


/* OPEN_CFLIST - open the the .cflist file.  If it doesn't exist, get the
 * dflt.cflist file instead.  Returns the open file descriptor if we got it,
 * or NULL if there was no accessible one.
 */

FILE *open_cflist(int mine)
{
    FILE *fp;
    char fname[BFSZ+1];

    if (check_handle("*.cflist",TYP_TEXT,mine ? 4 : 20,fname))
	return NULL;

    fp= fopen(fname,"r");
    restore_cfadm_status();
    return fp;
}
