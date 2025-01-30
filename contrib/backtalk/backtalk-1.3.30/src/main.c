/* (c) 1996-2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <signal.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "sysdict.h"
#include "cgi_query.h"
#include "exec.h"
#include "str.h"
#include "stack.h"
#include "dict.h"
#include "debug.h"
#include "lock.h"
#include "date.h"
#include "user.h"
#include "conf.h"
#include "format.h"
#include "showopt.h"
#include "last_log.h"
#include "byteorder.h"
#include "ps_part.h"
#include "ps_item.h"
#include "ps_sum.h"
#include "set_uid.h"
#include "sql.h"
#include "baaf.h"


#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef MTRACE
#include <mcheck.h>
#endif

int initializing;
int ttymode= FALSE;

FILE *output;			/* Where to write script output */

void logmsg(char *, char *);
#ifdef TRAP_CRASH
void set_sigtrap(void);
void unset_sigtrap(void);
#endif /*TRAP_CRASH*/
int direct_exec= FALSE;

void send_favicon(char *path);

#ifdef DEBUG_MEMORY
/* Really just a place to set a breakpoint */
void merr(int errcode)
{
    printf("MEMORY ERROR: code %d\n",errcode);
}
#endif

/* Crash after a timeout */
RETSIGTYPE timeout(int arg)
{
    die("program terminated by timeout");
}


