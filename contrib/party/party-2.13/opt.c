/* PARTY PROGRAM -- OPTION MAINTAINANCE ROUTINES -- Jan Wolter */

#include "party.h"
#include "opt.h"

#define VRSZ	1024	/* More than the total size of all options */
char var_buf[VRSZ];	/* Storage for string values of options */
int var_end= 0;		/* Index of first unused byte of var_buf */

struct optent dflt_copt[NCOPT];

FILE *openchn();


/* The partytab contains any number of lines of the form:
 *
 *   progname default-option-list
 *
 * progname is the name of the program, as in argv[0].  The options are in the
 * usual format.  This lets you set up different versions of party by making a
 * link to the party program with a different name and entering a line setting
 * the options for that name in partytab.
 */

void readtab()
{
    FILE *fp;
    register int pnl;
    char *partytab= PARTYTAB;

    if ((fp= fopen(partytab,"r")) == NULL)
    {
	err("cannot open %s -- using default options\n",partytab);
	return;
    }
    pnl= strlen(progname);

    while (fgets(txbuf,BFSZ,fp) != NULL)
	if (!strncmp(txbuf,progname,pnl))
	{
	    if (txbuf[pnl] != ' ' && txbuf[pnl] != '\t') continue;
	    parseopts(txbuf+pnl+1,1);
	}
    fclose(fp);
}


/* This sets the channel options from the chantab.  The chantab contains
 * any number of lines of the form:
 *
 *    chn_name_pat channel-option-list
 *
 * chn_name_pat is an text pattern that is matched against the channel name.
 * It may include the usual ?, *, \ and [] constructs, but not {}.  The
 * options are in the usual format.  Normally only channel options should be
 * specified here because only those are reset when you leave a channel.  The
 * "file=<partyfile>" option should be specified on just about all channels.
 *
 * The first line with a pattern matching the named channel will be used.  If
 * no line matches, then no options are read and the function returns 1.
 */

int chnopt(char *newchn)
{
    int i;
    FILE *fp;
    char *endpat;

    /* Open the Channel Table */
    if ((fp= openchn()) == NULL)
	    return 1;

    /* Check if the Channel Table includes a line for this name */
    for (;;)
    {
	if (fgets(txbuf,BFSZ,fp) == NULL)
	    return 1;

	*(endpat= firstin(txbuf," \t\n"))= '\0';
	if (patmatch(newchn,txbuf))
	    break;
    }

    /* First, reset defaults values of all channel options */
    for (i= 0; i<NCOPT; i++)
    {
	    if (opt[i].pf & (PF_NO|PF_SEE))
		    opt[i].yes= dflt_copt[i].yes;
	    if (opt[i].pf & PF_STR)
		    setstr(i,dflt_copt[i].str,strlen(dflt_copt[i].str));
    }

    /* Load the options */
    parseopts(endpat+1,1);
    return 0;
}

/*
 * Check if a channel option has it's default value.
 */

int hasdefval(int i)
{
    return (!(opt[i].pf & (PF_NO|PF_SEE)) || opt[i].yes == dflt_copt[i].yes) &&
           (!(opt[i].pf & PF_STR) || !strcmp(opt[i].str,dflt_copt[i].str));
}


/*
 * Set options to the compiled-in defaults.  This must be done to
 * initialize.
 */

void initopts()
{
    int i,len;
    char *s;

    /* Default value of "alias" is realname */
    opt[OPT_ALIAS].str= realname;

    /* If EDITOR env variable is defined, use that as default shell */
    if ((s= getenv("SHELL")) != NULL) opt[OPT_SHELL].str= strdup(s);

    /* Save the defaults for channel options */
    for (i= 0; i<NCOPT; i++)
	dflt_copt[i]= opt[i];	/* Structure Assignment */

    /* Move all defaults into the buffer so they can be modified */
    var_end= 0;
    for (i= 0; i<NOPT; i++)
	if (opt[i].pf & PF_STR)
	{
	    len= strlen(opt[i].str);
	    if (var_end + len + 1 > VRSZ)
	    {
		err("Out of variable space\n");
		return;
	    }
	    strcpy(var_buf+var_end,opt[i].str);
	    opt[i].str= var_buf + var_end;
	    var_end+= len + 1;
	}
}


