/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Yapp-Compatible Identification Database Interface
 *
 * This supports an identity database compatible with Yapp installations that
 * do *not* use Unix logins.  Yapp maintains a file in the user's home
 * directory called 'passwd'.  This file contains one line:
 *
 *     <login>:<full name>:<email-address>
 *
 * With this module installed, Backtalk expects all users to have such a file,
 * and uses it as the place to store full names and email addresses.
 *
 * It will also create a shortened version of the standard Backtalk 'id'
 * tag file, containing only the user's uid and gid number.  If a user is
 * encountered who does not have an 'id' file, we create one, assigning
 * the USER group id and a new uid.
 *
 * As in ident_tagfile, the directory name is generated from the login name.
 */


#include "backtalk.h"
#include "authident.h"
#include "nextuid.h"
#include "group.h"
#include "set_uid.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include "set_uid.h"
#include "str.h"
#include "udb.h"
#include "lock.h"
#include "tag.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define YAPP_PASSWD_FILE "passwd"

char *showopt_ident_module= "ident_yapp";

/* SAVE_TAG - Write the given variable/value pair into a tag file, replacing
 * the old value, if there was one, or appending the value.  The value is
 * not changed if it was protected and the user is not an administrator.
 * This always takes five arguments, but the last may be either an integer
 * or a string depending on the type.
 */

#if __STDC__
void save_tag(char *fname, int amadm, char *varname, int type, ...)
#else
void save_tag(fname, amadm, varname, type, va_alist)
    char *fname;
    int amadm;
    char *varname;
    int type;
    va_dcl
#endif
{
    va_list ap;
    taglist *tagl= NULL;

    VA_START(ap, type);

    /* Add our new tag to our tag list */
    if (type == TK_INTEGER)
	add_int_tag(&tagl, varname, va_arg(ap, int));
    else
	add_str_tag(&tagl, varname, va_arg(ap, char *));

    /* Update the tag file with the tag list */
    set_httpd_if_httpd_home();
    update_tag(fname, &tagl, amadm);
    set_cfadm_if_httpd_home();

    va_end(ap);
}

/* YF_CLEAN - replace colons and newlines in a string with spaces.  This is
 * a cleanup done before writing a name to a yapp passwd file.  Probably
 * mostly unnecessary, since the check routines do much the same.
 */

void yf_clean(char *p)
{
    if (p == NULL) return;
    while (*(p= firstin(p,":\n")) != '\0')
	*(p++)= ' ';
}


/* IDENT_GETPWNAM - Get the database entry for the named user.
 */

#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *nam)
{
    static struct passwd pw;
    static char fn[BFSZ];
    static char em[BFSZ];
    char fname[BFSZ+1];
    char bf[BFSZ+1], *dir, *var, *val, *end, *b, *e;
    FILE *fp;
    int ch;

    pw.pw_name= nam;

    /* Construct the directory name */
    if (nam == NULL || (dir= homedirpath(nam)) == NULL)
	return NULL;
    pw.pw_dir= dir;

    /* Open Yapp-ish passwd file */
    sprintf(fname,"%s/"YAPP_PASSWD_FILE,dir);
    set_httpd_if_httpd_home();
    fp= fopen(fname,"r");
    set_cfadm_if_httpd_home();
    if (fp == NULL) return NULL;

    b= fgets(bf,BFSZ,fp);
    fclose(fp);

    /* Various sanity checks */
    if (b == NULL || (e= strchr(b,':')) == NULL) return NULL;
    *e= '\0';
    if (strcmp(b,nam) || (e= strchr(b= e+1,':')) == NULL) return NULL;

    /* Get full name */
    *e= '\0';
    strcpy(fn,b);
    pw.pw_gecos= fn;

    /* Get email address - 
     * stash it in the 'shell' field just to put it somewhere */
    *firstin(b= e+1,"\n\r ");
    if (e-b > BFSZ) b[BFSZ-1]= '\0';
    strcpy(em,b);
    pw.pw_shell= em;

    /* Open tag file */
    sprintf(fname,"%s/"IDENT_FILE,dir);
    set_httpd_if_httpd_home();
    if ((fp= fopen(fname,"r")) == NULL)
    {
	/* Tag file is missing - make one assigning new uid */
	set_cfadm_if_httpd_home();
	pw.pw_uid= next_uid(1);
	pw.pw_gid= user_group_id();
	set_httpd_if_httpd_home();
	if ((fp= fopen(fname,"w")) == NULL)
	{
	    set_cfadm_if_httpd_home();
	    return NULL;
	}
	lock_exclusive(fileno(fp),fname);
	fprintf(fp,"*uid=%d\n*gid=%d\n", pw.pw_uid, pw.pw_gid);
	fclose(fp);
	unlock(fname);
	set_cfadm_if_httpd_home();
	return &pw;
    }
    lock_shared(fileno(fp),fname);
    
    /* Read through tag file - probably should use get_tag() here, but aren't */
    while (fgets(bf,BFSZ,fp) != NULL)
    {
	/* Parse apart the line */
	var= bf+ (bf[0] == '*');
	if ((val= strchr(var, '=')) == NULL)
	{
	    fclose(fp);
	    unlock(fname);
	    set_cfadm_if_httpd_home();
	    return NULL;
	}
	*(val++)= '\0';
	end= val + strlen(val) - 1;
	if (*end == '\n')
	    *(end--)= '\0';
	else
	    /* skip over rest of over-long line */
	    while ((ch= fgetc(fp)) != '\n' && ch != EOF)
		;

	if (!strcmp(var, "uid"))
	{
	    if (!isascii(*val) || !isdigit(*val))
	    {
		fclose(fp);
		unlock(fname);
		set_cfadm_if_httpd_home();
		return NULL;
	    }
	    pw.pw_uid= atoi(val);
	}
	else if (!strcmp(var, "gid"))
	{
	    if (!isascii(*val) || !isdigit(*val))
	    {
		fclose(fp);
		unlock(fname);
		set_cfadm_if_httpd_home();
		return NULL;
	    }
	    pw.pw_gid= atoi(val);
	}
    }

    fclose(fp);
    unlock(fname);
    set_cfadm_if_httpd_home();
    return &pw;
}
#endif