main (int argc, char **argv)
{
    char *progname, *scriptdir, *id, *query= NULL;
    char *file= NULL;
    int i, f, hflag=FALSE, is_public, saverep= TRUE, backfile= FALSE;
#ifdef TRAP_CRASH
    int trapsig= TRUE;
#endif /* TRAP_CRASH */

#ifdef DEBUG_MEMORY
    malloc_debug(2+4+8);
    malloc_error(merr);
#endif
#ifdef MTRACE
    mtrace();
#endif

    /* Default place to write script output */
    output= stdout;

    /* Set default umask */
    umask(022);

    /* Decide if we are big-endian or little-endian */
    check_endian();

    /* Set time limit to trap infinite loops */
    signal(SIGALRM, timeout);

    /* Save program name (for use in debug output) */
    if ((progname= strrchr(argv[0],'/')) == NULL)
	sets(VAR_PROGNAME,progname= argv[0],SM_COPY);
    else
	sets(VAR_PROGNAME,++progname,SM_COPY);

    /* Parse arguments - Most are for debugging only */
    for (i= 1; i < argc; i++)
    {
	if (argv[i][0] == '-')
	    switch(argv[i][1])
	    {
	    case 'v':	/* Display configuration and exit */
	        fputs(show_options(0),stdout);
		exit(0);
	    case 'h':	/* Run in !ttymode even on tty */
		hflag= TRUE;
		break;
#ifdef TRAP_CRASH
	    case 's':	/* Don't trap signals */
		trapsig= FALSE;
		break;
#endif
	    case 'f':	/* Execute named script */
		file= argv[++i];
		ttymode= TRUE;
		break;
	    case 'q':	/* Execute given query string */
		query= argv[++i];
		ttymode= TRUE;
		break;
#ifdef ID_GETUID
	    case 'd':	/* Direct execution - force getting UID with getuid() */
	    	direct_exec= TRUE;
		break;
#endif
	    case 'r':	/* Reexecute last query */
		ttymode= TRUE;
		saverep= FALSE;
		if (i+1 < argc)
		    load_repeat(argv[++i]);
		else
		    load_repeat(REPEAT_FILE);
		break;
	    }
    }
    if (hflag) ttymode= FALSE;

#ifdef TRAP_CRASH
    if (trapsig)
	set_sigtrap();
#endif

    /* Initialize the system dictionary - it is mostly pre-compiled */
    init_sysdict();

#if ATTACHMENTS
    backfile= (!strncmp(progname,"backfile",8) &&
	    (progname[8] == '\0' || progname[8] == '.'));
#endif

    if (!backfile)
    {
	char *t;

	/* Find the script to execute */
	if (file == NULL) file= getenv("PATH_INFO");
	
	/* Strip off leading slashes */
	if (file != NULL)
	    while (file[0] == '/')
		file++;

	/* If no script, default to the welcome script */
	if (file == NULL || *file == '\0') file= "public/welcome";

	/* Don't allow paths into parent directories */
	nodotdot(file);

	/* Strip off anything after ':' for urlarg */
	if ((t= strchr(file,':')) != NULL)
	{
	    sets(VAR_URLARG,t+1,SM_COPY);
	    file= strndup(file, t-file);
	}

	/* Set "flavor" to directory name, and "scriptname" to rest of path */
	if ((t= strchr(file,'/')) != NULL)
	{
	    setsn(VAR_FLAVOR,file,t-file);
	    sets(VAR_SCRIPTNAME,t+1,SM_COPY);
	}
    }

    /* cd into the script directory */
    if (chdir(scriptdir= strdup(sdict(VAR_SCRIPTDIR))))
	die("script directory \"%s\" not accessible",scriptdir);

    /* Run the "config.bt" script first
     * (initializing flag lets us redefine constants and protected vars)
     */
    initializing= TRUE;
    run("config.bb");
    initializing= FALSE;

    /* Is this a public page? */
    is_public= !backfile &&
	inlist(sdict(VAR_FLAVOR),sdict(VAR_PUBLICFLAVORS),", ");

    /* For non-public scripts, look up and save user information */
    if (!is_public && getuser(bdict(VAR_SECURE)) == 2)
    {
	/* If session is expired, run relogin script, passing in the
 	 * original script name as if it had been a CGI argument
	 */
	set_sys_dict("oldscript",file,SM_COPY);
	file= "public/relogin";  /* script for expired sessions */
	is_public= 1;
    }

    /* Get User information */
    seti(VAR_UID, user_uid());
    seti(VAR_GID, user_gid());
    sets(VAR_HOMEDIR, user_dir(), SM_COPY);
    id= user_id();
    sets(VAR_ID, id, SM_COPY);
    seti(VAR_AMADM, user_cfadm());

    /* Update the lastlog file */
    update_lastlog(idict(VAR_UID));

#if ATTACHMENTS
    if (backfile)
    {
	baaf_file(getenv("PATH_INFO"));
	exit(0);
    }
#endif

    /* If we don't allow anonymous, any anonymous user trying to run
     * anything not in the directories listed in 'publicflavors' runs
     * public/welcome
     */
    if (*id == '\0' && !idict(VAR_ALLOWANON) && !is_public)
    {
#ifdef ID_SESSION
	file= "public/login";
#else
	file= "public/welcome";
#endif
	is_public= 1;
    }

    /* Remove trailing /'s from  file */
    i= strlen(file);
    while (i > 0 && file[i-1] == '/')
	file[--i]= '\0';

    /* If a favicon.ico request, don't try to run a script */
    if (i >= 11  && !strcmp(file+i-11,"favicon.ico"))
    {
	send_favicon(sdict(VAR_FAVICON));
	exit(0);
    }

    /* Set mayseefname flag */
    switch (idict(VAR_ANONYMITY))
    {
    default:
    case 0: f= 1; break;
    case 1: f= (id[0] != '\0'); break;
    case 2: f= bdict(VAR_AMADM) || user_gradm(); break;
    case 3: f= bdict(VAR_AMADM); break;
    }
    seti(VAR_MAYSEEFNAME,f);

    /* cd into the new script directory, if it has changed */
    if (strcmp(scriptdir,sdict(VAR_SCRIPTDIR)) &&
	chdir(sdict(VAR_SCRIPTDIR)))
	die("script directory \"%s\" not accessible",sdict(VAR_SCRIPTDIR));

    /* Get the Query - if it hasn't already been set by debugging option */
    if (query == NULL) query= get_query(saverep && bdict(VAR_SAVEREP));
    if (idict(VAR_LOGLEVEL) > 0) logmsg(file,query);
    if (query != NULL) sets(VAR_CGIQUERY,query,SM_COPY);
    /* This should become a built in command, called first in most scripts */
    load_query(query);

    /* Execute the selected script */
    run(file);

    /* Write out participation files */
    func_close_conf();

    /* Dump the stack */
    func_print();

    /* If CLEANUP is defined, deallocate everything we know about in
     * dynamic memory.  This is done to help detect memory leaks.
     */

#ifdef CLEANUP
    free(scriptdir);
    flushusercache();			/* Free cached user */
    free_exec();			/* Free execution stack */
    free_dictstack();			/* Free dictionary stack */
    free_file_list();			/* Free file list */
    free_stack();			/* Free operand stack */
    free_cookies();			/* Free any pending cookies */
    free_timezone();			/* Free any timezone information */
    free_user();			/* Free user information */
    free_clist();			/* Free any cached conflist stuff */
    free_search();			/* Free any search data */
    free_format();			/* Free text formating tables */
    if (old_env) free_repeat();		/* free environment */
    if (!strcmp(getenv("REQUEST_METHOD"),"POST"))
	free(query);			/* free query, if malloc'ed */
#endif /*CLEANUP*/

    exit(0);
}