/*
 * Set the list of options given by the string c.
 *   source == 1 if this comes from partytab or chantab.
 *   source == 2 if this comes from command line.
 *   source == 0 otherwise.
 */

void parseopts(char *c, int source)
{
    char *optbeg,*keybeg,*keyend,*strbeg,*strend,*optend,*p;
    register int i;
    int prefix,quote;

    for (;;)
    {
        /* Skip white leading space and punctuation */
	c= firstout(c," \t,;:");

        /* Is this the end of the line? */
        if (*c == '\0' || *c == '\n') return;

        /* Find end of keyword */
        optbeg= c++;
        optend= c= firstin(c," ,;:\t=\n");
                
	/* Is it a channel selector? */
	if (source == 2 && *optbeg == '#')
	{
	    setstr(OPT_START,optbeg+1,optend-optbeg-1);
	    continue;
	}

        /* Is there a prefix? */
        keybeg= optbeg;
        prefix= 1;             /* Assume no prefix */
        if (keybeg[0] == 'n' && keybeg[1] == 'o')
        {
            prefix= 0;
            keybeg+= 2;
        }
        if (keybeg[0]=='s' && keybeg[1]=='e' && keybeg[2]=='e')
        {
            prefix= (prefix == 1) ? 2 : 1;
            keybeg+= 3;
        }

        /* Is there a string value? */
        if (*c == '=')
        {
	    strbeg= ++c;
	    if (source == 2)
	    {
		strend= c= firstin(c,"\n");
	    }
	    else
	    {
		if (*c == '\042') {
		    /* Delimit string in double-quotes */
		    strbeg= ++c;
		    strend= c= firstin(c,"\n\042");
		} else if (*c == '\047') {
		    /* Delimit string in single-quotes */
		    strbeg= ++c;
		    strend= c= firstin(c,"\n\047");
		} else 
		    /* Delimit unquoted string */
		    strend= c= firstin(c," ,:;\t\n");

		if (*c=='\042' || *c=='\047') c++;
	    }
        }
        else
                strbeg= NULL;
        optend= c;

        /* Look for name in table */
        for (i=0; i < NOPT; i++)
        {
            if (!strncmp(keybeg,opt[i].name,strlen(opt[i].name)))
            {
                /* Am I allowed to change this option? */
                if ((opt[i].pf & (PF_SYS|PF_CHN)) && source != 1)
                {
                    err("%s is not a user settable option\n",opt[i].name);
                    break;
                }
                /* Do string valued options */
                if (strbeg)
                {
                    /* Check that number is assigned to numeric option */
                    if (opt[i].pf & PF_NUM)
                    {
                        for (p=strbeg; p < strend; p++)
                            if (*p < '0' || *p > '9')
                            {
				err("Nonnumeric value assigned to %s\n",
                                    opt[i].name);
                                goto next;
                            }
                    }
                    /* Save the new value */
                    if (opt[i].pf & PF_STR)
                        setstr(i,strbeg,strend-strbeg);
                    else
                    {
                        err("%s does not take a string value\n",opt[i].name);
                        break;
                    }
                    if (i == OPT_FILTER && opt[OPT_FILTER].yes)
                        start_filter();
                    if (i == OPT_MAILDIR)
                        setmailfile();
		    if (i == OPT_WRAP)
			stashname();
                }

                /* Do "See" options */
                if (prefix == 2)
                {
                    if (opt[i].pf & PF_SEE)
                        opt[i].yes= prefix;
                    else
                        err("Invalid option: see%s\n", opt[i].name);
                }
                else /* Do boolean options */
                {
                    if (!(opt[i].pf & PF_NO))
                    {
                        if (prefix == 0)
                        {
			    err("%s does not take a boolean value\n",
                                opt[i].name);
                        }
                    }
                    else
                    {
                        opt[i].yes= prefix;
                        if (i == OPT_FILTER)
                        {
                                if (prefix)
                                        start_filter();
                                else
                                        stop_filter();
                        }
                        if (i == OPT_DEBUG) set_debug();
                    }
                }
                break;
            }
        }
        if (i == NOPT)
            err("Invalid option: %.*s\n",optend-optbeg,optbeg);
next:;
    }
}


