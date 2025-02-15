/* PARTY PROGRAM -- USER INPUT ROUTINES - from keyboard to file - Jan Wolter */

#include "party.h"
#include "opt.h"
#include <ctype.h>
#include <errno.h>
#include <termios.h>

char *cancel= " XXX\n";		  /* Cancel string */
char name[UT_NAMESIZE+2];	  /* User's internal name or alias */
char realname[UT_NAMESIZE+2]= ""; /* User's real name (used for mail check) */
char logname[UT_NAMESIZE+2];	  /* Name user logged in under (utmpx) */
char logtty[UT_LINESIZE+1];	  /* Tty user logged in to (utmpx) */
time_t logtime;			  /* Time user logged in at (utmpx) */
struct termios cooked, cbreak;

struct cc_tab {
	char cmd;	/* command character that brings up prompt */
	char prompt;	/* prompt character brought up */
	void (*func)(char *);	/* procedure to call to interpret the command */
	int enable;	/* option that enables this command */
	char *failmsg; /* message to print if not enabled */
	} cmd_char[]={	/* TABLE OF INPUT MODE COMMAND CHARACTERS */

	{':',  ':',	cmd_colon,	OPT_COLON,	NULL},
	{'-',  '-',	cmd_scrollback,	OPT_NONE,	NULL},
	{'#',  '#',	cmd_join,	OPT_NONE,	NULL},
	{'!',  '!',	cmd_shell,	OPT_SHELL,	NULL},
	{'/',  '/',	cmd_noise,	OPT_MAKENOISE,
					"No noises allowed in this channel\n"},
	/* End of cmd_char table */
	{  0,    0,	NULL,		OPT_NONE,	NULL}};


/* DOCMD: User has typed command character 'ch' in listening mode.  Act on that.
 * Much of the behavior is driven by the table above.
 */
void docmd(int ch)
{
    struct cc_tab *ccmd;

    if (ch == EOF_CHAR) done(0);

    /* Do Table Commands */
    for (ccmd= cmd_char; ccmd->cmd != 0; ccmd++)
       if (ch == ccmd->cmd)
       {
	    /* Check if command is enabled */
	    if (ccmd->enable == OPT_NONE || opt[ccmd->enable].yes)
	    {
		/* Print the prompt */
		STTY(0,&cooked);
		putc(ccmd->prompt,stderr);

		/* Read the command */
		if (fgets(txbuf,BFSZ,stdin))
		    /* Execute the command */
		    (*ccmd->func)(txbuf);
		else 
		{
		    /* Input aborted */
		    fputs(cancel,stderr);
		    clearerr(stdin);
		}
		STTY(0,&cbreak);
	    }
	    else if (ccmd->failmsg != NULL)
		err(ccmd->failmsg);
	    else
		speak(ch);
	    return;
       }

    /* Do non-table commands */
    switch (ch)
    {
    case '=': /* Print options */
	printopts(stderr,0,':',(char *)NULL);
	break;

    case 'q': /* quit -- unless we have firstchar */
	if (opt[OPT_FIRSTCHAR].yes)
	    speak(ch);
	else
	    done(0);
	break;

    case 'h': /* help -- unless we have firstchar */
	if (opt[OPT_FIRSTCHAR].yes)
	{
	   speak(ch);
	   break;
	}

    case '?': /* help */
	if (opt[OPT_HELP].yes)
	    help(opt[OPT_HELP].str,1);
	else
	    speak(ch);
	break;

    default: /* input text */
	speak(ch);
	break;
    }
}


/* SPEAK:  Input a line from the user.  Flag indicates if input mode was
 * invited by typing a space.
 */

void speak(int ch1)
{
    if (ch1 != ' ' && opt[OPT_SPACEONLY].yes)
    {
	fprintf(stderr,"%s\n",opt[OPT_SPACEONLY].str);
	return;
    }
    fputs(opt[OPT_PROMPT].str,stderr);
    if (pgetline(txbuf,BFSZ,ch1) && txbuf[0] != '\0')
    {
	fputs(opt[OPT_TPMORP].str,stderr);
	append(inbuf,wfd,lfd);
    }
    else
    {
	fputs(opt[OPT_TPMORP].str,stderr);
	fputs(cancel,stderr);
	clearerr(stdin);
    }
}


