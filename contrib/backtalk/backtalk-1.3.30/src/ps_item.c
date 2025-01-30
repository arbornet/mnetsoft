/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* ROUTINES TO READ ITEM FILES
 *
 * Mar 15, 1993 - Jan Wolter:  Original bbsread version
 * Jun  4, 1993 - Jan Wolter:  Skip scribbled text
 * Dec  2, 1995 - Jan Wolter:  Ansification
 * Jan 10, 1996 - Jan Wolter:  Backtalk version.
 */

#include "backtalk.h"

#include <errno.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <ctype.h>

#include "sysdict.h"
#include "format.h"
#include "dict.h"
#include "str.h"
#include "ps_item.h"
#include "ps_part.h"
#include "ps_sum.h"
#include "lock.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int curr_item= -1;	/* Which item we currently have open (-1 if none) */
int curr_excl;          /* Do we have an exclusive lock on the current file? */
FILE *item_fp;		/* File descriptor for current item file */
int reading_resp;	/* number of the response we are reading */
int read_head;		/* Have we read the header for this response yet? */
char item_path[BFSZ];	/* Full pathname of item file */
int resp_type;		/* Type of current resp:
			 * 0=plain, 1=html, 2=plain-as-html */


/* OPENITEM - Make sure the item named by the "item" system variable in the
 * directory named by the "confdir" system variable is open and ready to
 * read from.  If the excl flag is true, and exclusive lock is set and the
 * file is opened read/write, otherwise a shared lock is set and the item
 * is only opened to read.  If exec is 2 and "editfrozen" is enabled, then
 * frozen items will be briefly thawed while opening them.  Returns non-zero
 * if the item does not exist or cannot be opened.
 */

int openitem(int excl)
{

    if (curr_item != -1 && idict(VAR_ITEM) == curr_item &&
        ((!excl) == (!curr_excl)))
	return 0;

    /* Close some other item */
    if (curr_item != -1)
    {
	fclose(item_fp);
	unlock(item_path);
    }

    curr_item= idict(VAR_ITEM);
    curr_excl= excl;
    sprintf(item_path,"%s/_%d",sdict(VAR_CONFDIR),curr_item);
    if ((item_fp= fopen(item_path,excl?"r+":"r")) == NULL)
    {
	/* If the open fails because the file doesn't exist there is a tiny
	 * chance we hit it in the middle of a retitle operation.  Wait a bit
	 * and try again. */
	if (errno == ENOENT)
	{
	    sleep(2);
	    item_fp= fopen(item_path,excl?"r+":"r");
	}

	/* If we lack permission to open the item, it may be a frozen item.
	 * If "editfrozen" is enabled, try briefly thawing it. */
	if (item_fp == NULL && errno == EACCES && excl > 1 &&
	    bdict(VAR_EDITFROZEN))
	{
	    struct stat st;
	    if (!stat(item_path,&st) &&
	        !chmod(item_path, st.st_mode|0200))
	    {
	    	item_fp= fopen(item_path,"r+");
		chmod(item_path, st.st_mode);
	    }
	}

	if (item_fp == NULL)
	{
	    /* All our tricks failed */
	    curr_item= -1;
	    return 1;
	}
    }
    lock(excl,fileno(item_fp),item_path);
    reading_resp= 0;
    read_head= FALSE;

    /* Also open the index file */
    open_item_index(sdict(VAR_CONFDIR), curr_item);

    return 0;
}


/* CLOSEITEM - Make sure no item is open.
 */

void closeitem()
{
    if (curr_item != -1)
    {
	fclose(item_fp);
	unlock(item_path);
	curr_item= -1;
    }

    /* Also close the index file */
    close_item_index();
}


/* FIXIMODES - Fix any incorrectly set item modes, both in the system
 * dictionary and in the sum file.
 */

