/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* PARTICIPATION FILE UTILITY
 *
 * When backtalk coexists with yapp or picospan, the participation files
 * created by yapp or picospan may not be created with permissions that make
 * them writable to backtalk, and backtalk may not have the access required
 * to create or delete files from the user's .cfdir.
 *
 * To correct this, this program is installed as an suid root program which
 * backtalk can execute in order to perform certain operations on user's
 * participation and cflist files.  It permits the user's participation
 * files and .cflist to group CF_GID, so backtalk can read and write them.
 * It also creates and destroys these files.
 *
 *   partutil -c <conf> <user>		- create a participation file
 *   partutil -p <conf> <user>		- permit a participation file
 *   partutil -d <conf> <user>		- delete a participation file
 *   partutil -C <conf> <user>		- create a extension file
 *   partutil -P <conf> <user>		- permit a extension file
 *   partutil -D <conf> <user>		- delete a extension file
 *   partutil -l <user>                 - permit a .cflist
 *   partutil -b <user>                 - permit a .backtalk
 *   partutil -n <user>                 - permit a .plan
 *
 * The program will refuse to run for any user except the http server.
 */

#include "backtalk.h"

#include <pwd.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <errno.h>
extern int errno;

#include "sysdict.h"
#include "str.h"
#include "ps_config.h"
#include "ps_conflist.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char *progname;

main(int argc, char **argv)
{
    struct passwd *pw;
    int i,ext= 0;
    char *path,*conf= NULL,*user= NULL;
    char bf1[BFSZ+1];
    char bf2[BFSZ+1];
    FILE *fp;
    char op= 'p';
    struct stat st;

    progname= argv[0];

    if (getuid() != SERVER_UID)
	die("Permission to run %s denied.",argv[0]);

    for (i= 1; i < argc; i++)
    { 
	if (argv[i][0] == '-')
	{
	    switch (op= argv[i][1])
	    {
#ifdef PART_DIR
	    case 'n': conf= "-"; break;
	    case 'b': case 'l': case 'c': case 'd': case 'p':
	    case 'C': case 'D': case 'P':
		die("%s: Option -%c not available in this installation\n",
		    progname,op);
#else
	    case 'b': case 'l': case 'n':
	    	conf= "-";
		break;
	    case 'c': case 'd': case 'p':
	    	break;
	    case 'C': case 'D': case 'P':
		op= op - 'A' + 'a';
		ext= 1;
		break;
#endif /*PART_DIR */
	    default:
	    	goto usage;
	    }
	}
	else if (conf == NULL)
	    conf= argv[i];
	else if (user == NULL)
	    user= argv[i];
	else
	    goto usage;
    }

    if (user == NULL || conf == NULL) goto usage;

    if ((pw= getpwnam(user)) == NULL)
	die("user %s does not exist",user);
    /* Clip off office info */
    *firstin(pw->pw_gecos,",")= '\0';

    if (op == 'l' || op == 'b' || op == 'n')
    {
	int fd, mode;

	if (op == 'n')
	{
	    sprintf(bf2,"%s/.plan",pw->pw_dir);
	    mode= 0664;
	}
#ifndef PART_DIR
	else
	{
	    sprintf(bf2,"%s/.cfdir",pw->pw_dir);
	    if (op == 'l')
	    {
		sprintf(bf2,"%s%s/.cflist", pw->pw_dir,
			access(bf2,0) ? "" : "/.cfdir");
		mode= 0664;
	    }
	    else
	    {
		sprintf(bf2,"%s%s/.backtalk", pw->pw_dir,
			access(bf2,0) ? "" : "/.cfdir");
		mode= 0660;
	    }
	}
#endif

	if ((fd= open(bf2,O_RDONLY)) < 0)
	{
	    /* file doesn't exist -- create it */
	    umask(0000);
	    if ((fd= open(bf2,O_WRONLY|O_CREAT|O_EXCL,mode)) < 0)
		die("could not create file %s: %s",bf2,strerror(errno));
	    fchown(fd,pw->pw_uid,CF_GID);
	}
	else
	{
	    /* file does exist -- check ownership */
	    if (fstat(fd,&st) || st.st_uid != pw->pw_uid)
	    	die("user %s does not own file %s",user,bf2);

	    /* all OK, so permit it */
	    fchown(fd,-1,CF_GID);
	    fchmod(fd, mode);
	}
	close(fd);
	exit(0);
    }

#ifndef PART_DIR
    /* Look up conference in conflist to get conference directory */
    if ((path= cfpath(conf)) == NULL)
	die("conference %s does not exist",conf);
    
    /* read participation file name into bf1 */
    readconfig(path, bf1, NULL, NULL, NULL);

    /* Construct full participation file name in bf2 */
    sprintf(bf2,"%s/.cfdir",pw->pw_dir);
    if (access(bf2,0))
	sprintf(bf2,"%s/%s%s", pw->pw_dir, bf1, ext?"x":"");
    else
	sprintf(bf2,"%s/.cfdir/%s%s", pw->pw_dir, bf1, ext?"x":"");

    if (op == 'c')
    {
	int fd;

	umask(0000);
	setgid(CF_GID);
	setuid(pw->pw_uid);
	if ((fd= open(bf2,O_WRONLY|O_CREAT|O_EXCL,0660)) < 0)
	    die("could not create file %s: %s",bf2,strerror(errno));
	fp= fdopen(fd,"w");
	if (ext)
	    fprintf(fp,"!<px01>\n");
	else
	    fprintf(fp,"!<pr03>\n%s\n",pw->pw_gecos);
	fclose(fp);
    }
    else
    {
	/* Open a file and check if it is really a participation file */

	if ((fp= fopen(bf2,"r")) == NULL)
	    die("participation file %s does not exist",bf2);
	if (fstat(fileno(fp),&st) || st.st_uid != pw->pw_uid)
	    die("user %s does not own file %s",user,bf2);
	if (fgets(bf1,BFSZ,fp) == NULL ||
		(strcmp(bf1,"!<pr03>\n") &&
		 strcmp(bf1,"!<pr10>\n") &&
		 strcmp(bf1,"!<px01>\n")))
	    die("Bad magic number in %s",bf2);

	if (op == 'p')
	{

	    /* Change permissions on an existing file */
	    fchown(fileno(fp),-1,CF_GID);
	    fchmod(fileno(fp),0660);
	    fclose(fp);
	}
	else if (op == 'd')
	{
	    /* Destroy the file */
	    fclose(fp);
	    /* Become the user, so we can't delete anything we shouldn't */
	    setgid(pw->pw_gid);
	    setuid(pw->pw_uid);
	    unlink(bf2);
	}
    }
#endif /* !PART_DIR */

    exit(0);

usage:
#ifdef PART_DIR
    die("usage: %s [-bln] <user>\n", progname);
#else
    die("usage: %s [-pcd] <conf> <user>\n       %s [-bln] <user>\n",
        progname,progname);
#endif
}



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

    fprintf(stderr,"%s: ",progname);
    vfprintf(stderr, fmt, ap);
    fputc('\n',stderr);
    va_end(ap);
    exit(1);
}


/* Fake dictionary lookup for variables needed here */

char *csdict(int var)
{
    if (var == VAR_CONFLIST)
	return CONFLIST;
    else if (var == VAR_BBSDIR)
	return BBS_DIR;
    else
	die("Unknown variable %d",var);
}