/* CMD_COLON:  Handle a special command (: prefix). */

void cmd_colon(char *command)
{
    if (debug) db("command %s\n",command);

    switch (command_code(&command))
    {

    case CMD_SET:
	if (command)
	    parseopts(command,0);
	else
	    err("missing option on set command\n");
	break;

    case CMD_READ:
	if (command)
	{
	    *firstin(command," \t\n")= '\0';
	    readfile(command);
	}
	else
	    err("read what file?\n");
	break;

    case CMD_SAVE:
	if (command)
	{
	int n= -1;

	    if (*command == '-')
	    {
		if ((n= atoi(command+1)) <= 0)
		{
		    err("inproper offset\n");
		    break;
		}
		command= firstout(firstin(command+1," \t")," \t");
	    }

	    if (command)
	    {
		savelog(n,command);
		break;
	    }
	}
	err("save to what file?\n");
	break;

    case CMD_BACK:
	if (command)
	{
	    if (*command == '-') command++;
	    cmd_scrollback(command);
	}
	else
	    err("missing line count on back command\n");
	break;

    case CMD_WHO:
    case CMD_PWHO:
	if (command == NULL)
	    who_list(stderr,'c');
	else if (command[0] != '-' || (command[1] != 'c' &&
				       command[1] != 't' &&
				       command[1] != 'n'))
	    err("argument to :who must be -n, -t or -c\n");
	else
	    who_list(stderr,command[1]);
	break;

    case CMD_PRINT:
	printopts(stderr,0,':',command);
	break;

    case CMD_JOIN:
	if (command == NULL)
	    err("no channel name given\n");
	else
	{
	    *firstin(command," \t\n")= '\0';

	    if (!strcmp(command,channel))
		err("already on channel %s\n",txbuf);
	    else
		join_party(command);
	}
	break;

    case CMD_HELP:
	help(opt[OPT_HELP].str,1);
	break;

    case CMD_CHANTAB:
	help(opt[OPT_CHANTAB].str,1);
	break;

    case CMD_NOISES:
	if (opt[OPT_MAKENOISE].yes)
	{
	    fprintf(stderr,"legal noises:\n");
	    help(opt[OPT_MAKENOISE].str,1);
	}
	else
	    err("No noises allowed in this channel\n");
	break;

    case CMD_NAME:
	if (!opt[OPT_RENAME].yes)
	    err("rename option not set in this channel\n");
	else if (command == NULL)
	    err("must give new name on :name command\n");
	else
	{
	    *firstin(command," \t\n")= '\0';
	    if (opt[OPT_UNIQUENAME].yes &&
		!who_uniqalias(command, logname, channel) &&
		strcmp(command,logname) &&
		strcmp(command,realname))
		err("the name %s is already in use\n",command);
	    else
		changename(command);
	}
	break;

    case CMD_LIST:
	listchn();
	break;

#ifndef NOCLOSE
    case CMD_CLOSE:
	if (!opt[OPT_MAYCLOSE].yes)
	    err("mayclose option not set in this channel\n");
	else if (is_closed())
	    err("channel is already closed\n");
	else
	    close_chan();
	break;

    case CMD_OPEN:
	if (!is_closed())
	    err("channel is already open\n");
	else
	    open_chan();
	break;

    case CMD_INVITE:
	if (!is_closed())
	    err("this channel is open -- everyone is invited\n");
	else if (command == NULL)
	    err("must give a login id on :invite command\n");
	else
	{
	    char invitee[21];
	    *firstin(command," \t\n")= '\0';
	    strncpy(invitee,command,20);
	    invitee[20]= '\0';
	    invite(invitee);
	}
	break;
#endif /*NOCLOSE*/

#ifndef NOIGNORE
    case CMD_IGNORE:
	if (command == NULL)
	    listignore();
	else
	{
	    char *e, ch;

	    for (;;)
	    {
		e= firstin(command," \t\n");
		ch= *e;
		*e= '\0';
		if (addignore(command))
		    err("you were already ignoring %s\n",command);
		else
		    printf("Ignoring %s\n",command);
		if (ch == '\n' || ch == '\0') break;
		command= firstout(e+1," \t\n");
		if (*command == '\0') break;
	    }
	}
	break;

    case CMD_NOTICE:
	if (ignoring == NULL)
	    err("no users were being ignored\n");
	else if (command == NULL)
	{
	    noignore();
	    printf("Un-ignoring all ignored users.\n");
	}
	else
	{
	    char *e, ch;

	    for (;;)
	    {
		e= firstin(command," \t\n");
		ch= *e;
		*e= '\0';
		if (delignore(command))
		    err("you were not ignoring %s\n",command);
		else
		    printf("Un-ignoring %s\n",command);
		if (ch == '\n' || ch == '\0') break;
		command= firstout(e+1," \t\n");
		if (*command == '\0') break;
	    }
	}
	break;
#endif /*NOIGNORE*/

    case CMD_SHELL:
	if (command == NULL)
	    err("no command given on :shell command\n");
	else
	    cmd_shell(command);
	break;

    case CMD_VERSION:
	fprintf(stderr,"party version %s - (c) 1990, Jan Wolter\n",
		version);
	break;

    case CMD_QUIT:
	done(0);

    default:
	fprintf(stderr,"No such command.  Legal commands are:\n");
	listcmds(stderr);
	break;
    }
}


