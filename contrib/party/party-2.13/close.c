/* PARTY PROGRAM -- ROUTINES TO HANDLE CLOSED CHANNELS -- Jan Wolter */

#ifndef NOCLOSE

#include "party.h"
#include "opt.h"
#include <errno.h>
#include <setjmp.h>

extern jump_buf jenv;
extern int inhelp;

/* USR_FILE_NAME: Return the name of the user file name associated with the
 * given channel name.  This must be free()ed by someone else eventually.
 * This will normally contain a list of login id's, listed one per line, of
 * people who are permitted to join the channel.
 */

char *usr_file_name(char *chn)
{
    char *usr_file;

    usr_file= (char *)malloc(strlen(opt[OPT_DIR].str)+CHN_LEN+6);
    sprintf(usr_file,"%s/%s.usr", opt[OPT_DIR].str, chn);
    return(usr_file);
}


/* CHECK_OPEN:  This checks to see if we can enter the named channel.  Return
 * codes are:
 *   0 - channel is closed and we aren't allowed in. 
 *   1 - channel is open.
 *   2 - channel is closed and we are allowed in. 
 */

int check_open(char *chn)
{
    FILE *fp;
    char *fn;
    char buf[BFSZ+1];

    /* Open the user list -- if it doesn't exist, anyone can enter */
    fp= fopen(fn= usr_file_name(chn),"r");
    free(fn);
    if (fp == NULL) return(1);

    /* See if I am in the user list */
    while (fgets(buf,BFSZ,fp) != NULL)
    {
	*firstin(buf," \t\n")= '\0';
	if (!strcmp(buf,realname))
	{
	    fclose(fp);
	    return(2);
	}
    }
    fclose(fp);
    return(0);
}


/* IS_CLOSED:  Is the channel the user is in closed? */

int is_closed()
{
    char *fn;
    int rc;

    if (channel == NULL) return(0);
    rc= access(fn= usr_file_name(channel), 0);
    free(fn);
    return(!rc);
}

/* OPEN_CHAN:  Open up the current channel */

void open_chan()
{
    char *fn;
    time_t now= time(NULL);

    if (channel == NULL) return;
    unlink(fn= usr_file_name(channel));
    free(fn);

    /* Closable channels are now left depermitted even when open */
    /*fchmod(rst,CHN_MODE);*/

    sprintf(txbuf, "---- %s opened this channel (%.12s)\n",name,ctime(&now)+4);
    append(txbuf,wfd);
}


/* CLOSE_CHAN:  Close the current channel.  Create the .usr file and write in
 * a list of the people already in the channel. */

void close_chan()
{
    char *fn;
    int fd;
    FILE *fp;
    time_t now= time(NULL);

    if (channel == NULL) return;
    fd= open(fn= usr_file_name(channel),O_WRONLY|O_CREAT|O_EXCL,USR_MODE);
    free(fn);
    if (fd < 0)
    {
	if (errno == EEXIST)
		printf("channel is already closed\n");
	else
		printf("error - cannot create user file\n");
	return;
    }

    fp= fdopen(fd,"w");
    who_ison(fp,channel);
    fclose(fp);

    /* Closable channels are now left depermitted even when open */
    /*fchmod(rst,DEP_MODE);*/

    sprintf(txbuf, "---- %s closed this channel (%.12s)\n",name,ctime(&now)+4);
    append(txbuf,wfd);
}


/* INVITE:  invite a user into the the channel.  Well, really just permit them
 * to enter.
 */

void invite(char *login)
{
    char *fn;
    FILE *fp;
    time_t now= time(NULL);

    if (channel == NULL) return;
    fp= fopen(fn= usr_file_name(channel),"a");
    free(fn);
    if (fp == NULL)
    {
	printf("error - cannot append to user file\n");
	return;
    }

    fprintf(fp,"%s\n",login);

    fclose(fp);

    sprintf(txbuf, "---- %s invited %s to this channel (%.12s)\n",name,
	    login,ctime(&now)+4);
    append(txbuf,wfd);
}


/* READYN -- in cooked mode, read in a line.  It it starts with Y, return true.
 */

int read_yn()
{
    int ch;

    if ((ch= getchar()) != '\n')
	while (getchar() != '\n')
		;
    return(ch == 'y' || ch == 'Y');
}

	
/* KNOCK -- stuff a knock-knock message into the named channel.
 */

int knock(char *chn)
{
char bf[BFSZ], *fn;
FILE *tmp_wfd;

    sprintf(bf,"---- %s knocking on the door\n",logname);
    /* No time on this message so norepeat can be used against knock bombers */
    if (access(fn= chn_file_name(chn,0), 0))
	fn= chn_file_name(chn,1);
    if ((tmp_wfd= fopen(fn,"a")) == NULL)
	return(1);
    append(bf,tmp_wfd);
    fclose(tmp_wfd);
    return(0);
}


/* ENTER_CLOSED -- Go through the business of passing the doorway of a
 * channel.  If it is closed (a .usr file exists), check if we are in it.
 * If not, go through the knocking routine.  Return codes are:
 *
 *   0 - channel is closed and we aren't allowed in. 
 *   1 - channel is open.
 *   2 - channel is closed and we are allowed in. 
 */

int enter_closed(char *newchannel)
{
    int was_open;
    int i= 0;

    /* If he already has access, let him in */
    if (was_open= check_open(newchannel)) return(was_open);

    /* If the user doesn't want to knock, keep him out */
    err("channel %s is a closed channel\n",newchannel);
    fprintf(stderr,"Would you like to knock on the door? ");
    if (!read_yn()) return(0);

    /* Do the knock */
    if (!setjump(jenv,1))
    {
	inhelp= 1;
	fprintf(stderr,"Waiting...");
	fflush(stderr);
	knock(newchannel);
	while (!check_open(newchannel))
	{
	    if (i++ >= convert(opt[OPT_KNOCKWAIT].str))
	    {
		fprintf(stderr,"there seems to be no answer\n");
		/*unknock(newchannel);*/
		return(0);
	    }
	    sleep(1);
	}
	inhelp= 0;
	return(2);
    }
    else
    {
	inhelp= 0;
	fprintf(stderr,"wait interrupted\n");
	return(0);
    }
}
#endif /* NOCLOSE */