/* SEND_FAVICON - send the .ico file found in the given file.
 */

void send_favicon(char *path)
{
FILE *fp= fopen(path,"r");
int ch;

    /* Maybe this should be image/ico or image/x-ico or image/x-ms-icon or
     * any of several other things, but the normal response from most web
     * servers seems text/plain so we'll stick with that for now.
     */
    printf("Content-type: text/plain\n\n");

    if (path == NULL || path[0] == '\0' || (fp= fopen(path,"r")) == NULL)
    	return;

    while ((ch= getc(fp)) != EOF)
    	putchar(ch);
}


static int in_die= FALSE;

/* DIE - Exit with an error condition.  It prints suitable HTML headers
 * (if they haven't already been displayed), and so forth.  It also prints
 * some context information which leaks in here via various global variables,
 * plus dumps the stack.
 */

#if __STDC__
void die(const char *fmt,...)
#else
void die(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
    time_t now= time(0L);
    char *timestr;
    va_list ap;
    char version[BFSZ];
    char msg[BFSZ*2];
#ifdef ERROR_FILE
    int fd;
    FILE *debugErrs= NULL;
#endif

    /* If we call die() from inside die() - just exit */
    if (in_die) exit(0);
    in_die= TRUE;

    timestr= ctimez(&now,NULL);

    set_cfadm();

    VA_START(ap,fmt);
    vsnprintf(msg, BFSZ*2-1, fmt, ap);
    va_end(ap);

    /* If possible, reset http headers to HTML */
    sets(VAR_HTTP_CONTENT_TYPE, "text/html", SM_COPY);
    sets(VAR_HTTP_LOCATION, "", SM_COPY);

    send_http_headers();

    version_string(version);

    if (!ttymode)
    {
	fputs("<HTML><HEAD>\n<TITLE>Backtalk Crash</TITLE>\n"
	      "</HEAD><BODY BGCOLOR=FFFFFF TEXT=A00000>\n"
	      "<H1>Backtalk Crash!</H1>\n",stdout);

	printf("%24.24s<P>\n", timestr);
	fputs("<STRONG>Oops!</STRONG>  The authors of Backtalk are deeply\n"
	      "sorry to confess that our program has failed to operate\n"
	      "correctly.  This is due to an error in Backtalk itself, or\n"
	      "in one of Backtalk's script files, or in the configuration\n"
	      "of the system.  It is probably not due to anything you might\n"
	      "have done.\n", stdout);

	fputs("The page below contains an error message and a detailed dump\n"
	      "of the state Backtalk was in when the error occurred. This\n"
	      "may be useful to anyone trying to fix the problem.<P>\n",stdout);
    }

#ifdef ERROR_FILE
    /* Don't create the error log file if it doesn't already exist */
    if ((fd= open(ERROR_FILE, O_WRONLY|O_APPEND)) >= 0)
	debugErrs= fdopen(fd,"a");

    if (debugErrs != NULL)
    {
	fprintf(debugErrs,"------- %24.24s -------\n",timestr);

	fputs("DIED:  ", debugErrs);
	fputs(msg, debugErrs);
	fputc('\n', debugErrs);
	print_context(debugErrs);

	fprintf(debugErrs, "\n\nVersion:  %s", version);

	fputs("\n\nStack:  ", debugErrs);
	dump_stack(debugErrs, FALSE);

	putc('\n', debugErrs);
	dump_dict_stack(debugErrs);

	fputs("\nEnvironment:\n", debugErrs);
	dump_env(debugErrs, FALSE);

	if (!ttymode)
	    fputs("All this information has been logged, but it may still\n"
		  "be helpful for you to report this error and the\n"
		  "circumstances under which it occurred to the system\n"
		  "administrators.\n",stdout);
    }
    else
#endif
    if (!ttymode)
	fputs("This information has <STRONG>not</STRONG> been logged, so\n"
	      "it would be helpful for you to report this error to the\n"
	      "system administrators..\n",stdout);

    if (!ttymode)
	fputs("<P>Thanks for your assistance in getting the bugs out of this\n"
	      "conferencing system.  We sincerely and humbly beg your forgiveness.\n"
	      "<P><HR>\n", stdout);

    fputs(ttymode ? "\nERROR:  " : "<P>\nERROR:  ", stdout);
    fputs(msg, stdout);
    fputs(ttymode ? "\n" : "<BR>\n", stdout);
    print_context(stdout);

    printf(ttymode ? "\n\nVersion:  %s" : "<P>Version: %s\n", version);

    fputs(ttymode ? "\n\nStack:\n" : "<P><PRE>Stack:\n", stdout);
    dump_stack(stdout, !ttymode);

    putc('\n', stdout);
    dump_dict_stack(stdout);

    fputs(ttymode ? "\nEnvironment:\n" : "<P>\nEnvironment:\n", stdout);
    dump_env(stdout, !ttymode);

    if (!ttymode) fputs("</PRE></BODY></HTML>\n", stdout);

    /* Make sure we zap any lock files we may have */
    closepart();
    closeitem();
    closesum();

#ifdef USE_SQL
    sql_disconnect();
#endif

    exit(0);
}