/* Process a scrollback request */

void cmd_scrollback(char *tback)
{
    int n;

    if (debug) db("scrolling back %s\n",tback);

    if ((n= convert(tback)) < 0)
	err("tailback must be a number\n");
    else
    {
	tailed_from= lseek(rst,0L,1); /* Current pos */
	backup(n);
    }
}


/* Process a change-channel request */

void cmd_join(char *chn)
{
    *firstin(chn,"\n")= '\0';

    if (chn[0] == '\0')
	listchn();
    else if (!strcmp(chn,channel))
    {
	err("already on channel %s\n",chn);
    }
    else
	join_party(chn);
}


/* Process a save -- if n is -1, save entire log.  Otherwise save n lines */

void savelog(int n,char *filename)
{
    FILE *fp;
    long length;
    int m;
#define BS 1024
    char bf[BS];
    char *tilde;

    be_user();
    *firstin(filename,"\n")= '\0';

    /* Do tilde expansion */
    if ((tilde= exptilde(filename)) != NULL)
	filename= tilde;

    if ((access(filename,2) && !access(filename,0)) ||
	(fp= fopen(filename,"a")) == NULL)
    {
	err("Cannot open output file %s\n",filename);
	be_party();
	return;
    }
    be_party();

    fprintf(stderr,"%s to file %s...",
	ftell(fp) == 0L ? "Saving" : "Appending",
	filename);
    fflush(stderr);

    if (tilde != NULL) free(filename);

    length= lseek(rst,0L,1);				/* Current pos */
    length-= (n < 0) ? lseek(rst,0L,0) : backup(n);	/* New pos */

    for (;;)
    {
	m= (length > BS) ? BS : length;
	if ((m= read(rst,bf,m)) < 0) break;
	fwrite(bf, 1, m, fp);
	if (m < BS) break;
	length-= BS;
    }
    fclose(fp);
    fprintf(stderr,"done\n");
}


/* Process a change-channel request */

void cmd_noise(char *ncmd)
{
    *firstin(ncmd,"\n")= '\0';
    if (ncmd[0] == '\0')
    {
	fprintf(stderr,"legal noises:\n");
	help(opt[OPT_MAKENOISE].str,1);
    }
    else
	makenoise(ncmd);
}