void fiximodes()
{
    struct stat st;

    if (fstat(fileno(item_fp),&st))
	die("Unable to stat item");
    
    /* Fix linked status */
    if (st.st_nlink > 1)
    {
	if (!bdict(VAR_LINKED))
	{
	    put_sum(idict(VAR_ITEM),-1,SF_LINKED,0,0L,0L);
	    seti(VAR_LINKED,1);
	}
    }
    else
    {
	if (bdict(VAR_LINKED))
	{
	    put_sum(idict(VAR_ITEM),-1,0,SF_LINKED,0L,0L);
	    seti(VAR_LINKED,0);
	}
    }

    /* Fix frozen status */
    if (!(st.st_mode & 0200))
    {
	if (!bdict(VAR_FROZEN))
	{
	    put_sum(idict(VAR_ITEM),-1,SF_FROZEN,0,0L,0L);
	    seti(VAR_FROZEN,1);
	}
    }
    else
    {
	if (bdict(VAR_FROZEN))
	{
	    put_sum(idict(VAR_ITEM),-1,0,SF_FROZEN,0L,0L);
	    seti(VAR_FROZEN,0);
	}
    }

    /* Fix retired status */
    if (st.st_mode & 0100)
    {
	if (!bdict(VAR_RETIRED))
	{
	    put_sum(idict(VAR_ITEM),-1,SF_RETIRED,0,0L,0L);
	    seti(VAR_RETIRED,1);
	}
    }
    else
    {
	if (bdict(VAR_RETIRED))
	{
	    put_sum(idict(VAR_ITEM),-1,0,SF_RETIRED,0L,0L);
	    seti(VAR_RETIRED,0);
	}
    }
}


/* RESP_TYPE_NAME: Given a style number, generate a response type name
 */

char *resp_type_name(int rt)
{
    switch (rt)
    {
    case 0:	/* style 0 is plain text, unless it's a Yapp posting */
	return (idict(VAR_YAPP_FORMAT)&2) ? "text/html" : "text/plain";

    case 1: /* style 1 is standard HTML */
	return "text/html";
	break;

    case 2: /* style 2 is Yapp compatible plain-text-as-HTML */
    default: /* Anything else - treat as plain */
	return "text/plain";
    }
}


/* ITEMHEAD - Read in the header for an item, saving the various values in
 * various sysdict variables.  Returns non-zero if the item does not exist.
 */

int itemhead()
{
    char buf[BFSZ];
    char *a, *attach= NULL;

    /* Open the item, if we haven't already */
    if (openitem(0)) return 1;

    fiximodes();

    /* Position file pointer to read the header */
    if (reading_resp != 0 || read_head)
    {
	rewind(item_fp);
	reading_resp= 0;
	read_head= FALSE;
    }

    /* Read and check magic code number */
    if (fgets(buf,BFSZ,item_fp) == NULL || strcmp(buf,"!<ps02>\n"))
	die("Bad magic number in item file %s",item_path);

    while (fgets(buf,BFSZ,item_fp) != NULL)
    {
	if (buf[0] != ',') continue;
	switch(buf[1])
	{
	case 'H':
	    *firstin(buf+2,"\n")= '\0';
	    sets(VAR_TITLE,buf+2,SM_COPY);
	    break;
	case 'A':
	    *firstin(buf+2,"\n")= '\0';
	    sets(VAR_AUTHORNAME,bdict(VAR_MAYSEEALIAS)?buf+2:"",SM_COPY);
	    break;
	case 'U':
	    seti(VAR_AUTHORUID,atoi(buf+2));
	    if ((a= strchr(buf+2,',')) == NULL)
		die("Bad ,U syntax in item file %s",item_path);
	    *firstin(a+1,"\n")= '\0';
	    sets(VAR_AUTHORID,a+1,SM_COPY);
	    break;
	case 'R':
	    /* Handle flags later maybe */
	    /* ,R0001 censored */
	    seti(VAR_HIDDEN,buf[5] == '1');
	    /* ,R0003 scribbled */
	    seti(VAR_ERASED,buf[5] == '3');
	    resp_type= buf[4] - '0';
	    sets(VAR_TEXTTYPE, resp_type_name(resp_type), SM_COPY);
	    break;
	case 'D':
	    sett(VAR_DATE,atoth(buf+2));
	    if ((a= strchr(buf+2,' ')) != NULL)
		sett(VAR_EDITDATE,atoth(a+1));
	    else
		sett(VAR_EDITDATE,0);
#ifdef HIDE_HTML
	    if (resp_type == 0)
#endif
		break;
	    /* may drop through */
	case 'T':
	    read_head= TRUE;
#if ATTACHMENTS
	    if (attach == NULL)
		sets(VAR_ATTACHMENTS,"",SM_COPY);
	    else
		sets(VAR_ATTACHMENTS,attach,SM_FREE);
#endif
	    return 0;
	case 'P':
            /* Ignore this -- probably a Yapp "response to" line, which
             * we aren't handling yet.
             */
            break;
	case 'X':
            /* Attachment line - maintain comma-separated list of attachments */
#if ATTACHMENTS
	    if (buf[2] == ' ') break;  /* DELETED */
	    *(a= firstin(buf+2,"\n"))= '\0';
	    if (attach == NULL)
		attach= strdup(buf+2);
	    else
	    {
		int m= strlen(attach);
		attach= (char *)realloc(attach,m+a-buf);
		attach[m]= ',';
		strcpy(attach+m+1,buf+2);
	    }
#endif
            break;
	case ':':
            /* Ignore this -- probably an HTML response commented out for
             * Yapp compatibility.  We'd only be seeing it here if we
             * aren't compiled with HIDE_HTML.
             */
            break;
	default:
	    die("Unexpected ,%c in item %s header",buf[1],item_path);
	}
    }
    die("Premature end of file in %s header",item_path);
}


