#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "wnu.h"
#include "nu.h"

/* VERSION HISTORY:
 * 06/11/96 - Last version before I started collecting version history. [jdw]
 * 06/24/97 - Added version history. [jdw]
 *          - Added cpt_pass/clear_pass declaration for new newuser version.
 * 06/26/01 - Rlimits
 */

#ifndef SYSNAME
#define SYSNAME "this system"
#endif

#ifndef WELCOME_HTML
#define WELCOME_HTML "welcome.html"
#endif

#ifndef HTTP_UID
#define HTTP_UID 65534
#endif

int log_fwrt= 0;

/* Some of the following variables might be nice to load from the config file */
char *system_name= SYSNAME;
char *welcome_html= WELCOME_HTML;
char *wnu_log= WNU_LOG;
int http_uid= HTTP_UID;


char *query;
char *getenv();
#ifdef TESTING
char *get_saved_query();
void save_query(char *qs);
#endif
char hostname[50];

#if !defined(MKP2)
char cpt_pass[MAXSTRINGSIZE];
#else
char clear_pass[MAXSTRINGSIZE];
#endif

char *dummy_env[]= {"PATH=/bin:/usr/bin", NULL};
char **real_env;
extern char **environ;

main(int argc, char **argv)
{
char *datum;
struct rlimit rlim;
#ifdef GLOOP
int fd;
#endif

	/* Don't dump core (could contain part of shadow file) */
	rlim.rlim_cur = rlim.rlim_max = 0;
	(void)setrlimit(RLIMIT_CORE, &rlim);

#ifndef TESTING
	/* Only run for httpd */
	if (getuid() != http_uid)
	{
		err(SYSTEM,"Run by uid %d instead of %d\n",getuid(),http_uid);
	}
#endif

	/* Newuser likes to read its configuration files from its current
	 * directory.  It is not obvious to me in what directory cgi-scripts
	 * execute, or whether there any consistancy to that.  So we provide
	 * the capability to "cd" to the proper place before doing anything
	 * else.
	 */
#ifdef NU_DIR
	if (chdir(NU_DIR))
		err(SYSTEM,"Could not access directory " NU_DIR "\n");
#endif

#ifdef TESTING
	/* When testing, we save a copy of each query in a file.  Then
	 * rerunning the program with a "-r" will run again on that query.
	 * This way if things crash when run from the browser, you can
	 * rerun exactly the same thing under the debugger.
	 */
	if (argc > 1 && !strcmp(argv[1],"-r"))
		query= get_saved_query();
	else
	{
		query= get_query();
		save_query(query);
	}
#else
	query= get_query();
#endif

	if ((datum= getenv("REMOTE_HOST")) == NULL &&
	    (datum= getenv("REMOTE_ADDR")) == NULL)
		mytty= NULL;
	else
		sprintf(mytty= hostname,"[%.47s]", datum);

        /* Get rid of all environment variables */
	real_env= environ;
	environ= dummy_env;

	/* Read in the nu.config file (Marcus code) */
	output_off();
	read_config();
	output_on();

	/* Scrounge user info from the query, making sure it is legal */
	if (wnu_get_user_info(query))
	{
		end_err();
		exit(1);
	}
	log_fwrt= 1;

#ifdef  GLOOP
	fprintf(stderr,"Got valid user %s\n",loginid);
	if ((fd= open("/var/log/gloop",O_WRONLY|O_APPEND,0600)) >= 0)
	{
		char bf[1024];
		sprintf(bf,"%s: <%s>\n",loginid,full_name);
		write(fd,bf,strlen(bf));
		close(fd);
	}
	fprintf(stderr,"Logging done\n",loginid);
#endif  /*GLOOP*/

	/* Create the account (Marcus code) */
	output_off();
	add_user();
	output_on();

	/* Print some sort of "Welcome" page */
	show_page(welcome_html);

	exit(0);
}


/* DIE - Capture die() messages from Marcus's code and pass them to the
 * err() message handling system.
 */

void die(char *mesg)
{
    output_on();
    err(SYSTEM,"%s\n",mesg);
    /* This doesn't return */
}


int dev_null= -1;
int saved_out= -1;
int saved_err= -1;


/* OUTPUT_OFF - Redirect both stdout and stderr to /dev/null.  This kludge 
 * is done so we can use Marcus's code without having to strip out his output
 * statements, which wouldn't mix well into html text.  Output_on() turns it
 * back on.  Redirecting stderr may be unnecessary, but it probably does no
 * harm.
 */

output_off()
{
    if (saved_out != -1) return;
    if (dev_null == -1 && (dev_null= open("/dev/null",2)) < 0)
	err(SYSTEM,"Cannot open /dev/null");
    saved_out= dup(1);
    saved_err= dup(2);
    dup2(dev_null,1);
    dup2(dev_null,2);
}

output_on()
{
    if (saved_out == -1) return;
    dup2(saved_out,1);
    dup2(saved_err,2);
    close(saved_out);
    close(saved_err);
    saved_out= -1;
    saved_err= -1;
}


/* SHOWINFO and PRESSRETURN - This are occasionally run by add_user(), but
 * adduser's output is disabled by WNU, so why bother doing anything here?
 * we substitute dummies.
 */

void showinfo(char *file) {}
void pressreturn(char *fname) {}


/* MY_SLEEP - useradd.c calls this.  For testing on the NeXT, we don't have
 * it, and we don't care.
 */

#ifdef NeXT
my_sleep(int tm) {}
#endif


/* GET_SAVED_QUERY and SAVE_QUERY - This routines are to aid debugging by
 * saving a copy of a query string on one run, and then reusing it on the
 * next run.  They aren't very bullet-proof and aren't meant to be used in
 * production.
 */

#ifdef TESTING
#define QSAVE "/tmp/wnu-qsave"
#define QBSZ 4500

char *get_saved_query()
{
static char qbuf[QBSZ];
int len;
FILE *fp;

	if ((fp= fopen(QSAVE,"r")) == NULL)
		return(NULL);

	len= fread(qbuf,1,QBSZ,fp);
	qbuf[len]= '\0';

	fclose(fp);
	return(qbuf);
}

void save_query(char *qs)
{
FILE *fp;

	if (qs == NULL || (fp= fopen(QSAVE,"w")) == NULL)
		return;
	
	fputs(qs,fp);

	fclose(fp);
}
#endif TESTING