/* LIST_CMDS: Print out a list of the legal : commands
 */

void listcmds(FILE *fp)
{
int i,col=0,len;
int cols= convert(opt[OPT_COLS].str);

	for (i= 0; i < NCMD; i++)
	{
		len= strlen(cmd[i].name);
		if (col+len >= cols)
		{
			fputc('\n',fp);
			col= 0;
		}
		col+= len+1;
		fprintf(fp," %s",cmd[i].name);
	}
	fputc('\n',fp);
}


/* COMMAND_CODE:  Returns the command code of the given command string.
 * If it was a valid command, the pointer is advanced to point to the first
 * argument, or is set to NULL if there are none.  Otherwise, if it is not
 * a legal command, -1 is returned and *cp still points to the command word.
 */

int command_code(char **cp)
{
    int cmdlen;
    char *arg1;
    int i;
    int j;

    /* Skip leading spaces */
    *cp= firstout(*cp," \t");

    if (**cp == '!')
    {
	(*cp)++;
	return(CMD_SHELL);
    }

    /* Find end of first word */
    cmdlen= (arg1= firstin(*cp," \t\n"))-*cp;
    if (cmdlen == 0) return(-1);

    /* Find start of second word */

    for (i= 0; i< NCMD; i++)
    {
	for (j= 0; j<cmdlen && (*cp)[j] == cmd[i].name[j]; j++)
	    ;
	if (j==cmdlen && j>=cmd[i].abbr)
	{
	    /* Find first argument */
	    *cp= firstout(arg1," \t");
	    if (**cp == '\0' || **cp == '\n')
		*cp= NULL;
	    return(i);
	}
    }
    return(-1);
}


/* SETREALNAME:  Get the user's realname.  logname should already have been
 * set to the the name he was logged in under (as given in the utmp file).
 * The realname is basically the user who owns the current party process (this
 * is often different from logname if the user has done an "su"), but if there
 * is more than one entry in the passwd file with a uid matching this process,
 * then the login name is prefered.  If there is no passwd file entry matching
 * this process's uid, then the name is his id number.
 */

void setrealname()
{
    struct passwd *pwd;
    char *tname;

    /* if multiple names with same uid's use the one in utmp */
    if ((pwd= getpwnam(logname)) != NULL && pwd->pw_uid == getuid())
	    strcpy(realname,logname);
    else
    {
	if ((pwd= getpwuid(getuid())) != NULL)
	    strcpy(realname, pwd->pw_name);
	else
	    sprintf(realname, "%d", getuid());
    }
}


/* SETNAME: Put the user's name (possibly fake) in the global name variable.
 * This is called every time we change channels.  The name of the channel we
 * are joining is passed as an argument.
 */

