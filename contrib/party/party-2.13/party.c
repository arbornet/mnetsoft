/*      Party - (c) July 1990 - Jan Wolter
 *
 *	The party program simulates a party line, where any number of users
 *	can talk at the same time.  It was originally developed under System
 *	III Unix, and as such communicates through a log file rather than
 *	using interprocess communications.  It can only be used between
 *	different computers if they can share the log file via some sort
 *	of NFS-like mechanism.  It should however work under most any version
 *	of Unix.
 *	
 *      This program may be freely distributed without permission of the
 *	author.  No permission is needed to run this software anywhere.
 *	However selling the program itself would, of course, be inappropriate.
 *
 *						Dr. Jan Wolter
 *						janc@unixpapa.com
 * 
 *  RELEASE HISTORY (version numbers before 2.4 were assigned retroactively)
 *
 *  version 1.0:   Original two-process party program by Marcus Watts.
 *  version 1.8:   Jan Wolter's bug fixes, changes in output format, added
 *                 partytab
 *  version 2.0:   Single process rewrite by Jan Wolter.
 *  version 2.1:   Added user settable options.
 *  version 2.2:   Added multiple channels, external filter control.
 *  version 2.3:   Major internal clean-up, reorganized output filters, special
 *	           commands added, noises added.
 *  version 2.4:   Added "who" command, BSD compatibility.  The Grex release.
 *  version 2.5:   Added volatile channels.  :name command.
 *  version 2.6:   Minor bug fixes, System V and SCO portability patches.
 *  version 2.7:   Closable channels, BSDI bug fixes, other minor bug fixes.
 *  version 2.8:   Login names more than 8 characters for next BSDI version;
 *                 norepeat option added; sorted :who listings; showevent.
 *  version 2.9:   Idle time outs, capacity limits, intro commands, no control
 *                 characters in shelled command name
 *  version 2.10:  Add "raw" option and :ignore and :notice commands.
 *  version 2.10a: SIGTERM makes a clean exit.  Fixed Marcus's SIGPIPE bug.
 *                 :list in three columns.  Added "uniquename".  Better ttyname
 *                 checking.  Use MAIL environment variable.  Fixed /dev/tty
 *                 cloaking bug.  :ignore tracks name changes.
 *  version 2.10b: fix finduser's overflow of logname variable.
 *  version 2.10d: fix control characters in user names via PARTYOPTS.
 *  version 2.11:  use select() if NOSELECT is not defined.
 *  version 2.11a: turn NOSELECT off - it doesn't work and never will!
 *  version 2.12:  Fixed signals for Linux.  ANSIfication.  AUTOCONFication.
 *  version 2.13:  Default shell from environment.  Tilde expansion on :read
 *                 and :save.
 */

#include "party.h"
#include "opt.h"
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <setjmp.h>
#include <pwd.h>

char *version= "2.13";		/* current party version */

int inhelp=0;			/* Are we printing help text? */
off_t tailed_from=0L;		/* File offset we last tailed back from */
FILE *wfd;			/* Party file open for write */
int lfd;			/* Party file lock */
int rst;			/* Party file open for read */
char *channel= NULL;		/* Current channel name (NULL is outside)*/
char *progname;			/* Program name */
char mailfile[80];		/* Name of mail file */
int  mailfuse;			/* counter for deciding when to check mail */
long mailsize;			/* old size of mail file */
uid_t real_uid, eff_uid;	/* My real and effective group and user ids */
gid_t real_gid, eff_gid;
char inbuf[BFSZ+INDENT+2];      /* Text buffer - first 10 for "name:   " */
char *txbuf= inbuf + INDENT;    /* Text buffer - pointer to respose portion */
				/*               of inbuf */
FILE *debug= NULL;		/* Debug file, if one is open */
RETSIGTYPE (*oldsigpipe)();	/* Old SIGPIPE handler */

jump_buf jenv;