/* DIE_NOSCRIPT - Exit error condition.  This is used when attempting to run
 * non-existant scripts.  It creates a abbreviated and less apologetic crash
 * screen, because this is usually not our fault.
 */

void die_noscript(char *file)
{
    time_t now= time(0L);
    char *timestr;
    char *e;
#ifdef ERROR_FILE
    int fd;
    FILE *debugErrs= NULL;
#endif

    /* If we call die() from inside die() - just exit */
    if (in_die) exit(0);
    in_die= TRUE;

    timestr= ctimez(&now,NULL);

    set_cfadm();

    send_http_headers();

    if (!ttymode)
    {
	fputs("<HTML><HEAD>\n<TITLE>Backtalk Script Not Found</TITLE>\n"
	      "</HEAD><BODY BGCOLOR=FFFFFF TEXT=A00000>\n"
	      "<H1>Backtalk Script Not Found</H1>\n",stdout);

	printf("%24.24s<P>\n", timestr);
    }

#ifdef ERROR_FILE
    /* Don't create the error log file if it doesn't already exist */
    if ((fd= open(ERROR_FILE, O_WRONLY|O_APPEND)) >= 0)
	debugErrs= fdopen(fd,"a");

    if (debugErrs != NULL)
    {
	fprintf(debugErrs,"------- %24.24s -------\n",timestr);

	fprintf(debugErrs,"DIED:  Script %s does not exist\n",file);
	print_context(debugErrs);

	fprintf(debugErrs,"\ncgiquery=%s\n",sdict(VAR_CGIQUERY));
	fprintf(debugErrs,"REQUEST_URI=%s\n",(e= getenv("REQUEST_URI"))?e:"");
	fprintf(debugErrs,"PATH_INFO=%s\n",(e= getenv("PATH_INFO"))?e:"");
	fprintf(debugErrs,"QUERY_STRING=%s\n",(e= getenv("QUERY_STRING"))?e:"");
	fprintf(debugErrs,"HTTP_REFERER=%s\n",(e= getenv("HTTP_REFERER"))?e:"");
	fprintf(debugErrs,"HTTP_USER_AGENT=%s\n",
					(e= getenv("HTTP_USER_AGENT"))?e:"");
	fprintf(debugErrs,"REMOTE_ADDR=%s\n",(e= getenv("REMOTE_ADDR"))?e:"");
	fprintf(debugErrs,"REMOTE_PORT=%s\n",(e= getenv("REMOTE_PORT"))?e:"");
	fprintf(debugErrs,"REMOTE_USER=%s\n",(e= getenv("REMOTE_USER"))?e:"");
    }
#endif

    printf("You have attempted to run a Backtalk script named '%s'.\n"
	    "There is no Backtalk script by this name.\n"
	    "If you typed the URL in yourself, then you entered it\n"
	    "incorrectly.  If you got this message after clicking a link,\n"
	    "then that link was erroneous.  If the link was on a Backtalk\n"
	    "page, then probably Backtalk is broken.  Please inform the\n"
	    "system administrators if this is the case.\n",file);

    if (!ttymode) fputs("<PRE>\n",stdout);

    printf("cgiquery=%s\n",sdict(VAR_CGIQUERY));
    printf("REQUEST_URI=%s\n",(e= getenv("REQUEST_URI"))?e:"");
    printf("PATH_INFO=%s\n",(e= getenv("PATH_INFO"))?e:"");
    printf("QUERY_STRING=%s\n",(e= getenv("QUERY_STRING"))?e:"");
    printf("REMOTE_ADDR=%s\n",(e= getenv("REMOTE_ADDR"))?e:"");
    printf("REMOTE_PORT=%s\n",(e= getenv("REMOTE_PORT"))?e:"");
    printf("REMOTE_USER=%s\n",(e= getenv("REMOTE_USER"))?e:"");

    if (!ttymode) fputs("</PRE></BODY></HTML>\n",stdout);

    /* Make sure we zap any lock files we may have */
    closepart();
    closeitem();
    closesum();

    exit(0);
}