/* IDENT_NEWFNAME - change the user's fullname to the given value.
 */

#ifdef HAVEIDENT_NEWFNAME
void ident_newfname(struct passwd *pw, char *newfname)
{
    char *fname, *p;
    FILE *fp;

    /* Construct name of ident file */
    fname= gethomedir(pw->pw_name);
    strcat(fname,"/");
    strcat(fname,YAPP_PASSWD_FILE);

    flushusercache();

    /* Clean up new full name */
    yf_clean(newfname);

    /* Write new passwd file */
    set_httpd_if_httpd_home();
    if ((fp= fopen(fname,"w")) == NULL)
	die("Cannot write to %s",fname);

    fprintf(fp,"%s:%s:%s\n",pw->pw_name, newfname, pw->pw_shell);

    fclose(fp);
    set_cfadm_if_httpd_home();
}
#endif


/* IDENT_NEWEMAIL - change the user's email address to the given value.
 */

#ifdef HAVEIDENT_NEWEMAIL
void ident_newemail(struct passwd *pw, char *newemail)
{
    char *fname, *p;
    FILE *fp;

    /* Construct name of ident file */
    fname= gethomedir(pw->pw_name);
    strcat(fname,"/");
    strcat(fname,YAPP_PASSWD_FILE);

    flushusercache();

    /* Clean up new email address */
    yf_clean(newemail);

    /* Write new passwd file */
    set_httpd_if_httpd_home();
    if ((fp= fopen(fname,"w")) == NULL)
	die("Cannot write to %s",fname);

    fprintf(fp,"%s:%s:%s\n",pw->pw_name, pw->pw_gecos, newemail);

    fclose(fp);
    set_cfadm_if_httpd_home();
}
#endif

/* IDENT_NEWGID - change the user's groupid to the given value.
 */

#ifdef HAVEIDENT_NEWGID
void ident_newgid(struct passwd *pw, gid_t newgid)
{
    char *fname;

    /* Construct name of ident file */
    fname= gethomedir(pw->pw_name);
    strcat(fname,"/");
    strcat(fname,IDENT_FILE);

    flushusercache();

    save_tag(fname, 1, "gid", TK_INTEGER, newgid);
}
#endif


/* IDENT_ADD - Add an entry to the user database describing a user with
 * the given login, uid, gid, password, fullname, and directory.
 */

#ifdef HAVEIDENT_ADD
void ident_add(char *lid, uid_t uid, gid_t gid, char *fn, char *dir,
	char *email)
{
    char fname[BFSZ];
    FILE *fp;

    yf_clean(fn);
    yf_clean(email);

    set_httpd_if_httpd_home();
    sprintf(fname,"%s/"YAPP_PASSWD_FILE,dir);
    if ((fp= fopen(fname,"w")) == NULL)
	die("could not create ident file %s",fname);
    fprintf(fp,"%s:%s:%s\n",lid,fn,(email == NULL) ? "" : email);
    fclose(fp);

    sprintf(fname,"%s/"IDENT_FILE,dir);
    if ((fp= fopen(fname,"w")) == NULL)
	die("could not create ident file %s",fname);
    lock_exclusive(fileno(fp),fname);
    fprintf(fp,"*uid=%d\n*gid=%d\n", uid, gid);
    fclose(fp);
    unlock(fname);
    set_cfadm_if_httpd_home();
}
#endif


/* IDENT_DEL - Delete the named user from the ident database.  Actually, there
 * is nothing to do here, since we will always be deleting the directory
 * next, and that will take care of everything.
 */

#ifdef HAVEIDENT_DEL
void ident_del(char *name) { }
#endif
