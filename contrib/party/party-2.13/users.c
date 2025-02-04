/* PARTY PROGRAM -- USER LIST ROUTINES -- Jan Wolter */

#include "party.h"
#include "opt.h"

#include <sys/stat.h>
#include <pwd.h>

#define CMD_LEN 20
#define SUB_PARTY 8


/* PWHO FILE: this is a file basically similar to the utmp file, that keeps
 * track of who is on what party channel.  It is a binary file with entries
 * for each tty line.  To prevent strange contention problems, no party
 * process should ever write any line of the pwho file except the one
 * corresponding to it's own tty.
 *
 * Each pwho entry consists of one header, giving the ttyname and the
 * logname of the user on that line.  This is followed by up to eight
 * subparty lines for party invocations that have not exitted.  (Multiple
 * invocations can occur when users shell out or suspend their process.)
 * Each of these give that invocation's internal name, the time the process
 * started, the current channel, and any current shell command.
 *
 * One problem with this is that a hostile user can do a "kill -9" on his
 * party process, and leave without having his pwho entry erased, thus 
 * fooling the pwho program into thinking he is still there.  We try to
 * detect such chicanery by comparing the utmp file to the pwho file.  If
 * the start time of the process preceeds the login time of the user on that
 * line, we can safely assume that entry is a dud.
 *
 * We indicated users who have suspended with a "^" in the shelled line.
 */

struct pwho {			   /* HEADER: null name means no user on line */
	char line[UT_LINESIZE];	   /* user's tty name, as in utmp file */
	char logname[UT_NAMESIZE]; /* user's name, as in utmp file */
	};

struct sub_pwho {		 /* SUBFIELD: null alias invalidates entry */
	char alias[UT_NAMESIZE]; /* user's name inside party */
	long time;		 /* time the user entered party */
	char channel[CHN_LEN];	 /* user's current channel number */
	char shelled[CMD_LEN];	 /* name of command user shelled to */
	};
#define ENTRY_SIZE (sizeof(struct pwho) + SUB_PARTY * sizeof(struct sub_pwho))

/* ULIST STRUCTURE: The following structure used to store linked lists of users
 * to be output.  It includes only the info to be displayed.
 */

struct ulist {
	char alias[UT_NAMESIZE];
	time_t time;
	char channel[CHN_LEN];
	char shelled[CMD_LEN];
	struct ulist *next;
	};


void add_ulist(struct ulist **, char *, char *ch, time_t, char *, char);

void wscan_init(void);
int wscan_next(struct pwho *, struct sub_pwho *, int *);
int wscan_done(void);

FILE *pwfp= NULL;
struct pwho myhead;
struct sub_pwho mybody;
long myoff, mysuboff;

/* WHO_ENTER: Called when the user enters party.  If finds a slot for him in
 * the partywho file and writes in his current status.
 */

int who_enter()
{
    long off;
    int i;
    struct pwho ahead;
    struct sub_pwho abody;

    /* Setup our entry */
    strncpy(myhead.line,logtty+5,UT_LINESIZE);
    strncpy(myhead.logname,logname,UT_NAMESIZE);

    mybody.time= time(0L);
    if (name[0] != '\0') strncpy(mybody.alias,name,UT_NAMESIZE);
    if (channel) strncpy(mybody.channel,channel,CHN_LEN);
    strncpy(mybody.shelled,"",CMD_LEN);

    /* Scan for our entry */
    for (off= 0L;;off += ENTRY_SIZE)
    {
	fseek(pwfp,off,0);

	if (fread(&ahead,sizeof(struct pwho),1,pwfp) == 0)
	{
	    /* Create slot for us at end of file */
	    myoff = off;
	    mysuboff = off + sizeof(struct pwho);
	    break;
	}

	if (!strncmp(myhead.line,ahead.line,UT_LINESIZE))
	{
	    /* Found existing slot for us */
	    myoff = off;

	    if (strncmp(myhead.logname,ahead.logname,UT_NAMESIZE) ||
		fread(&abody,sizeof(struct sub_pwho),1,pwfp) == 0 ||
		abody.alias[0] == '\0'/* ||
		abody.time < logtime*/)
	    {
		/* No valid user in it -- take first subslot */
		who_clear(off,myhead.line);
		mysuboff = off + sizeof(struct pwho);
	    }
	    else
	    {
		off += sizeof(struct pwho);
		/* We're already there -- use first free slot */
		for (i= 1; i<SUB_PARTY; i++)
		{
		    if (fread(&abody,sizeof(struct sub_pwho),1,pwfp) == 0 ||
			abody.alias[0] == '\0')
			    break;
		}
		if (i == SUB_PARTY)
		    mysuboff= 0L;	/* No subrecords left */
		else
		    mysuboff= off+sizeof(struct sub_pwho)*i;
	    }
	    break;
	}
    }

    /* Write my Entries in their proper place */
    if (mysuboff != 0L)
    {
	fseek(pwfp,myoff,0);
	fwrite(&myhead,sizeof(struct pwho),1,pwfp);
	fseek(pwfp,mysuboff,0);
	fwrite(&mybody,sizeof(struct sub_pwho),1,pwfp);
	fflush(pwfp);
    }
    return 0;
}