void setname(char *chan)
{
    FILE *fp;
    char *ch;
    int i, havename;

    if (opt[OPT_UIDNAME].yes)
	    strcpy(name, opt[OPT_RENAME].yes ? opt[OPT_ALIAS].str : realname);
    else
    {
	/* take name from utmp file */
	if ((ch= getlogin()) != 0)
	    strcpy(name, ch);
	else
	    strcpy(name, "(unknown)");
    }

    havename= 0;
    if (opt[OPT_MAPNAME].yes && opt[OPT_MAPNAME].str[0] != '\0')
    {
	/* change the user's name according to the mapping in the file */
	if ((fp= fopen(opt[OPT_MAPNAME].str,"r")) == NULL)
	    err("cannot open mapname file %s\n",opt[OPT_MAPNAME].str);
	else
	{
	    while (fgets(txbuf,BFSZ,fp) != NULL)
	    {
		*(ch= firstin(txbuf," \t"))= '\0';
		if (!strcmp(realname,txbuf))
		{
		    ch= firstout(ch+1," \t");
		    *firstin(ch," \n\t")= '\0';
		    strncpy(name,ch,UT_NAMESIZE);
		    name[UT_NAMESIZE]= '\0';
		    havename= 1;
		    break;
		}
	    }
	}
    }

    if (!havename)
    {
	if (opt[OPT_RANDNAME].yes && opt[OPT_RANDNAME].str[0] != '\0')
	{
	    /* Pick a name from a random line of the named file */
	    if ((fp= fopen(opt[OPT_RANDNAME].str,"r")) == NULL)
		err("cannot open randname file %s\n",opt[OPT_RANDNAME].str);
	    else
	    {
		/* Initialize random number generator, adding some garbage
		   to the second so two users entering at the same time don't
		   get the same name */
		SEEDRAND((unsigned)(time(NULL) + realname[2] +
			50*realname[1] + 2500*realname[0]));

		i = 1;
		name[UT_NAMESIZE]= '\0';
		while (fgets(txbuf,BFSZ,fp) != NULL)
		{
		    if (txbuf[0] != '#' &&
			(unsigned)RAND() <= (unsigned)MAXRAND / (i++))
		    {
			*firstin(txbuf,": \n\t")= '\0';
			strncpy(name,txbuf,UT_NAMESIZE);
		        name[UT_NAMESIZE]= '\0';
		    }
		}
	    }
	}
	else if (opt[OPT_ASKNAME].yes)
	{
	    /* Prompt user for name -- For halloween parties */
	    for (;;)
	    {
		fprintf(stderr,"What name would you like (8 chars max)? ");
		fgets(txbuf,BFSZ,stdin);
		/* If none given, use real name */
		if (txbuf[0] == '\n')
		{
		    if (opt[OPT_RENAME].yes)
		    {
			/* Use his alias if it is unique */
			strcpy(name, opt[OPT_ALIAS].str);
			checkname(name);
			if (who_uniqalias(name, logname, chan))
			    break;
		    }
		    /* He can always use his real name, unique or not */
		    strcpy(name, realname);
		    break;
		}
		else
		{
		    checkname(txbuf);
		    if (strcmp(txbuf,logname) &&
		        strcmp(txbuf,realname) &&
		        !who_uniqalias(txbuf, logname, chan))
		    	fprintf(stderr,"That name is already in use\n");
		    else
		    {
			strncpy(name,txbuf,UT_NAMESIZE+1);
			break;
		    }
		}
	    }
	}
    }
    checkname(name);
    stashname();
}


/* CHANGENAME: change the user's name to the given string and append a message
 * telling about it to the user's current channel.
 */

void changename(char *newname)
{
    char onmbuf[UT_NAMESIZE+2];

    /* Save the old name */
    strcpy(onmbuf,name);

    /* Set the new name */
    strncpy(name,newname,UT_NAMESIZE+1);
    checkname(name);

    /* Stash the name in the input buffer */
    stashname();

    /* Print the message */
    /* If you change the format of this at all, edit ignore_line() too */
    sprintf(txbuf, "~~~~ %s turns into %s\n",onmbuf,name);
    append(txbuf,wfd,lfd);

    /* Record it in the partytmp file */
    who_chan();
}

/* STASHNAME:  copy the global variable "name" into the input buffer. Should
 * be called everytime name is changed.  Warning, this may redefine the txbuf
 * pointer as a side effect.
 */

void stashname()
{
    int i, namelen, headlen;

    /* Figure out length of header field of line */
    namelen= strlen(name);
    if (namelen < 8)		/* UT_NAMESIZE would be ugly here */
	headlen= 10;
    else
        headlen= namelen+2;
    txbuf= inbuf+headlen;	/* Point txbuf to body part of line */

    /* Stash the name in the input buffer with ':' and spaces after it */
    strcpy(inbuf,name);
    inbuf[namelen]= ':';
    for (i= namelen + 1; i < headlen; i++)
	inbuf[i]= ' ';
}


/* CHECKNAME:  clean up a name's syntax. Currently we don't allow upper case
 * or non-printable characters.  It may only be UT_NAMESIZE characters long.
 * No 0 length names (Apologies to Ryan Antkowiak, who found that bug and
 * wanted to keep it).  No names starting with space, = or - or <, since those
 * are used to recognize a line as a read, event or noise line.
 */