/* RESPHEAD - Read in the header for a response, saving the various values in
 * various sysdict variables.  The desired response number is given.
 * Return 1 if there is no such item or response.
 *
 * This should not be used for response 0.  Call itemhead instead.
 *
 * If we have both parent pointers and hidden HTML then this gets just a
 * bit messy.  These two tweaks to Picospan file formats don't mesh very
 * cleanly.
 */

int resphead(int resp)
{
    char buf[BFSZ];
    char *a, *attach= NULL;
    int parent= 0;
#if defined(PARENT_POINTERS) && defined(HIDE_HTML)
    long text_offset= 0L;
#endif

    /* Make sure file is opened */
    if (openitem(0)) return 1;

    /* If we are not already at the right response, jump to it */
    if (resp != reading_resp || read_head)
    {
	if ((reading_resp= seek_resp(item_fp,resp)) < 0)
	{
	    read_head= FALSE;
	    return 1;
	}
    }
    else
    	fseek(item_fp, 2L, 1);	/* Skip ",R" */

    /* Read and Parse the rest of the ,R line */
    if (fgets(buf,BFSZ,item_fp) == NULL)
    	return 1;
    seti(VAR_HIDDEN,buf[3] == '1');
    seti(VAR_ERASED,buf[3] == '3');
    resp_type= buf[2] - '0';
    sets(VAR_TEXTTYPE, resp_type_name(resp_type), SM_COPY);

    /* Parse the rest of the header */
    while (fgets(buf,BFSZ,item_fp) != NULL)
    {
#if defined(PARENT_POINTERS) && defined(HIDE_HTML)
	if (text_offset > 0L && (buf[0] != ',' || buf[1] != 'P'))
	{
	    fseek(item_fp, text_offset, 0);
	    goto done;
	}
#endif
	if (buf[0] != ',') continue;
	switch(buf[1])
	{
	case 'A':
	    *firstin(buf+2,"\n")= '\0';
	    sets(VAR_AUTHORNAME,bdict(VAR_MAYSEEALIAS)?buf+2:"",SM_COPY);
	    break;
	case 'U':
	    seti(VAR_AUTHORUID,atoi(buf+2));
	    if ((a= strchr(buf+2,',')) == NULL)
		die("Bad ,U syntax in item %s rhead",item_path);
	    *firstin(a+1,"\n")= '\0';
	    sets(VAR_AUTHORID,a+1,SM_COPY);
	    break;
	case 'D':
	    sett(VAR_DATE,atoth(buf+2));
	    if ((a= strchr(buf+2,' ')) != NULL)
		sett(VAR_EDITDATE,atoth(a+1));
	    else
		sett(VAR_EDITDATE,0);
#ifdef HIDE_HTML
	    if (resp_type != 0)
	    {
#ifdef PARENT_POINTERS
		text_offset= ftell(item_fp);
#else
	        goto done;
#endif /*PARENT_POINTERS*/
	    }
#endif /*HIDE_HTML*/
	    break;
#ifdef PARENT_POINTERS
	case 'P':
	    parent= atoi(buf+2);
#ifdef HIDE_HTML
	    if (resp_type != 0)
	    	goto done;
#endif /*HIDE_HTML*/
	    break;
#endif /*PARENT_POINTERS*/
	case 'X':
            /* Attachment line - maintain comma-separated list of attachments */
#if ATTACHMENTS
	    if (buf[2] == ' ') break;  /* DELETED */
	    *(a= firstin(buf+2,"\n"))= '\0';
	    if (attach == NULL)
		attach= strdup(buf+2);
	    else
	    {
		int m= strlen(attach);
		attach= (char *)realloc(attach,m+a-buf);
		attach[m]= ',';
		strcpy(attach+m+1,buf+2);
	    }
#endif
            break;
	case 'T':
	    goto done;
	case ':':
            /* Ignore this -- probably an HTML response commented out for
             * Yapp compatibility.  We'd only be seeing it here if we
             * aren't compiled with HIDE_HTML.
             */
            break;
	default:
	    die("Unexpected ,%c in item %s rhead",buf[1],item_path);
	}
    }
    die("Premature end of file in %s rhead",item_path);

    /* Have reached end of headers */
    done:
    seti(VAR_PARENTRESP, parent);
    read_head= 1;
#if ATTACHMENTS
    if (attach == NULL)
	sets(VAR_ATTACHMENTS,"",SM_COPY);
    else
	sets(VAR_ATTACHMENTS,attach,SM_FREE);
#endif
    return 0;
}


