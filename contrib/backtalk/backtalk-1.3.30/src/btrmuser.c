/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <pwd.h>
#if HAVE_SETRLIMIT
#include <sys/resource.h>
#endif /* HAVE_SETRLIMIT */

#include "udb.h"
#include "set_uid.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef ID_GETUID
int direct_exec= FALSE;
#endif


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
    char bf[BFSZ];
    struct passwd *pw;
    int force=0, i;

#if NOPWEDIT
    fprintf(stderr,"%s not supported at this site.\n",argv[0]);
    exit(1);
#else
    init_cmd(argv[0]);

    if (argc > 1 && !strcmp(argv[1],"-f"))
	force= 1;

    if (argc <= 1+force)
    {
	fprintf(stderr,"usage: %s [-f] <username>...\n",argv[0]);
	exit(1);
    }

    for (i= 1+force; i < argc; i++)
    {
	if ((pw= getdbnam(argv[i])) == NULL)
	{
	    fprintf(stderr,"Backtalk account %s does not exist.\n", argv[i]);
	    continue;
	}
	if (!force)
	{
	    fprintf(stderr,"Delete %s Backtalk account? ", argv[i]);
	    fgets(bf,BFSZ,stdin);
	    if (bf[0] != 'y' && bf[0] != 'Y')
		continue;
	}

	if (kill_user(pw))
	    fprintf(stderr,"could not kill %s.\n",argv[i]);
	else
	    fprintf(stderr,"%s deleted.\n",argv[i]);
    }
    exit(0);
#endif
}
