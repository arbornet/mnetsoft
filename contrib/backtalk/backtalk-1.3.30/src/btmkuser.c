/* (c) 1996-2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <signal.h>
#include <pwd.h>
#if HAVE_SETRLIMIT
#include <sys/resource.h>
#endif /* HAVE_SETRLIMIT */

#include "str.h"
#include "lock.h"
#include "group.h"
#include "partdir.h"
#include "udb.h"
#include "udb_check.h"
#include "adm_check.h"
#include "set_uid.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef ID_GETUID
int direct_exec= FALSE;
#endif


char *getpass();


/* LOG - Log the account creation.
 */

void logmsg(char *lid, char *fn, char *dir)
{
    char *var, *val;
    FILE *fp;
    time_t clock= time(0L);

    if ((fp= fopen(ACCT_LOG_FILE,"a")) == NULL)
	return;
    lock_exclusive(fileno(fp),ACCT_LOG_FILE);

    fprintf(fp,"-- %15.15s btmkuser: %s (%s)\n dir=%s\n",
	ctime(&clock)+4,lid,fn,dir);
    fclose(fp);
    unlock(ACCT_LOG_FILE);
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
    char bf[BFSZ],err[BFSZ],dir[BFSZ];
    struct passwd *pw;
    char *fullname= NULL, *passwd= NULL, *login= NULL;
    char *passwd2;
    FILE *fp;
#ifdef USER_GID
    gid_t gid= USER_GID;
#else
    gid_t gid= groupid(USER_GROUP);
#endif
    int i;
    int init_status= 0;
    time_t regdate= -1;

#if NOPWEDIT
    fprintf(stderr,"%s not supported at this site.\n",argv[0]);
    exit(1);
#else
    init_cmd(argv[0]);

    for (i= 1; i < argc; i++)
    {
	if (argv[i][0] != '-' || argv[i][2] != '\0') goto usage;

	switch(argv[i][1])
	{
	case 'l':
	    if (++i >= argc) goto usage;
	    login= argv[i];
	    if (login_bad(login,err))
	    {
		printf("Login %s bad: %s",login,err);
		exit(1);
	    }
	    else if (getpassword(login,FALSE))
	    {
		printf("Login %s already exists.\n",login);
		exit(1);
	    }
	    break;

	case 'n':
	    if (++i >= argc) goto usage;
	    fullname= argv[i];
	    if (fullname_bad(fullname,err))
	    {
		printf("Fullname %s bad: %s",fullname,err);
		exit(1);
	    }
	    break;

	case 'p':
	    if (++i >= argc) goto usage;
	    passwd= argv[i];
	    /* Don't do sanity checking until we have login */
	    break;

	case 'g':
	    if (++i >= argc) goto usage;
	    if (argv[i][0] >= '0' && argv[i][0] <= '9')
		gid= atoi(argv[i]);
	    else
		gid= groupid(argv[i]);
	    break;

	case 'R':
	    if (++i >= argc) goto usage;
	    if (argv[i][0] >= '0' && argv[i][0] <= '9')
		regdate= atoi(argv[i]);
	    else
		goto usage;
	    break;

	case 'v':
	    init_status= 1;
	    break;

	default:
	    goto usage;
	}
    }

    /* If no command-line login, read one in */
    while (login == NULL)
    {
	printf("Login name: ");
	if (fgets(bf,BFSZ,stdin) == NULL || bf[0] == '\n') exit(1);
	*firstin(bf,"\n")= '\0';
	if (login_bad(bf,err))
	    fputs(err,stdout);
	else if (getpassword(bf,FALSE))
	    printf("Login %s already exists.\n",bf);
	else
	    login= strdup(bf);
    }

    /* If no command-line fullname, read one in */
    while (fullname == NULL)
    {
	printf("Full name: ");
	if (fgets(bf,BFSZ,stdin) == NULL || bf[0] == '\n') exit(1);
	*firstin(bf,"\n")= '\0';
	if (fullname_bad(bf,err))
	    fputs(err,stdout);
	else
	    fullname= strdup(bf);
    }

    /* Do sanity check for command-line passwords */
    if (passwd != NULL)
    {
	if (passwd_bad(passwd,login,err))
	{
	    printf("Password bad: %s",err);
	    exit(1);
	}
    }
    else
    {
	/* If no command-line password, read one in */
	while (passwd == NULL)
	{
	    if ((passwd2= getpass("Password: ")) == NULL || passwd2[0] == '\0')
		exit(1);
	    if (passwd_bad(passwd2,login,err))
	    {
		fputs(err,stdout);
		continue;
	    }
	    passwd2= strdup(passwd2);
	    if ((passwd= getpass("Password Again: ")) == NULL ||
		    passwd[0] == '\0')
		exit(1);
	    if (strcmp(passwd,passwd2))
	    {
		printf("Passwords don't match.\n");
		free(passwd2);
		passwd= NULL;
		continue;
	    }
	}
	free(passwd2);
    }

    /* Hold some signals */
    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    set_cfadm();

    /* Create conference account */
    adduser(login, gid, passwd, fullname, init_status, dir, NULL);

    /* Log the creation */
    logmsg(login,fullname,dir);

    /* Create a .backtalk file with just the registration date */
#ifdef PART_DIR
    bbspartpath(bf,login,".backtalk");
    mkpartdir(bf);
#else
    sprintf(bf,"%s/.backtalk", dir);
#endif
    if ((fp= fopen(bf,"w")) != NULL)
    {
	fprintf(fp,"regdate=%ld\n",regdate < 0 ? time(0L) : regdate);
	fclose(fp);
    }

    printf("Account %s created.\n",login);
    exit(0);
usage:
    fprintf(stderr,"usage: %s [-l <login>] [-g <group>] [-p <password>] "
		    "[-n <fullname>] [-v] [-R <unixtime>]\n",argv[0]);
    exit(1);
#endif
}