/* LOG - write a log entry for the current transaction.  Don't call if loglevel
 * is zero.  The original plan was to output more information with higher
 * loglevels, but there doesn't really seem to be all that much interesting
 * information to output.
 */

void logmsg(char *file, char *query)
{
    time_t clock= time(0L);
    char *datum;
    FILE *fp;

    if ((fp= fopen(LOG_FILE,"a")) == NULL)
	return;
    lock_exclusive(fileno(fp),LOG_FILE);

    fprintf(fp,"%15.15s %s", ctimez(&clock,NULL)+4, file);

    if ((datum= sdict(VAR_ID)) != NULL && datum[0] != '\0')
	fprintf(fp, " %s", datum);
    else
	fprintf(fp, " anonymous");
    if ((datum= getenv("REMOTE_HOST")) != NULL)
	fprintf(fp, " %s", datum);
    if ((datum= getenv("REMOTE_ADDR")) != NULL)
	fprintf(fp, "(%s)", datum);

    if (query != NULL)
	fprintf(fp,"\n %s\n", query);
    else
	fputc('\n',fp);

    fclose(fp);
    unlock(LOG_FILE);
}

#ifdef TRAP_CRASH

/* SIGTRAP - This routine is used to catch weird signals, like segmentation
 * violations, that would normally cause crashes and core dumps.  It tries
 * to generate a backtalk crash screen instead of a core dump, on the
 * assumption that in a CGI environment the former is more useful.  In many
 * cases this won't work, since whatever caused the crash may have screwed
 * things up too badly to be able to even die gracefully, but it's worth
 * a try.
 */

RETSIGTYPE sigtrap(int arg)
{
    /* Could skip unset on systems with oneshot signal signal handlers */
    unset_sigtrap();

    die("terminated by signal %d",arg);
}


/* SET_SIGTRAP - set traps on most signals that could kill the process.
 */

void set_sigtrap()
{
#ifdef SIGABRT
    signal(SIGABRT,sigtrap);
#endif
#ifdef SIGBUS
    signal(SIGBUS,sigtrap);
#endif
#ifdef SIGFPE
    signal(SIGFPE,sigtrap);
#endif
#ifdef SIGHUP
    signal(SIGHUP,sigtrap);
#endif
#ifdef SIGILL
    signal(SIGILL,sigtrap);
#endif
#ifdef SIGINT
    signal(SIGINT,sigtrap);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE,sigtrap);
#endif
#ifdef SIGPWR
    signal(SIGPWR,sigtrap);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV,sigtrap);
#endif
#ifdef SIGSYS
    signal(SIGSYS,sigtrap);
#endif
/* Don't trap SIGTERM - http servers send them often and we should honor them
#ifdef SIGTERM
    signal(SIGTERM,sigtrap);
#endif
*/
#ifdef SIGUSR1
    signal(SIGUSR1,sigtrap);
#endif
#ifdef SIGUSR2
    signal(SIGUSR2,sigtrap);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT,sigtrap);
#endif
#ifdef SIGXCPU
    signal(SIGXCPU,sigtrap);
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ,sigtrap);
#endif
}


/* UNSET_SIGTRAP - set traps on most signals that could kill the process.
 */

void unset_sigtrap()
{
#ifdef SIGABRT
    signal(SIGABRT,SIG_DFL);
#endif
#ifdef SIGBUS
    signal(SIGBUS,SIG_DFL);
#endif
#ifdef SIGFPE
    signal(SIGFPE,SIG_DFL);
#endif
#ifdef SIGHUP
    signal(SIGHUP,SIG_DFL);
#endif
#ifdef SIGILL
    signal(SIGILL,SIG_DFL);
#endif
#ifdef SIGINT
    signal(SIGINT,SIG_DFL);
#endif
#ifdef SIGPIPE
    signal(SIGPIPE,SIG_DFL);
#endif
#ifdef SIGPWR
    signal(SIGPWR,SIG_DFL);
#endif
#ifdef SIGSEGV
    signal(SIGSEGV,SIG_DFL);
#endif
#ifdef SIGSYS
    signal(SIGSYS,SIG_DFL);
#endif
#ifdef SIGTERM
    signal(SIGTERM,SIG_DFL);
#endif
#ifdef SIGUSR1
    signal(SIGUSR1,SIG_DFL);
#endif
#ifdef SIGUSR2
    signal(SIGUSR2,SIG_DFL);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT,SIG_DFL);
#endif
#ifdef SIGXCPU
    signal(SIGXCPU,SIG_DFL);
#endif
#ifdef SIGXFSZ
    signal(SIGXFSZ,SIG_DFL);
#endif
}

#endif /*TRAP_CRASH*/
