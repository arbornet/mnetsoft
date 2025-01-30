/* Copyright 1998, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* MAKECONF
 *
 * This program creates a backtalk conference.  It is meant to be used from
 * scripts and has truely hideous calling parameters.
 *
 * usage: makeconf <fw> <name> <alias> <title> <des> <login> <ulist>
 *
 * Parameters:
 *   Fairwitness (string)
 *      Comma-separated string of fairwitnesses.  If you don't want one, just
 *      set it to something like the name of an admin account, which is
 *      effectively a fairwitness anyway.
 *   Internal Conference Name (string) 
 *      Conference directory name - one word, all lower case - actually, I
 *      think this should be the project ID number.  That way I can use it to
 *      reference into the file database too.
 *   Conference Alias (string)
 *      Name people type at "Goto" box to enter a conference.  Like "alum_ni".
 *      All lower case with underscores to mark abbreviations, if any.  Could
 *      be more than one, like "alum_ni,grad_uates,grads".  Could be null, in
 *      which case the conference will be accessible only by number.
 *   Conference Title (string) 
 *      Short name of conference displayed to user all over the place, as in
 *      "<whatever> Conference Item 16".  Multiple words, mixed case.
 *   Conference Description (string)
 *      Long description string to display in menu of conferences.
 *   Login Screen (string)
 *      HTML login screen.  I'm thinking this might contain the conference name
 *      and the company logo.  Can contain newlines and such, or not.
 *   User List (string)
 *      Whitespace-separated list of entries like "user=janc" or "status=Faculty"
 *      or "project=239".  Anyone matching at least one of these requirements,
 *      plus any anyone with status=Admin, will be able to join the conference.
 *      Nobody else will.  The exception is that if the User list is an empty
 *      string, then the conference will be open to all users.
 *
 */

#include "common.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "adm_conf.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *getenv();

#if __STDC__
void die(const char *fmt,...)
#else
void die(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
va_list ap;

    VA_START(ap,fmt);
    fputs("Error in makeconf: ",stdout);
    vprintf(fmt, ap);
    va_end(ap);
    exit(1);
}


void add_confmenu(char *conf, char *title, char *descrip)
{
    FILE *fp;

    if ((fp= fopen(BBS_DIR"confmenu","a")) == NULL)
	die("cannot open "BBS_DIR"confmenu");

    fprintf(fp,"%s:%s:%s\n",conf,title,descrip);

    fclose(fp);
}


void install_ulist(char *dir, char *ulist)
{
    char bf[BFSZ];
    FILE *fp;
    int len= strlen(ulist);

    if (len == 0) return;

    sprintf(bf,"%.100s/ulist",dir);
    if ((fp= fopen(bf,"w")) == NULL)
	die("cannot open %s",bf);

    fputs(ulist,fp);
    if (ulist[len-1] != '\n')
       fputc('\n',fp);

    fclose(fp);
}


void install_login(char *dir, char *login)
{
    char bf[BFSZ];
    FILE *fp;
    int len= strlen(login);

    if (len == 0) return;

    sprintf(bf,"%.100s/settings",dir);
    if ((fp= fopen(bf,"w")) == NULL)
	die("cannot open %s",bf);

    fputs("login=(",fp);
    while (*login != '\0')
    {
	switch (*login)
	{
	case '\r':                   break;
	case '\n': fputs("\\n",fp);  break;
	case '\\': fputs("\\\\",fp); break;
	case '(':  fputs("\\(",fp);  break;
	case ')':  fputs("\\)",fp);  break;
	default:   fputc(*login,fp); break;
	}
	login++;
    }
    fputs(")\n",fp);

    fclose(fp);
}


main(int argc, char **argv)
{
    char *fw, *conf, *aliases, *title, *descrip, *login, *ulist;
    char *pfile, *err;
    char dir[BFSZ], *b, *e, tmp;
    uid_t uid;

    /* Check authorization */
    uid= getuid();
    if (uid != SERVER_UID && uid != CFADM_UID)
	die("this program can only be run by uids %d and %d",
	    SERVER_UID, CFADM_UID);

    /* Get the arguments */
    if (argc < 6 || argc > 8)
	die("usage: makeconf <fw> <name> <alias> <title> <des> <login> <ulist>\n");

    fw= argv[1];
    conf= argv[2];
    aliases= argv[3];
    title= argv[4];
    descrip= argv[5];

    if (argc > 6 && argv[6][0] != '\0')
	login= argv[6];
    else
	login= NULL;

    if (argc > 7 && argv[7][0] != '\0')
	ulist= argv[7];
    else
	ulist= NULL;

    /* Do various error checks */
    if (strchr(fw,'\n') != NULL)
	die("improper newline in fairwitness list");

    pfile= make_part_name(conf);
    if (err= check_conf(conf, BBS_DIR, pfile))
	die("%s",err);

    if (strchr(title,'\n') != NULL)
	die("improper newline in conference title");
    if (strchr(title,':') != NULL)
	die("improper colon in conference title");

    if (strchr(descrip,'\n') != NULL)
	die("improper newline in conference description");


    /* Construct full pathname of conference directory */
    sprintf(dir,BBS_DIR"%.100s",conf);

    /* Make the conference directory and config file */
    make_confdir(dir, pfile, fw, ulist?4:0, title[0]?title:NULL);

    /* Add the aliases to the conflist */
    if (aliases[0] != '\0')
    {
	b= aliases;
	do {
	    e= firstin(b,",");
	    tmp= *e;
	    *e= '\0';
	    add_conflist(b,dir);
	    b= e+1;
	} while (tmp != '\0');
    }
    add_conflist(conf,dir);

    /* Add an entry to the end of the conference menu */
    add_confmenu(conf,title,descrip);

    /* Install the ulist */
    if (ulist)
	install_ulist(dir,ulist);

    /* Install the login screen */
    if (login)
	install_login(dir,login);

    exit(0);
}