int
main(int argc, char **argv)
{
    int n;
    char ch, *pnl;
    time_t lastaction;
    fd_set input_set, iset;
    struct timeval tv;

    progname= leafname(argv[0]);	/* Get the program name */

    /* Look self up in utmp */
    if (finduser(logtty,logname,&logtime)) exit(1);

    setrealname();		/* Get the user name */
    initopts();			/* Set default options */
    readtab();			/* Read options from partytab */
    if (who_open()) exit(1);	/* Open the partytmp file */

    /* Check if we are just doing a pwho command */
    if (opt[OPT_USERLIST].yes)
    {
	who_list(stdout,'c');
	exit(0);
    }

    /* Check if there is room for one more in party - race conditions
     * could allow the capacity to be exceeded if two people enter at
     * once, but who cares?
     */
    if (opt[OPT_CAPACITY].yes &&
	who_count() >= atoi(opt[OPT_CAPACITY].str))
    {
	printexec(stderr,opt[OPT_FULLMESG].str);
	exit(1);
    }

    /* If possible, set OPT_COLS from TERMCAP at this point */
    setcols();

    /* Read options from PARTYOPTS env var */
    if (opt[OPT_ENV].yes && (pnl= getenv("PARTYOPTS")))
	parseopts(pnl,0);

    /* Read options from parameters */
    if (opt[OPT_ARG].yes)
	for (n= 1; n < argc; n++)
	    parseopts(argv[n],2);

    real_uid= getuid(); eff_uid= geteuid();
    real_gid= getgid(); eff_gid= getegid();

    initmodes();
    setmailfile();
    initoutput();
    if (who_enter()) done(1);

    printexec(stderr,opt[OPT_INTRO].str);

    /* Put up the signal handlers */
    signal(SIGHUP,(RETSIGTYPE (*)())hup);
    signal(SIGQUIT,(RETSIGTYPE (*)())intr);
    signal(SIGINT,(RETSIGTYPE (*)())intr);
    signal(SIGTERM,(RETSIGTYPE (*)())term);
    oldsigpipe= signal(SIGPIPE,SIG_IGN);
    signal(SIGTSTP,(RETSIGTYPE (*)())susp);
    signal(SIGWINCH,(RETSIGTYPE (*)())setcols);

    if (join_party(opt[OPT_START].str)) exit(1);

    /* Get in cbreak/noecho mode */
    STTY(0,&cbreak);

    /* Set up the input file set for the select call */
    FD_ZERO(&input_set);
    FD_SET(0,&input_set);

    /* Main loop */
    lastaction= time((time_t *)0);
    for(;;)
    {
	memset(&tv, 0, sizeof(tv));
 	tv.tv_usec = 100000;  // 10 Hz.
	/* Copy text while I can -- otherwise go to input mode */
	if (output())
	{
	    /* Wait up to 4 seconds for input in file or from user */
	    iset= input_set;
	    select(rst+1, &iset, NULL, NULL, &tv);

	    /* process typed command from user */
	    if (FD_ISSET(0,&iset))
	    {
	        read(0,&ch,1);
	        docmd(ch);
	        lastaction= time((time_t *)0);
	    }

	    /* Check for new mail or idleness */
	    if (mailfuse-- == 0)
	    {
		struct stat mailstat;

		/* Check if he has been idle a long time */
		if (opt[OPT_IDLEOUT].yes &&
		    time((time_t *)0) - lastaction >
		       atoi(opt[OPT_IDLEOUT].str)*60)
		{
		    fprintf(stderr,
		      "You have been idle in party for more than %s minutes\nBye!\n",
		      opt[OPT_IDLEOUT].str);
		    done(0);
		}

		/* Check if size of mailfile has increased */
		mailfuse= MAILDELAY;
		if (stat(mailfile,&mailstat))
		    mailsize= 0L;
		else
		{
		    if (mailstat.st_size > mailsize)
			fprintf(stderr,"You have more mail\n");
		    mailsize= mailstat.st_size;
		}
	    }
	}
    }
}


/* HELP: Prints a file name to the screen.  An interrupt brings you back to
 * party.  If complain is false, it silently does nothing when the file
 * doesn't exist.
 */