void checkname(char *name)
{
    int i;
    for (i= 0;i < UT_NAMESIZE;i++)
    {
	if (name[i] == '\n' || name[i] == '\0')
	    break;
	else if (!isprint(name[i]))
	    name[i]= '#';
	else if (isupper(name[i]))
	    name[i]= tolower(name[i]);
    }
    /* Fix null names */
    if (i == 0) name[i++]= '_';
    name[i]= '\0';

    /* Fix names starting with cue characters */
    if (name[0] == ' ' || name[0] == '<' || name[0] == '-' || name[0] == '~')
	name[0]= '_';
}


/* cmd_shell:  execute a shell escape */
void cmd_shell(char *buf)
{
    *firstin(buf,"\n")= '\0';
    who_shout(buf);
    usystem(buf);
    fputs("!\n",stderr);
    initmodes();  /* Reread ttymodes - may change */
    who_shin();
}


/* MAKENOISE:  Look up a noise and stick it in the file
 */
int makenoise(char *cmd)
{
    FILE *fp;
    int cmdlen;
    int nargs,cargs=0;
#define MAXNARGS 10
    char *arg[MAXNARGS];
    char *p,*q;
    char nbuf[BFSZ];
    char rbuf[BFSZ];
    int i,j;

    /* Open noise file */
    if ((fp= fopen(opt[OPT_MAKENOISE].str,"r")) == NULL)
    {
	err("cannot open noisetab %s\n",opt[OPT_MAKENOISE].str);
	return(1);
    }

    /* Get command word length */
    *firstin(cmd,"\n")= '\0';
    cmd= firstout(cmd," \t");
    cmdlen= firstin(cmd," \t") - cmd;

    /* Count arguments and save their starting positions */
    nargs= 0;
    p= firstout(cmd + cmdlen," \t");
    while (*p != '\0' && nargs < MAXNARGS)
    {
	arg[nargs++]= p;
	p= firstout(firstin(p," \t")," \t");
    }

    /* Find the first matching line in the file */
    while (fgets(rbuf,BFSZ,fp) != NULL)
    {
	if (!strncmp(cmd,rbuf,cmdlen) &&
		    (rbuf[cmdlen] == ' ' || rbuf[cmdlen] == '\t'))
	{
	    p= firstin(rbuf+cmdlen+1,"0123456789");
	    if ((cargs= atoi(p)) > nargs) continue;
	    p= firstin(p,"<");
	    if (*p == '\0') continue;

	    /* Generate noise from template and put it in nbuf */
	    i= 0;
	    nbuf[i++]= '<';
	    /* Insert tag */
	    for (q= name; *q != '\0'; q++)
		nbuf[i++]= *q;
	    nbuf[i++]= ':';
	    /* Interpret noise definition */
	    for (p++ ;*p != '\0' ;p++)
	    {
		if (*p != '$')
		    nbuf[i++]= *p;
		else
		{
		    j= atoi(++p);
		    p= firstout(p,"0123456789")-1;
		    if (j > cargs) continue;
		    for (q= (j==0)?name:arg[j-1];
			 *q != '\0' && *q != '\n' &&
			 (j == cargs || (*q != ' ' && *q != '\t'));
			 nbuf[i++]= *(q++));
		}
	    }
	    nbuf[i]= '\0';

	    /* Put noise in the party file */
	    append(nbuf,wfd,lfd);

	    fclose(fp);
	    return(0);
	}
    }

    if (cargs > 0)
	err("need %d argument%s\n", cargs,(cargs == 1)?"":"s");
    else
	err("no such noise\n");
    fclose(fp);
    return(0);
}


/* APPEND: Output a string to the end of the party log file */

void append(char *string, FILE *wfd, int lfd)
{
    LOCK(lfd);
    fseek(wfd,0L,2);
    fputs(string,wfd);
    if (strchr(string,'\n') == NULL) fputc('\n',wfd);
    fflush(wfd);
    UNLOCK(lfd);
}