/* RESPTEXT - Read in the text of the next response.  Stores the whole text
 * in a malloced string, and returns a pointer to it.  If <format> is 0, it
 * returns it in plain text form.  If <format> is 1, it returns it as HTML.
 * If <format> is 2, it returns the best raw text.
 */

char *resptext(int form)
{
    char buf[BFSZ];
    char *b;
    char *string= NULL;
    int l, len= 0, size= 0;
#ifdef HIDE_HTML
    /* should we skip first version we find? */
    int skip= (form == 0 && resp_type != 0) || (form == 2 && resp_type == 2);
#endif

    if (!read_head)
	die("Not ready to read_text");

    while (fgets(buf,BFSZ,item_fp) != NULL)
    {
	b= buf;
	if (b[0] == ',')
	{
	    switch (b[1])
	    {
	    case 'R':	/* Sometimes the ,E is missing for some fool reason */
		fseek(item_fp,-strlen(b),1);
		/* drop through */
	    case 'E':
		goto eot;
	    case ',':
#ifdef YAPP_COMPAT
		b+= 2;	/* tripled comma means line starts with one comma */
#else
		b++;	/* doubled comma means line starts with one comma */
#endif
		break;
	    case ':':   /* ,: just gets dropped off line */
		b+= 2;
		break;
#ifdef HIDE_HTML
	    case 'T':
	    	skip= !skip;
	    	if (resp_type > 0)
		    continue;
#endif
	    	/* drop through */
	    default:
		die("Unexpected ,%c in response text of %s",b[1],item_path);
	    }
	}

#ifdef HIDE_HTML
	if (skip) continue;
#endif

	if (len + (l= strlen(b)) > size)
	{
	    size= len + l + BFSZ;
	    if (string == NULL)
		string= (char *)malloc(size+1);
	    else
		string= (char *)realloc(string,size+1);
	}
	strcpy(string+len,b);
	len+= l;
    }
eot: 

    if (form == 1 && len > 0)
    {
	/* Convert plain text (or Yapp HTML) to HTML */
	if (resp_type == 0)
	{
	    if (idict(VAR_YAPP_FORMAT)&2)
	    {
		/* Assume it's a webYapp posting - expand cross-reference links,
		 * clean the HTML and append <BR> to each line. */
		string= format_html(string, "<BR>\n", FALSE, TRUE);
	    }
	    else
	    {
		/* A plain text posting.  Do an "expand" on text and surround
		 * it by <PRE> and </PRE> */
		string= format(string, TRUE, TRUE, 79,
			"<PRE>", "</PRE>\n", NULL);
	    }
	}
	else
	{
	    /* An HTML posting - expand cross-reference links */
	    string= format_html(string, NULL, TRUE, TRUE);
	}
	len= strlen(string);
    }

    /* Convert HTML or plain-as-HTML to plain */
#ifndef HIDE_HTML
    else if (len > 0 && ((form == 0 && resp_type != 0) ||
			 (form == 2 && resp_type == 2)))
    {
	char *oldstring= string;
    	string= format_html2text(oldstring);
	free(oldstring);
    	len= strlen(string);
    }
#endif /*HIDE_HTML*/
    else if (form == 2 && resp_type == 1 && (idict(VAR_YAPP_FORMAT)&1))
    {
	/* Want raw version of HTML response that has been Yapp-proofed -
	 * replace all <!--\n--> sequences with simple \n
	 */
	char *zr, *zw;
	for (zr= zw= string; *zr != '\0'; zr++, zw++)
	{
	    if (!strncmp(zr," <!--\n-->",9))
	        { *zw= '\n'; zr+= 8; }
	    else if (!strncmp(zr," <!-- \n-->",10))
	        { *zw= '\n'; zr+= 9; }
	    else
		*zw= *zr;
	}
	*zw= '\0';
	len= zw - string;
    }

    /* If we aren't blindfolded, mark the item as seen */
    if (!bdict(VAR_BLINDFOLD))
	update_item(idict(VAR_ITEM), reading_resp, time(0L), FALSE);

    /* Set up for reading the next item */
    reading_resp++;
    read_head= FALSE;

    /* Return the string */
    return ((string == NULL) ? strdup("") : (char *)realloc(string,len+1));
}