/* WHO_OPEN:  Open the pwho file.
 */

int who_open()
{
    if ((pwfp= fopen(opt[OPT_WHOFILE].str,"r+"))==NULL)
    {
	err("cannot open whofile: %s\n", opt[OPT_WHOFILE].str);
	return 1;
    }
    return 0;
}


/* WHO_EXIT:  called when we exit the program.  Erases our subentry, and
 * maybe the header too if this was the last one.
 */

void who_exit()
{
    int i;
    struct sub_pwho abody;

    if (!pwfp || !mysuboff) return;

    /* Erase our subentry */
    strncpy(mybody.alias,"",UT_NAMESIZE);
    fseek(pwfp,mysuboff,0);
    fwrite(&mybody,sizeof(struct sub_pwho),1,pwfp);
    fflush(pwfp);

    /* Scan subentries.  If any non-null ones are left, we are done */
    fseek(pwfp,myoff+sizeof(struct pwho),0);
    for (i= 0; i<SUB_PARTY; i++)
    {
	if (fread(&abody,sizeof(struct sub_pwho),1,pwfp) == 0)
	    break;
	if (abody.alias[0] == '\0')
	    return;
    }

    /* We erased last subentry.  Erase header too. */
    strncpy(myhead.logname,"",UT_NAMESIZE);
    fseek(pwfp,myoff,0);
    fwrite(&myhead,sizeof(struct pwho),1,pwfp);
    fflush(pwfp);
}


/* WHO_CHAN:  Called when a channel is changed.  Channel changes may involve
 * a name change as well.
 */

void who_chan()
{
    strncpy(mybody.channel,channel,CHN_LEN);
    strncpy(mybody.alias,name,UT_NAMESIZE);

    if (!pwfp || !mysuboff) return;

    fseek(pwfp,mysuboff,0);
    fwrite(&mybody,sizeof(struct sub_pwho),1,pwfp);
    fflush(pwfp);
}


/* WHO_SHOUT:  Record that the user has shelled out.
 */

void who_shout(char *cmd)
{
    char *ptr;
    int len;

    if ((ptr= strchr(txbuf,' ')) == NULL)
	len= CMD_LEN;
    else if ((len= ptr - txbuf) > CMD_LEN)
	len= CMD_LEN;
    ncstrncpy(mybody.shelled,cmd,len);

    if (!pwfp || !mysuboff) return;

    fseek(pwfp,mysuboff,0);
    fwrite(&mybody,sizeof(struct sub_pwho),1,pwfp);
    fflush(pwfp);
}

/* WHO_SHIN: Record that the user has returned from a shell escape.
 */

void who_shin()
{
    strncpy(mybody.shelled,"",CMD_LEN);

    if (!pwfp || !mysuboff) return;

    fseek(pwfp,mysuboff,0);
    fwrite(&mybody,sizeof(struct sub_pwho),1,pwfp);
    fflush(pwfp);
}

/* WHO_ISOUT: Check if the users is currently shelled out */
int who_isout()
{
    return (mybody.shelled[0] != '\0');
}


/* WHO_LIST:  Print a list of who is on.  The sortkey may be
 *    'n' - sort by user name
 *    'c' - sort by conference
 *    't' - sort by time
 */

void who_list(FILE *fp, char sortkey)
{
    struct pwho ahead;
    struct sub_pwho abody;
    int i,n;
    struct ulist *head= NULL, *curr, *next;

    wscan_init();

    /* Scan file and load into ulist */
    while (wscan_next(&ahead,&abody,&n))
	add_ulist(&head, abody.alias, abody.channel, abody.time, abody.shelled,
		  sortkey);
    wscan_done();

    /* Print Headings */
    fprintf(fp,"User");
    for (i= 4; i < UT_NAMESIZE; i++) putc(' ',fp);
    fprintf(fp," Started          Channel\n");

    /* Print ulist */
    for (curr= head; curr != NULL; curr= curr->next)
    {
	fprintf(fp,"%-*.*s %15.15s  %.*s",
		UT_NAMESIZE,UT_NAMESIZE,curr->alias,
		ctime(&curr->time)+4,
		CHN_LEN,curr->channel);
	if (curr->shelled[0] == '\0')
	       fprintf(fp,"\n");
#ifdef SIGTSTP
	else if (curr->shelled[0] == '^' &&
		 curr->shelled[1] == '\0')
	       fprintf(fp," (suspended)\n");
#endif /*SIGTSTP*/
	else
	       fprintf(fp," (shelled to %s)\n",curr->shelled);
    }

    /* Deallocate ulist */
    for (curr= head, next= NULL; curr != NULL; curr= next)
    {
	next= curr->next;
	free(curr);
    }
}