void setnum(int nopt,int new)
{
    char anew[12];

    sprintf(anew,"%d",new);	
    setstr(nopt,anew,strlen(anew));
}

/* Set the string valued option number nopt to the value new which has a
 * length of newlen.
 */

void setstr(int nopt, char *new, int newlen)
{
    register char *src, *dst;
    register int i;
    char *stop;
    int oldlen= strlen(opt[nopt].str);
    int inc;

    if (oldlen != newlen)
    {
	if ((inc= newlen - oldlen) > 0)
	{
	    /* String lengthened */
	    if (var_end + inc >= VRSZ)
	    {
		err("Out of variable space\n");
		return;
	    }
	    src= var_buf+var_end;
	    dst= src+inc;
	    stop= opt[nopt].str + oldlen;
	    while (src > stop)
		*(--dst)= *(--src);
	}
	else
	{
	    /* String shortened */
	    src= opt[nopt].str + oldlen;
	    dst= src + inc;
	    stop= var_buf+var_end;
	    while (src < stop)
		*(dst++)= *(src++);
	}
	/* Fix affected pointers */
	var_end+= inc;
	for (i=nopt+1; i<NOPT; i++)
	    if (opt[i].str)
		opt[i].str +=inc;
    }
    /* Put in new value */
    strncpy(opt[nopt].str,new,newlen);
}


/* Print options. If list is NULL, print all PF_SHOW options.  If it is "all"
 * print all options.  If it is "chan" print channel options which differ
 * from their default values.  Otherwise print only the options in the list.
 * printed gives the number of characters already printed on the current line.
 * prefix is a character to start each line with.  Returns 0 if anything is
 * printed, 1 otherwise.
 */

int printopts(FILE *fp, int printed, char prefix, char *list)
{
    register int i,a;
    char *pref;
    int cols= convert(opt[OPT_COLS].str);
    int all= (list && inlist("all",list));
    int chan= (list && !all && inlist("chan",list));
    int didprint=0;

    printed= 0;
    for (i= 0; i<NOPT; i++)
    {
	/* Decide if we should print this */
	if (!all &&
	    (!chan || i >= NCOPT || hasdefval(i)) &&
	    (!list || !inlist(opt[i].name,list)) &&
	    (list || !(opt[i].pf & PF_SHOW)))
	    continue;

	didprint=1;

        if (opt[i].pf & (PF_NO|PF_SEE))
        {
            pref= (opt[i].yes == 1) ? "" : ((opt[i].yes == 0) ? "no" : "see");
            printed+= (a= strlen(opt[i].name) + strlen(pref) + 2);
            if (printed >= cols)
            {
                fputc('\n',fp);
                printed= a;
            }
            fprintf(fp,"%c %s%s",
		(printed==a) ? prefix : ' ', pref, opt[i].name);
        }
        if (opt[i].pf & PF_NUM)
        {
            printed+= (a= strlen(opt[i].name) + strlen(opt[i].str) + 3);
            if (printed >= cols)
            {
                fputc('\n',fp);
                printed= a;
            }
            fprintf(fp,"%c %s=%s",
                (printed==a) ? prefix : ' ', opt[i].name, opt[i].str);
        }
        else if (opt[i].pf & PF_STR)
        {
            printed+= (a= strlen(opt[i].name) + strlen(opt[i].str) + 5);
            if (printed >= cols)
            {
                fputc('\n',fp);
                printed= a;
            }
            fprintf(fp,"%c %s=\042%s\042",
                (printed==a) ? prefix : ' ', opt[i].name, opt[i].str);
        }
    }
    if (printed != 0) fputc('\n',fp);

    return !didprint;
}