void help(char *filename, int complain)
{
    register int hf;
    register int n;

    if ((hf= open(filename,O_RDONLY)) < 0)
    {
	if (complain) err("Cannot open file %s\n",filename);
    }
    else
    {
	if (!setjump(jenv,1))
	{
	    inhelp= 1;
	    while((n= read(hf,txbuf,BFSZ)) > 0)
		write(1,txbuf,n);
	}
	close(hf);
	inhelp= 0;
    }
}

/* EXPTILDE:  given a file name that starts with a ~, return a copy in malloc'ed
 * memory that has the the ~ expanded.  If there is no ~ or the ~ cannot be
 * expanded return NULL.
 */

char *exptilde(char *filename)
{
    char *dir= NULL;
    char *slash;
    struct passwd *pw;

    /* Need a ~ and a slash to get started */
    if (filename[0] != '~' || (slash= strchr(filename+1,'/')) == NULL)
	return NULL;

    /* Figure out the home directory */
    *slash= '\0';
    if (slash == filename + 1)
    {
	if ((dir= getenv("HOME")) == NULL)
	{
	    if ((pw= getpwuid(getuid())) != NULL)
		dir= pw->pw_dir;
	}
    }
    else if ((pw= getpwnam(filename+1)) != NULL)
	dir= pw->pw_dir;
    *slash= '/';

    if (dir == NULL)
	return NULL;

    /* Construct the path */
    filename= (char *)malloc(strlen(dir) + 1 + strlen(slash+1));
    sprintf(filename,"%s/%s",dir,slash+1);

    return filename;
}


/* READFILE: This reads a file into the partylog, much as if you had typed it
 * in, except that each line is prepended with a space instead of your logname.
 * Files must be readable *both* to party and to the user. 
 */

void readfile(char *filename)
{
    FILE *fp;
    int readlim= convert(opt[OPT_READLIM].str);
    int lines= 0;
    char *tilde;

    if (readlim == 0)
    {
	err("File reading not enabled in this channel\n");
	return;
    }

    /* Do tilde expansion */
    if ((tilde= exptilde(filename)) != NULL)
	filename= tilde;

    /* Can only open files if user and party can read it */
    be_user();
    if (access(filename,4) || (fp= fopen(filename,"r")) == NULL)
    {
	err("Cannot open input file %s\n",filename);
	be_party();
	return;
    }
    if (tilde != NULL) free(filename);
    be_party();

    /* Print user's name */
    txbuf[0]= '\n';
    txbuf[1]= '\0';
    LOCK(lfd);
    fseek(wfd,0L,2);
    fputs(inbuf,wfd);
    fflush(wfd);
    
    txbuf[0]= ' ';
    while ((lines++ < readlim)  && fgets(txbuf+1,BFSZ-1,fp))
    {
	fseek(wfd,0L,2);
	fputs(txbuf,wfd);
	if (strchr(txbuf,'\n') == NULL) fputc('\n',wfd);
	fflush(wfd);
    }
    UNLOCK(lfd);
    fclose(fp);
}


/* CHN_FILE_NAME:  return the channel file name for the current channel.
 * Somebody needs to free this.
 */

char *chn_file_name(char *chn, int keeplog)
{
    char *file;
    size_t len;

    len= strlen(opt[OPT_DIR].str)+strlen(chn)+5+1;
    file= (char *)malloc(len);
    if (file == NULL)
    {
	err("malloc failed\n");
	exit(1);
    }
    snprintf(file,len,"%s/%s.%s",
	opt[OPT_DIR].str,
	chn,
	keeplog?"log":"tmp");
    return(file);
}

/* CHN_LOCKFILE_NAME:  return the channel lock file name for the named
 * channel.  This is malloc'ed; the caller must free.
 *
 * Note that the lock file is separate from the channel file and is
 * permitted only to the party user.  This separation prevents abusive
 * users from doing antisocial things, like opening the channel file
 * and taking a lock on it, preventing other users from joining, while
 * still allowing us to keep (most) channel files world-readable, so
 * that interested users can peek at them without joining.
 */