/* ADD_ULIST:  Add a user entry to the ulist, inserting it in the list in the
 * proper place as selected by sortkey.
 */

void add_ulist(struct ulist **head, char *al, char *ch, time_t tm, char *sh,
	char sortkey)
{
    struct ulist *new= (struct ulist *)malloc(sizeof(struct ulist));
    struct ulist *curr, *prev;
    int c_ch, c_al, c_tm;

    new->time= tm;
    strncpy(new->alias,al,UT_NAMESIZE);
    strncpy(new->channel,ch,CHN_LEN);
    ncstrncpy(new->shelled,sh,CMD_LEN);

    for (curr= *head, prev= NULL; curr != NULL; prev= curr, curr=curr->next)
    {
	c_ch= strncmp(ch,curr->channel,CHN_LEN);
	c_al= strncmp(al,curr->alias,UT_NAMESIZE);
	c_tm= (tm < curr->time) ? -1 : 1;

	if ((sortkey == 'c' &&
	     (c_ch<0 || (c_ch==0 && (c_al < 0 || (c_al == 0 && c_tm < 0))))) ||
	    (sortkey == 'n' &&
	     (c_al<0 || (c_al==0 && c_tm < 0))) ||
	    (sortkey == 't' && c_tm < 0))
	    break;
    }

    /* Insert the new element before curr */
    if (prev == NULL)
	*head= new;
    else
	prev->next= new;
    new->next= curr;
}


#ifndef NOCLOSE

/* WHO_ISON:  Print a list of who is on the named channel.  This just prints
 * the real names of the users.  It is mainly used for initializing .usr
 * files for closed channels.
 */

void who_ison(FILE *fp, char *chn)
{
    struct pwho ahead;
    struct sub_pwho abody;
    int n;

    wscan_init();

    while (wscan_next(&ahead,&abody,&n))
    {
	if (!strncmp(chn,abody.channel,CHN_LEN))
	    fprintf(fp,"%-.*s\n",UT_NAMESIZE,ahead.logname);
    }
    wscan_done();
}
#endif /*NOCLOSE*/


/* WHO_CLEAR:  Erase an entry in the pwho file */

void who_clear(off_t off, char *line)
{
    struct pwho ahead;
    struct sub_pwho abody;
    int i;

    if (!pwfp) return;
    fseek(pwfp,off,0);
    strncpy(ahead.line,line,UT_LINESIZE);
    strncpy(ahead.logname,"",UT_NAMESIZE);
    fwrite(&ahead,sizeof(struct pwho),1,pwfp);

    strncpy(abody.alias,"",UT_NAMESIZE);
    for(i=0;i<SUB_PARTY;i++)
	fwrite(&abody,sizeof(struct sub_pwho),1,pwfp);
    fflush(pwfp);
}


/* WHO_EMPTY:  return true if the given channel is empty.  Actually, it returns
 * true if there is one person left, because we call it just before we leave.
 */

int who_empty(char *channel)
{
    struct pwho ahead;
    struct sub_pwho abody;
    int count=0;
    int n;

    wscan_init();

    while (wscan_next(&ahead,&abody,&n))
    {
	if (!strncmp(abody.channel,channel,CHN_LEN) && count++)
	{
	    wscan_done();
	    return 0;
	}
    }
    wscan_done();
    return 1;
}


/* WHO_COUNT:  count the total number of users currently running party.  */

int who_count()
{
    struct pwho ahead;
    struct sub_pwho abody;
    int count=0;
    int n;

    wscan_init();
    while (wscan_next(&ahead,&abody,&n))
	if (n==1) count++;
    wscan_done();
    return count;
}


/* WHO_CLIST:  Build up a linked list of active channels, with user counts,
 * and return it.  If old is non-null, it should point to the first element
 * of a list of channels already known.
 */

struct chnname *who_clist(struct chnname *old)
{
    struct pwho ahead;
    struct sub_pwho abody;
    struct chnname *head=old,*ch;
    int n;

    wscan_init();

    while (wscan_next(&ahead,&abody,&n))
    {
	/* Check if we have seen this channel before */
	for (ch= head; ch != NULL; ch= ch->next)
	{
	    if (!strncmp(ch->name,abody.channel,CHN_LEN))
		break;
	}

	if (ch)
	    ch->users++;
	else
	    head= addchn(head,abody.channel,1);
    }
    wscan_done();
    return head;
}


/* WHO_UNIQALIAS:  Is the given alias used only by the given user in the
 * given channel?
 */