int inlist(char *name, char *list)
{
    int nmlen;
    char *p;

    nmlen= strlen(name);
    p= list-1;
    while ((p= strstr(p+1,name)) != NULL)
    {
	    if ((p == list || p[-1] == ' ' || p[-1] == '\t') &&
		strchr(" \t\n",p[nmlen]))
		    return 1;
    }
    return 0;
}


/* Firstin() returns a pointer to the first character in s that is in l.
 * \0 is always considered to be in the string l.
 *
 * Firstout() returns a pointer to the first character s that is not in l.
 * \0 is always considered to be not in the string l.
 *
 * Note that unlike strpbrk() these never return NULL.  They always return
 * a valid pointer into string s, if only a pointer to it's terminating
 * \0.  They are amazingly useful for simple tokenizing.
 */

char *firstin(char *s, char *l)
{
    for (;*s;s++)
	if (strchr(l,*s)) break;
    return s;
}

char *firstout(char *s, char *l)
{
    for (;*s;s++)
	if (!strchr(l,*s)) break;
    return s;
}

/* STRSTR() find a string in a string.  This is the dumb algorithm, meant only
 * for short strings.  It is only here for those Unixes who don't have one in
 * the system library.
 */

#ifndef HAVE_STRSTR
char *strstr(char *s, char *p)
{
    register char *sp, *pp;

    for(sp= s, pp= p; *sp && *pp; )
    {
	if (*sp == *pp)
	{
	    sp++;
	    pp++;
	}
	else
	{
	    sp= sp - (pp - p) + 1;
	    pp= p;
	}
    }
    return *pp ? NULL : sp-(pp-p);
}
#endif /* !HAVE_STRSTR */
 

/* LISTCHN: prints a list of channels.  This will include any channel named
 * uniquely in the chantab, plus any volatile channels with at least one user.
 */

int listchn()
{
    FILE *fp;
    char *o,*n;
    struct chnname *head=NULL, *ch, *chn;
    int ln,clen;

    /* Open the Channel Table */
    if ((fp= openchn()) == NULL)
	return(1);

    /* Put names from the chantab into the list */
    while(fgets(txbuf,BFSZ,fp) != NULL)
    {
	*firstin(txbuf," \t\n")= '\0';

	/* Scan for wild cards, and eliminate backslashes */
	for (o= n= txbuf; *o != '\0'; o++,n++)
	{
	    if (*o == '*' || *o == '?' || *o == '[')
		goto skipit;
	    if (*o == '\\') o++;
	    *n= *o;
	}
	*n= '\0';

	head= addchn(head,txbuf,0);
    skipit: ;
    }

    /* Count up the users and find any volatile channels */
    head= who_clist(head);

    /* Print out the list */
    fputs(" Channel           Users |  Channel           Users |  Channel           Users",
          stderr);
    ln= 0;
    for (ch= head; ch != NULL; ch= ch->next)
    {
        if ((ln++)%3 == 0)
            fputc('\n',stderr);
        else
	    fputs(" | ",stderr);

        fputc(strncmp(ch->name,channel,CHN_LEN) ? ' ' : '>', stderr);

        fputs(ch->name,stderr);
        clen= strlen(ch->name);

#ifndef NOCLOSE
	if (check_open(ch->name) != 1)
        {
	    fputs("(closed)",stderr);
            clen+= 8;
        }
#endif /*NOCLOSE*/

        for ( ; clen < 20; clen++)
		fputc(' ', stderr);

	fprintf(stderr,"%3d",ch->users);
    }
    fputc('\n',stderr);
    if (ln == 0) fputs("No Channels\n",stderr);

    /* Free the list */
    for (ch= head; ch != NULL; ch= chn)
    {
	chn= ch->next;
	free(ch);
    }
}

