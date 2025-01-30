/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <pwd.h>
#if HAVE_SETRLIMIT
#include <sys/resource.h>
#endif

#include "udb.h"
#include "set_uid.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef ID_GETUID
int direct_exec= FALSE;
#endif

int isy(char ch)
{
    if (ch == 'n' || ch == 'N') return 0;
    if (ch == 'y' || ch == 'Y') return 1;
    return 2;
}

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
    char *username;
    char *oldpw;
    char newpw[BFSZ];
    struct passwd *pw;
    int i,validate;

#if NOPWEDIT
    fprintf(stderr,"%s not supported at this site.\n",argv[0]);
    exit(1);
#else
    init_cmd(argv[0]);

    if (argc != 3 || (validate= isy(argv[2][0])) == 2)
    {
	fprintf(stderr,"usage: %s <username> y|n\n",argv[0]);
	exit(1);
    }
    username= argv[1];

    if ((oldpw= getpassword(username,FALSE)) == NULL ||
	(pw= getdbnam(username)) == NULL)
    {
	fprintf(stderr,"%s: user %s does not exist.\n",
	    argv[0], username);
	exit(1);
    }

    if (oldpw[0] == '*' || oldpw[0] == '?') oldpw++;
    i= 0;
    if (!validate) newpw[i++]= '*';
    strcpy(newpw+i, oldpw);

    change_passwd(pw, newpw);

    printf("User %s %svalidated.\n",username,validate?"":"in");
    exit(0);
#endif
}