char *chn_lockfile_name(char *chn, int keeplog)
{
    char *lockfile;
    size_t len;

    len= strlen(opt[OPT_DIR].str)+strlen(chn)+1+strlen(".lock")+1+strlen("log")+2+1;
    lockfile= (char *)malloc(len);
    if (lockfile == NULL)
    {
	err("malloc failed");
	exit(1);
    }
    snprintf(lockfile,len,"%s/.lock/%s.%s",
        opt[OPT_DIR].str,
	chn,
	keeplog?"log":"tmp");
    return(lockfile);
}


/* JOIN_PARTY: changes from the current channel to the new channel.  Either one
 * of those could be NULL.  Returns 1 if the named channel doesn't exist.
 */

int join_party(char *nch)
{
    char *file;
    char *lockfile;
    FILE *tmp_wfd;
    int tmp_lfd,tmp_rst,oumask,waskept;
    time_t now= time((time_t *)0);
    char newchannel[CHN_LEN+1];
    static char ch[CHN_LEN+1];

    if (debug) db("join_party %s\n",nch?nch:"(nil)");

    waskept= opt[OPT_KEEPLOG].yes;

    /* If we are to enter a new channel, make sure we can */
    if (nch != NULL)
    {
	/* Make a local copy of name */
	strncpy(newchannel,nch,CHN_LEN);
	newchannel[CHN_LEN]= '\0';

	if (badname(newchannel))
	{
	    err("improper channel name\n");
	    return(1);
	}

#ifndef NOCLOSE
	if (!enter_closed(newchannel))
	    return(1);
#endif /*NOCLOSE*/

	/* Set the channel options */
	if (chnopt(newchannel))
	{
	    err("channel %s does not exist\n",newchannel);
	    return(1);
	}
	if (debug) printopts(debug,0,' ',"all");

	/* Construct the channel log file name */
	file= chn_file_name(newchannel,opt[OPT_KEEPLOG].yes);

	/* Open the partyfile to read - if it doesn't exist create it */
	if ((!opt[OPT_MAYCLOSE].yes && access(file,4)) ||
	    (tmp_rst= open(file,O_RDONLY)) < 0)
	{
	    /* I don't have read access, or the file doesn't exist */
	    oumask= umask(000);
	    if (!access(file,0) ||
	       (tmp_rst= open(file,O_RDONLY|O_CREAT,
		    opt[OPT_MAYCLOSE].yes ? DEP_MODE : CHN_MODE)) < 0)
	    {
		err("channel %s not accessible\n(%s unreadable)\n",
		    newchannel,file);
		free(file);
		umask(oumask);
		if (channel != NULL) chnopt(channel);
		return(1);
	    }
	    umask(oumask);
	}

	/* Open partyfile to write */
	if ((tmp_wfd= fopen(file,"a")) == NULL)
	{
	    err("Cannot write partyfile %s\n",file);
	    close(tmp_rst);
	    close(tmp_lfd);
	    free(file);
	    if (channel != NULL) chnopt(channel);
	    return(1);
	}
	free(file);

	lockfile= chn_lockfile_name(newchannel,opt[OPT_KEEPLOG].yes);
	tmp_lfd= open(lockfile,O_RDONLY|O_CREAT, 0600);
	if (tmp_lfd < 0)
	{
	    err("channel %s not accessible\n(lock %s unreadable)\n",
	        newchannel,lockfile);
	    close(tmp_rst);
	    free(lockfile);
	    umask(oumask);
	    if (channel != NULL) chnopt(channel);
	    return(1);
	}
	free(lockfile);

	if (debug) db("join_party: new channel open\n");
    }

    /* If we were in some channel, get out */
    if (channel != NULL)
    {
	    if (nch == NULL)
		    sprintf(txbuf,"---- %s leaving (%.12s)\n",
			    name,ctime(&now)+4);
	    else
		    sprintf(txbuf,
			    "---- %s switching to channel %s (%.12s)\n",
			    name,newchannel,ctime(&now)+4);
	    write(out_fd,txbuf,strlen(txbuf));

	    /* Put departure message in the file */
	    append(txbuf,wfd,lfd);

	    /* Close the old files */
	    fclose(wfd);
	    close(lfd);
	    close(rst);

	    /* If it was a temporary channel and it is empty, delete it */
	    if (!waskept && who_empty(channel))
	    {
		file= chn_file_name(channel,0);
		unlink(file);
		free(file);
#ifndef NOCLOSE
		file= usr_file_name(channel);
		unlink(file);
		free(file);
#endif /*NOCLOSE*/
	    }
	if (debug) db("join_party: old channel closed\n");
    }

    /* If we are to enter a new channel, finish the job */
    if (nch != NULL)
    {
	/* Make the new file the current file */
	rst= tmp_rst;
	lfd= tmp_lfd;
	wfd= tmp_wfd;
	tailed_from= 0L;

	/* Set streams to close on exec */
	fcntl(rst,F_SETFD,1);
	fcntl(fileno(wfd),F_SETFD,1);

	/* Print Channel Banner */
	if (channel != NULL)
	{
	    if (opt[OPT_CHANINTRO].yes)
		printexec(stderr,opt[OPT_CHANINTRO].str);
	    else
		fprintf(stderr,
		"\n================== channel %s ==================\n",
		newchannel);
	    fprintf(stderr,"Options:");
	    if (printopts(stderr,8,' ',"chan"))
		fprintf(stderr,"normal\n\n");
	    else
		fputc('\n',stderr);
	}

	/* Get the user's name or pseudonym */
	setname(newchannel);

	/* Put in a join message */
	LOCK(lfd);
	fseek(wfd,0L,2);
	if (channel == NULL)
	    fprintf(wfd,"---- %s joining (%.12s)\n",
		    name,ctime(&now)+4);
	else
	    fprintf(wfd,"---- %s joining from channel %s (%.12s)\n",
		    name,channel,ctime(&now)+4);
	fflush(wfd);
	UNLOCK(lfd);

	/* Wind back a few lines */
	lseek(rst,0L,2);			/* Goto end of file */
	backup(convert(opt[OPT_BACK].str));

	strcpy(channel=ch,newchannel);

	who_chan();
    }
    else
	who_exit();

    return(0);
}