/* ATOTH -- This interprets a character string as a hex string and returns an
 * time_t equivalent.
 */

time_t atoth(char *str)
{
    time_t n=0;

    for( ; isascii(*str) && isxdigit(*str); str++)
	    n= n*16 + *str - (isdigit(*str) ? '0'
					    : (isupper(*str) ? 'A' : 'a') - 10);
    return n;
}


/* ITEM_POWERS - After opening a new item and setting all the descriptive
 * variables (like "frozen", "linked", "authorid", etc), this routine can
 * be called to set "mayfreeze", "mayretire", "mayretitle" and "maykill" to
 * the appropriate values for the item, and also set "mayhide" and "mayshow"
 * to the values for the item text.
 *
 * The various variables in here have values with the following meanings:
 *   -1    We haven't decided yet if the user has this power
 *    0    The user doesn't have this power
 *    1    The user has this power because he is the item author
 *    2    The user has this power because he is a fairwitness or admin
 */

void item_powers()
{
    int freeze= -1, retire= -1, kill= -1;
    int retitle= -1, erase= -1, hide= -1, edit= -1;

    /* Normally can't do much with frozen items */
    if (!bdict(VAR_EDITFROZEN) && bdict(VAR_FROZEN))
    {
    	retitle= 0;
    	erase= 0;
    	hide= 0;
	edit= 0;
    }

    /* Impossible to hide or unerase responses that are erased */
    if (bdict(VAR_ERASED))
    {
    	erase= 0;
    	hide= 0;
    }

    /* Authors have all sorts of magical powers sometimes*/
    if (idict(VAR_AUTHORUID) == idict(VAR_UID) &&
        !strncmp(sdict(VAR_AUTHORID),sdict(VAR_ID),8))
    {
	if (bdict(VAR_AUTHOR_FREEZE) && freeze == -1) freeze= 1;
	if (bdict(VAR_AUTHOR_RETIRE) && retire == -1) retire= 1;
	if (bdict(VAR_AUTHOR_KILL) && idict(VAR_MAXRESP) == 0 && kill == -1)
		kill= 1;
	if (bdict(VAR_AUTHOR_RETITLE) && retitle == -1) retitle= 1;
	if (bdict(VAR_AUTHOR_ERASE) && erase == -1) erase= 1;
	if (bdict(VAR_AUTHOR_HIDE) && hide == -1) hide= 1;
	if (bdict(VAR_AUTHOR_EDIT) && edit == -1) edit= 1;
    }

    /* Administrators and Fairwitnesses can leap tall buildings */
    if (bdict(VAR_AMFW))
    {
    	if (kill == -1) kill= 2;

    	if (bdict(VAR_AMADM) || !bdict(VAR_LINKED))
    	{
	    if (freeze == -1) freeze= 2;
	    if (retire == -1) retire= 2;
	    if (retitle == -1) retitle= 2;
    	}
    	if ((bdict(VAR_AMADM) || (bdict(VAR_FW_ERASE) && !bdict(VAR_LINKED))) &&
    	    erase == -1) erase= 2;
    	if ((bdict(VAR_AMADM) || (bdict(VAR_FW_HIDE) && !bdict(VAR_LINKED))) &&
    	     hide == -1) hide= 2;
    	if ((bdict(VAR_AMADM) || (bdict(VAR_FW_EDIT) && !bdict(VAR_LINKED))) &&
    	     edit == -1) edit= 2;
    }

    /* Plain ordinary users haven't got much */
    if (freeze == -1) freeze= 0;
    if (retire == -1) retire= 0;
    if (kill == -1) kill= 0;
    if (retitle == -1) retitle= 0;
    if (erase == -1) erase= 0;
    if (hide == -1) hide= 0;
    if (edit == -1) edit= 0;

    /* Go and set the variables */
    seti(VAR_MAYFREEZE, freeze);
    seti(VAR_MAYRETIRE, retire);
    seti(VAR_MAYKILL, kill);
    seti(VAR_MAYRETITLE, retitle);
    seti(VAR_MAYERASE, erase);
    seti(VAR_MAYHIDE, hide);
    seti(VAR_MAYEDIT, edit);
}