/* INITMODES: Set up the cooked and cbreak modes for future use.  Note that
 * this doesn't actually change the modes.
 */

void initmodes()
{
    /* Get ioctl modes */
    errno= 0;
    if (debug) db("reading stty modes\n");
    GTTY(0,&cooked);
#ifdef DEBUG_STTY
    if (debug) {db("cooked modes read\n");printstty(debug,&cooked);}
#endif /*DEBUG_STTY*/
#ifdef F_STTY
    ioctl(0, TIOCGETC, &tch);
#ifdef TIOCGLTC
    ioctl(0, TIOCGLTC, &ltch);
#endif
#endif /*F_STTY*/
    if (errno != 0)
    {
	err("Input not a tty\n");
	exit(1);
    }
    cbreak= cooked;		/* Structure assignment works? */
#if defined(F_TERMIO) || defined(F_TERMIOS)
    cbreak.c_lflag&= ~(ECHO | ICANON);
    cbreak.c_cc[VMIN]= 1;
    cbreak.c_cc[VTIME]= 0;
#endif /*F_TERMIO or F_TERMIOS*/
#ifdef F_STTY
    cbreak.sg_flags &= ~ECHO;
    cbreak.sg_flags |= CBREAK;
#endif /*F_STTY*/

#ifdef DEBUG_STTY
    if (debug) {db("cbreak modes set up\n");printstty(debug,&cbreak);}
#endif /*DEBUG_STTY*/
}


/* PGETLINE -- read in a line from the user.  If ch1 is not EOF and the
 * firstchar option is set, it will be the first character read.  The
 * interfaces is vaguely fgetslike since it returns the newline in the
 * string.
 */

char *pgetline(char *bf, int n, int ch1)
{
    int col,ch,i;
    char *rc;

    if (isascii(ch1) && isprint(ch1) && opt[OPT_FIRSTCHAR].yes)
    {
	/* Read in cbreak mode, doing as much of our own cooking as possible */
	putchar(ch1);
	bf[0]= ch1;
	col= 1;
	for(;;)
	{
	    ch= getchar();
	    if (ch == '\t') ch= ' ';	/* Tabs are a pain--get rid of them */
	    if (ch == '\n')
	    {
		bf[col++]= ch;
		bf[col]='\0';
		putchar(ch);
		return(bf);
	    }
	    else if (ch == EOF_CHAR)
	    {
		if (col == 0) return(NULL);
		putchar('\007');
	    }
	    else if (ch == ERASE_CHAR)
	    {
		if (col == 0) continue;
		col--;
		putchar('\b'); putchar(' '); putchar('\b');
	    }
	    else if (ch == KILL_CHAR)
	    {
		while (col > 0)
		{
		    col--;
		    putchar('\b'); putchar(' '); putchar('\b');
		}
	    }
	    else if (ch == WERASE_CHAR)
	    {
		while (col > 0 && bf[col-1] == ' ')
		{
		    putchar('\b');
		    col--;
		}
		if (col == 0) continue;
		col--;
		putchar('\b'); putchar(' '); putchar('\b');
		while (col > 0 && bf[col-1] != ' ')
		{
		    col--;
		    putchar('\b'); putchar(' '); putchar('\b');
		}
	    }
	    else if (ch == REPRINT_CHAR)
	    {
		putchar('\n');
		for (i= 0; i < col; i++)
		    putchar(bf[i]);
	    }
	    else if (isascii(ch) && isprint(ch) && col < n-2)
	    {
		bf[col++]= ch;
		putchar(ch);
	    }
	    else
		putchar('\007');
	}
    }
    else
    {
	/* Read in cooked mode ... the good old fashioned party style */
	STTY(0,&cooked);
	rc= fgets(txbuf,BFSZ,stdin);
	STTY(0,&cbreak);
	return(rc);
    }
}