/* SETMAILFILE:  Start monitering the user's mail directory so we can notify
 * him when he has new mail.
 */

void setmailfile()
{
    struct stat mailstat;
    char *env;

    if ((env= getenv("MAIL")) != NULL)
	strcpy(mailfile,env);
    else
    {
	strcpy(mailfile,opt[OPT_MAILDIR].str);
	strcat(mailfile,"/");
	strcat(mailfile,realname);
    }
    if (stat(mailfile,&mailstat))
	    mailsize= 0L;
    else
	    mailsize= mailstat.st_size;
    mailfuse= MAILDELAY;
}

/* LEAFNAME: Return the trailing name from a pathname.
 */

char *leafname(char *c)
{
    register char *t;

    if ((t= strrchr(c,'/')) == NULL)
	return(c);
    else
	return(t+1);
}


/* BADNAME:  Check a channel name to make sure it contains only legal
 * characters: namely printable, non-space, non-dot, non-slash characters.  We
 * could probably stand dots, but we don't.
 */

int badname(char *chn)
{
    char *badchr= " ./#";

    for(;*chn;chn++)
	if (!isascii(*chn) || !isprint(*chn) || strchr(badchr,*chn))
	    return(1);
    return(0);
}

/* CONVERT:  Convert a string to a non-negative integer.  This does a bit more
 * checking than atoi().  It returns -1 if the string is not syntactially
 * correct.
 */

int convert(char *c)
{
    register int n=0;

    /* Skip leading spaces and tabs */
    while (*c == ' ' || *c == '\t')
	c++;

    for ( ; *c != '\n' && *c != '\0'; c++)
    {
	if (*c >= '0' && *c <= '9')
	    n= n*10 + *c - '0';
	else
	    return (-1);
    }
    return(n);
}

/* Helper function to do fcntl locks.
 */

void setlock(int fd, int type)
{
    struct flock lk;

    lk.l_type= type;
    lk.l_whence= 0;
    lk.l_start= 0L;
    lk.l_len= 0L;
    fcntl(fd,F_SETLKW,&lk);
}


/* BACKUP:  This seeks backwards by the given number of lines.
 */