/* RESP_POWERS - After opening a new response in an item and setting all the
 * descriptive variables (like "hidden", "erased", "authorid", etc), this
 * routine can be called to set "mayhide", "mayedit" and "mayshow" to the
 * values for that response.
 */

void resp_powers()
{
    int erase= -1, hide= -1;   /* -1==dunno, 0==no, 1==yes(author), 2=yes(fw) */
    int edit= -1;

    /* Difficult to do anything to frozen items */
    if (!bdict(VAR_EDITFROZEN) && bdict(VAR_FROZEN))
    {
    	edit= 0;
	erase= 0;
	hide= 0;
    }

    /* Can't erase or hide erased items (but can edit them) */
    if (bdict(VAR_ERASED))
    {
	erase= 0;
	hide= 0;
    }

    /* Authors sometimes may */
    if (idict(VAR_AUTHORUID) == idict(VAR_UID) &&
        !strncmp(sdict(VAR_AUTHORID),sdict(VAR_ID),8))
    {
	if (bdict(VAR_AUTHOR_ERASE) && erase == -1) erase= 1;
	if (bdict(VAR_AUTHOR_HIDE) && hide == -1) hide= 1;
	if (bdict(VAR_AUTHOR_EDIT) && edit == -1) edit= 1;
    }

    /* Administrators may */
    if (bdict(VAR_AMADM))
    {
	if (hide == -1) hide= 2;
	if (erase == -1) erase= 2;
	if (edit == -1) edit= 2;
    }

    /* Fairwitnesses sometimes may */
    if (bdict(VAR_AMFW))
    {
	if (erase == -1 && bdict(VAR_FW_ERASE) && !bdict(VAR_LINKED)) erase= 2;
	if (hide == -1 && bdict(VAR_FW_HIDE) && !bdict(VAR_LINKED)) hide= 2;
	if (edit == -1 && bdict(VAR_FW_EDIT) && !bdict(VAR_LINKED)) edit= 2;
    }

    /* Strangers may not */
    if (erase == -1) erase= 0;
    if (hide == -1) hide= 0;
    if (edit == -1) edit= 0;

    /* Go and set the variables */
    seti(VAR_MAYERASE, erase);
    seti(VAR_MAYHIDE, hide);
    seti(VAR_MAYEDIT, edit);
}