/* ADDCHN:  Add an entry to the channel list in alphabetical order.
 */

struct chnname *addchn(struct chnname *head, char *name, int users)
{
    struct chnname *c, *p, *new;

    for (c= head, p= NULL; c != NULL; c= (p= c)->next)
	if (strncmp(name,c->name,CHN_LEN) < 0)
	    break;

    new= (struct chnname *)malloc(sizeof(struct chnname));
    strncpy(new->name,name,CHN_LEN);
    new->name[CHN_LEN]= '\0';
    new->users= users;

    new->next= c;
    if (p == NULL)
	head= new;
    else
	p->next= new;

    return head;
}


/* OPENCHN:  Open the chantab file for reading and returns the file descriptor.
 * (actually it only opens it on the first call.  Later it just rewinds.)
 */

FILE *openchn()
{
    static FILE *fp= NULL;

    /* Open the Channel Table */
    if (fp == NULL)
    {
	if ((fp= fopen(opt[OPT_CHANTAB].str,"r")) == NULL)
	{
	    err("cannot open %s\n", opt[OPT_CHANTAB].str);
	    return(NULL);
	}
    }
    else
	rewind(fp);
    return fp;
}


/* This returns true if the string s matches the pattern p.  The pattern syntax
 * is similar to the shell's globbing mechanism, with ? matching any character,
 * [...] matching any character in the brackets, and * matching any sequence
 * of characters.  Backslashes escape any of these special characters.  This
 * version doesn't support {...} patterns, mainly because the kludgy way I
 * wrote it makes those a bit difficult.
 */

int patmatch(char *s, char *p)
{
    int found;
    char *q;

    while (*s != '\0')
    {
	switch (*p)
	{
	case '\0': /* End of pattern but not end of text -- fail */
	    return 0;

	case '?': /* '?' matches any one character */
	    break;

	case '*': /* '*' matches any sequence of characters */
	    if (*(p+1) == '\0') return(1);  /* speeds common case */
	    return patmatch(s,p+1) || patmatch(s+1,p);

	case '[': /* [...] matches any character in the brackets */

	    found= 0;
	    while (*(++p) != ']')
	    {
		if (*p == '\\') p++;
		if (*p == '\0') return 0;
		if (*p == *s) {found= 1; break;}
		if (*(p+1) == '-' && *(q= p+2) != ']')
		{
		    if (*q == '\\') q++;
		    if (*q == '\0') return 0;
		    if (*p < *s && *s <= *q) {found= 1; p= q; break;}
		    p= q;
		}
	    }
	    if (!found) return 0;
	    while (*(++p) != ']')
	    {
		if (*p == '\\') p++;
		if (*p == '\0') return 0;
	    }
	    break;

	case '\\': /* '\x' matches character x */
	    p++;
	default:
	    if (*s != *p) return 0;
	    break;
	}
	p++; s++;
    }
    return (*p == '\0');
}

void set_debug()
{
    time_t tm;

    if (debug) fclose(debug);

    if (opt[OPT_DEBUG].str[0] == '\0')
	opt[OPT_DEBUG].yes= 0;
    
    if (!opt[OPT_DEBUG].yes)
    {
	debug= NULL;
	err("debugging off\n");
	return;
    }

    if ((debug= fopen(opt[OPT_DEBUG].str,"a")) == NULL)
    {
	fprintf(stderr,"cannot open debug file %s\n",
		opt[OPT_DEBUG].str);
	return;
    }

    tm= time(NULL);
    fprintf(debug,"debug started %s",ctime(&tm));
    printopts(debug,0,' ',"all");
    fflush(debug);

    fprintf(stderr,"debugging output in %s\n",opt[OPT_DEBUG].str);
}