off_t backup(int lines)
{
#define BUB_SIZE 512
    char bub[BUB_SIZE];
    register char *p;
    off_t off;
    int n;

    if (lines <= 0) return 0;

    p= bub;
    off= lseek(rst,0L,1);

    /* Scan backwards, counting newlines */
    do
	if (p-- == bub)
	{
	    if (off == 0L) break;
	    /* Fetch a new block of data */
	    off -= (n= (off > BUB_SIZE) ? BUB_SIZE : off);
	    lseek(rst, off, 0);
	    read(rst, bub, n);
	    p= bub + n - 1;
	}
    while ( *p != '\n' || lines-- > 0);

    /* Position file pointer */
    return(lseek(rst, off+(p-bub)+1, 0));
}


/* INTR: Handle user interrupts.  If he is tailing back or printing a help
 * file, just interrupt that.
 */

RETSIGTYPE intr()
{
    if (inhelp)
    {
	/* Old Unixes lost interupt handlers after interupts.  Modern
	 * Unixes don't need this but we do it anyway, */
	signal(SIGQUIT,(RETSIGTYPE (*)())intr);
	signal(SIGINT,(RETSIGTYPE (*)())intr);
	longjump(jenv,1);
    }
    if (tailed_from > lseek(rst,0L,1))
    {
	signal(SIGQUIT,(RETSIGTYPE (*)())intr);
	signal(SIGINT,(RETSIGTYPE (*)())intr);
	printf("\nTailback Interrupted...\n");
	lseek(rst,tailed_from,0);
	return;
    }

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    join_party(NULL);

    STTY(0,&cooked);
    kill_filter();
    exit(0);
}


/* TERM:  Exit the program because we got a SIGTERM. This is like a hup(), but
 * prints a message to the user. */

RETSIGTYPE term()
{
    if (debug) db("killed\n");

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    join_party(NULL);

    kill_filter();
    STTY(0,&cooked);

    printf("\nParty Process Killed.\n");

    exit(0);
}


/* DONE:  Exit the program at the user's command.  This should be used just
 * about everywhere you would normally call exit.
 */

void done(int rc)
{
    if (debug) db("exiting\n");

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    join_party(NULL);
    stop_filter();
    STTY(0,&cooked);
    exit(rc);
}

/* HUP:  Handle Hangups.  The only difference betwen this and done() is that
 * we are more brutal about killing off the filter.
 */

RETSIGTYPE hup()
{
    if (debug) db("hangup\n");

    signal(SIGINT,SIG_IGN);
    signal(SIGQUIT,SIG_IGN);

    join_party(NULL);

    kill_filter();
    STTY(0,&cooked);
    exit(0);
}

/* SUSP: Suspend execution temporarily.
 */

RETSIGTYPE susp()
{
    struct termios old;
    int mask;
    int was_shelled;

    if (debug) db("suspended\n");

    if (!(was_shelled= who_isout())) who_shout("^");
    mask= sigblock(sigmask(SIGTSTP));

    GTTY(0,&old);
    STTY(0,&cooked);

    signal(SIGTSTP,SIG_DFL);
    sigsetmask(0);
    kill(getpid(),SIGTSTP);

    /* STOP HERE */

    sigsetmask(mask);
    signal(SIGTSTP,(void (*)())susp);

    initmodes();
    STTY(0,&old);
    if (!was_shelled) who_shin();

    if (debug) db("restarted\n");
}

void db(char *msg, ...)
{
    va_list ap;
    time_t now;

    if (debug)
    {
        va_start(ap, msg);
	now= time((time_t *)0);
        fprintf(debug,"%8.8s:  ",ctime(&now)+11);
        vfprintf(debug,msg,ap);
	fflush(debug);
	va_end(ap);
    }
}


void err(char *msg, ...)
{
    va_list ap;
    time_t now;

    va_start(ap, msg);
    fprintf(stderr,"%s error: ",progname);
    vfprintf(stderr,msg,ap);

    if (debug)
    {

	now= time((time_t *)0);
	fprintf(debug,"%8.8s:  ",ctime(&now)+11);
	vfprintf(debug,msg,ap);
	fflush(debug);
    }
    va_end(ap);
}