int who_uniqalias(char *alias, char *name, char *channel)
{
    struct pwho ahead;
    struct sub_pwho abody;
    int n, skipping= 0;

    wscan_init();
    while (wscan_next(&ahead,&abody,&n))
    {
    	/* If this belongs to me, skip the entire entry */
    	if (n == 1)
	    skipping= !strncmp(name, ahead.logname, UT_NAMESIZE);
	if (skipping)
	    continue;

    	/* Ignore users in other channels */
    	if (strncmp(channel, abody.channel, CHN_LEN))
    	    continue;

	if (!strncmp(alias, abody.alias, UT_NAMESIZE))
	{
	    wscan_done();
	    return 0;
	}
    }
    wscan_done();
    return 1;
}


/*********  P A R T Y T M P   S C A N N I N G   R O U T I N E S  *********/


int subcnt;	          /* Number of subfields read so far */
long curtime;	          /* Time stamp from utmp file */

/* WSCAN_INIT -- This initializes for a new scan of the partytmp file. */

void wscan_init()
{
    if (!pwfp) return;
    fseek(pwfp,0L,0);
    subcnt= 0;
}

/* WSCAN_NEXT -- Get the next valid head/body pair from the party file.  If
 * there isn't one, return 0.  'n' gives the subfield number of the current
 * entry, 1 for the first subfield, and so on.  wscan_init() must be called
 * shortly before the first call to this.  wscan_done() should be called after
 * it is complete.
 */

int wscan_next(struct pwho *phead, struct sub_pwho *pbody, int *n)
{
    struct passwd *pw;
    struct stat s;
    char login[UT_NAMESIZE + 1];
    char line[UT_LINESIZE + 1 + 5];
    int j;
    int ignore;

    if (!pwfp) return 0;

    for (;;)
    {
        ignore= 0;
	if (subcnt == 0)
	{
	    /* Get the next header */
	    if (fread(phead,sizeof(struct pwho),1,pwfp) == 0)
		return 0;

            strlcpy(line, "/dev/", sizeof(line));
            strlcat(line, phead->line, sizeof(line));
            strlcpy(login, phead->logname, sizeof(login));
	    /* If the user does not exist or does not own the line
	     * in the partytmp file, then this is an obsolete
	     * entry.  We will ignore it.
	     */
            ignore= 1;
            pw= getpwnam(login);
            if (pw != NULL)
            {
                if (stat(line, &s) == 0 && s.st_uid == pw->pw_uid)
                    ignore= 0;
	        curtime= s.st_ctime;
            }
	}

	/* Read in sub-records */
	for (subcnt++; subcnt<=SUB_PARTY; subcnt++)
	{
	    if (fread(pbody,sizeof(struct sub_pwho),1,pwfp) == 0)
		return 0;
	    else if (!ignore &&
		     pbody->alias[0] != '\0'/* &&
		     pbody->time >= curtime*/)
	    {
		*n= subcnt;
		return 1;
	    }
	}
	subcnt= 0;
    }
}


/* WSCAN_DONE -- Finish up a scan of the partytmp file */
int wscan_done()
{
    return 0;
}


/* NCSTRNCPY -- Do a string copy, replacing non-printable characters with ? */
void ncstrncpy(char *s1, char *s2, int n)
{
    for (;*s2 != '\0' && n > 0; s1++,s2++,n--)
    {
	if (isascii(*s2) && isprint(*s2))
	    *s1= *s2;
	else
	    *s1= '?';
    }

    for (;n > 0; s1++,n--)
	*s1= '\0';
}


/* FINDUSER - Figure out the user's control tty and /etc/utmp name in a fairly
 * robust manner.  This is generally tougher to fool than getlogin().  This
 * returns TRUE if the the user cannot be found for any reason.
 */

int finduser(char *logtty, char *logname, time_t *logtime)
{
    struct passwd *pw;
    const char *tty;
    int i;
    struct stat s;

    /* Find our login name */
    pw= getpwuid(getuid());
    if (pw == NULL)
    {
        err("Who are you?\n");
        return 1;
    }
    strlcpy(logname, pw->pw_name, sizeof(logname));

    /* See if we can find a useful ttyname for stderr, stdout, or stdin.
     * default to "(none)". */
    strlcpy(logtty, "(none)", sizeof(logtty));
    for (i= 2; i >= 0; i--)
    {
        if ((tty= ttyname(i)) != NULL &&
            strcmp(tty,"/dev/tty") != 0)
        {
            strlcpy(logtty, tty, UT_LINESIZE + 1);
            break;
        }
    }

    /* Figuring out our login time is a bit more complex.
     * Approximate this by looking at the inode ctime on
     * our tty.  Default to the current time.
     */
    time(logtime);
    if (stat(logtty,&s) == 0)
    {
        *logtime= s.st_ctime;
    }

    return 0;
}
