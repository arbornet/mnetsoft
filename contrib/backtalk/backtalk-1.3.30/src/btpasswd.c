/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <pwd.h>
#if HAVE_SETRLIMIT
#include <sys/resource.h>
#endif /* HAVE_SETRLIMIT */

#include "str.h"
#include "udb.h"
#include "udb_check.h"
#include "set_uid.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef ID_GETUID
int direct_exec= FALSE;
#endif


char *getpass();


void init_cmd(char *progname)
{
#if HAVE_SETRLIMIT
    struct rlimit rlim;

    /* Don't dump core (dumps could contain sensitive data) */
    rlim.rlim_cur = rlim.rlim_max = 0;
    (void)setrlimit(RLIMIT_CORE, &rlim);
#endif

    if (getuid() == 0)
	set_root_uid();
    else
	set_httpd();
    if (geteuid() != SERVER_UID)
    {
	fprintf(stderr,"%s: This program should be installed setuid "
	   "to uid=%d.\n",progname,SERVER_UID);
	exit(1);
    }

    set_cfadm();
    if (geteuid() != CFADM_UID)
    {
	fprintf(stderr,"%s: You are not the conference adminstrator \n",
	    progname);
	exit(1);
    }
}


main(int argc, char **argv)
{
    char err[BFSZ];
    char *username;
    char *newpw,*duppw;
    struct passwd *pw;

#if NOPWEDIT
    fprintf(stderr,"%s not supported at this site.\n",argv[0]);
    exit(1);
#else
    init_cmd(argv[0]);

    if (argc != 2)
    {
	fprintf(stderr,"usage: %s <username>\n",argv[0]);
	exit(1);
    }
    username= argv[1];

    if ((pw= getdbnam(username)) == NULL)
    {
	fprintf(stderr,"%s: User %s not found.\n",argv[0],username);
	exit(1);
    }

    for (;;)
    {
	if ((newpw= getpass("New Password: ")) == NULL ||
	    newpw[0] == '\0')
	{
	    fprintf(stderr,"%s: No new password given.\n",argv[0]);
	    exit(1);
	}

	if (passwd_bad(newpw,pw->pw_name,err))
	{
	    fprintf(stderr,"%s: %s",argv[0],err);
	}
	else
	{
	    newpw= strdup(newpw);
	    if ((duppw= getpass("New Password Again: ")) == NULL ||
		    newpw[0] == '\0')
		exit(1);
	    if (!strcmp(newpw,duppw))
		break;

	    fprintf(stderr,"%s: Typed passwords do match\n",
		argv[0]);
	    free(newpw);
	}
    }

    change_passwd(pw,mkpasswd(newpw,0));

    fprintf(stderr,"Password changed.\n");

    exit(0);
#endif
}
