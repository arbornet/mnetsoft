#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include "wnu.h"

/* VERSION HISTORY:
 * 12/14/95 - Last version before I started collecting version history. [jdw]
 * 06/24/97 - Added version history. [jdw]
 */

int reject= 0;		/* Is this a bad account application? */

extern char *rmdir_on_error;
extern char *query;
extern int log_fwrt;

extern char **environ, **real_env, *dummy_env[];

/* ERR  - An error has occurred.  Set the "reject" flag to indicate that
 * we aren't going to accept the application.  If this is the first error
 * message, write out the error page header.  Otherwise just continue the
 * error page with the new message.  It takes arguments like printf(), 
 * with an additional first argument that should be either USER or SYSTEM.
 * If the error is a system error, the program terminates and this routine
 * does not return.  For user errors, it does return.
 */

void err(int type, char *format, ...)
{
    va_list ap;

    va_start(ap,format);

    if (!reject)
    {
	/* Print various header stuff before the first error message */

	print_cgi_header(type & SYSTEM ? "Error in Account Creation" :
		"Error in Newuser Application");
	printf("Your application for an account on %s " 
	       "has not been processed due to\n",system_name);
	if (type & SYSTEM)
	    printf("a system problem.\n");
	else
	    printf("an error in your application.\n");
	printf("<UL>\n");
    }

    /* Print the specific error message */
    printf("<P><LI>\n");
    vcgiprintf(format,ap);

    reject|= type;

    if (type & SYSTEM)
    {
	/* Clean up and Terminate */
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	un_lock();
	if (rmdir_on_error != NULL && rmdir(rmdir_on_error) < 0)
		err(0,"Cannot remove directory %S after failure.\n",
			rmdir_on_error);
	log_error(SYSTEM,format,ap);
	end_err();
	exit(1);
    }
    else if (type & HOSTILE)
	log_error(USER,format,ap);

    va_end(ap);
}


/* END_ERR - Finish an error screen output */

void end_err()
{
    printf("</UL>\n");
    if (reject & SYSTEM)
	printf("Sorry.  This system error has been logged and will be "
		"addressed by our staff.\nPlease, try again later.\n");
    else
    {
	printf("Please go back to the application form, correct it, "
	       "and submit it again.\n");
	if (reject & HOSTILE)
		printf("A record of this error has been "
			"logged for our staff.\n");
     }

    print_cgi_trailer();
}


/* LOG_ERROR - Save a error message to our log file.  In the future this should
 * also send the full text in mail to root, and put a short message on the
 * console.
 */

log_error(int by, char *fmt, va_list ap)
{
FILE *fp;
char *var, *val, *cp, *ctime(), *getenv();
time_t now= time(0L);
int i;

    if ((fp= fopen(wnu_log,"a")) == NULL)
	    err(0,"Cannot open %s\n",wnu_log);
    fprintf(fp,"===== %s Error at %24.24s =====\n",
	    (by == USER) ? "Hostile User" : "System",
	    ctime(&now));
    vcgifprintf(fp,fmt,ap);

    /* We suppress this, because it contains the user's password 
    fprintf(fp,"Original QUERY=");
    i= 15;
    for (cp= query; *cp != '\0'; cp++,i++)
    {
	if (i == 70)
	    { fputc('\n',fp); i= 0; }
	fputc(*cp,fp);
    }
    fputc('\n',fp);
    */

    environ= real_env;
    fprintf(fp,"REMOTE_HOST=%s\n",(cp= getenv("REMOTE_HOST")) ? cp:"");
    fprintf(fp,"REMOTE_ADDR=%s\n",(cp= getenv("REMOTE_ADDR")) ? cp:"");
    fprintf(fp,"REMOTE_USER=%s\n",(cp= getenv("REMOTE_USER")) ? cp:"");
    fprintf(fp,"REQUEST_METHOD=%s\n",(cp= getenv("REQUEST_METHOD")) ? cp:"");
    fprintf(fp,"HTTP_USER_AGENT=%s\n",(cp= getenv("HTTP_USER_AGENT")) ? cp:"");
    environ= dummy_env;

    if (log_fwrt)
	fwrtusr(fp,0);
    else if (var != NULL)
    {
	firstvar(&var,&val);
	do {
	    fprintf(fp,"%s=",var);
	    if (!strncmp(var,"passwd",6))
		fprintf(fp,"(suppressed)\n");
	    else
	    {
		for (cp= val; *cp != '\0'; cp++)
		{
		    putc(*cp,fp);
		    if (*cp == '\n' && cp[1] != '\0')
			fputs("\t",fp);
		}
		if (cp[-1] != '\n')
		    putc('\n',fp);
	    }
	} while (nextvar(&var,&val));
    }

    fclose(fp);
}
