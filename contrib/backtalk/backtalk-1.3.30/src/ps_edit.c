/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* ROUTINES TO EDIT ITEM FILES
 *
 * This implements various rarely used operations on Picospan items files:
 * hiding, erasing, editing, retitling, freezing, retiring, killing, linking.
 *
 * All these routines open item files on their own, instead of using
 * the openitem() mechanism.  I've had problems with this causing internal
 * deadlocks, so I made sure they all call closeitem() before opening anything.
 * But I'd be much happier if all items were opened my a single routine.  We
 * should never have more than one open at a time, no matter what people put
 * in the script files.
 */

#include "backtalk.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <errno.h>

#include "ps_edit.h"
#include "ps_post.h"
#include "date.h"
#include "vstr.h"
#include "dict.h"
#include "sysdict.h"
#include "str.h"
#include "format.h"
#include "ps_conflist.h"
#include "ps_config.h"
#include "ps_sum.h"
#include "ps_acl.h"
#include "lock.h"
#include "baaf.h"
#include "baaf_core.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char item_file[BFSZ];


/* OPEN_RESP - Open an item, and move to the first character after the ",R"
 * line of the response "resp".  "resp" may be zero, in which case we go to
 * the item header.  Returns NULL if no such item or response.  If write is
 * true, we must have write access.  If the item is frozen and the 
 * "editfrozen" flag is true, then we briefly thaw it while opening, but
 * freeze it again before returning.  Also returns the response flags (if
 * rcode is not NULL), the author uid and the author id (if id is not NULL).
 */

static FILE *open_resp(int item, int resp, int write, char *rcode,
    char *id, uid_t *uid)
{
    char bf[BFSZ+1];
    long off;
    int  fd;
    struct stat st;
    char *a;
    FILE *fp;

    /* Close any other open items, to prevent deadlocks */
    closeitem();

    sprintf(item_file,"%s/_%d",sdict(VAR_CONFDIR),item);
    open_item_index(sdict(VAR_CONFDIR),item);

    /* Open and Lock the file -- frozen items thawed or opened read-only */
    if ((fd= open(item_file,O_RDWR)) < 0)
    {
        if (write)
	{
	    if (!bdict(VAR_EDITFROZEN)) return NULL;

	    /* write permit the file if we can */
	    if (stat(item_file,&st)) return NULL;
	    if (chmod(item_file, st.st_mode|0200)) return NULL;

	    /* Try opening it again */
	    fd= open(item_file,O_RDWR);

	    /* Immediately restore the old perms */
	    chmod(item_file, st.st_mode);

	    if (fd < 0) return NULL;

	    /* Success */
	    lock_exclusive(fd,item_file);
	    fp= fdopen(fd,"r+");
	}
	else
	{
	    /* Fall back to opening it read only */
	    if ((fd= open(item_file,O_RDONLY)) < 0)
		return(NULL);
	    lock_shared(fd,item_file);
	    fp= fdopen(fd,"r");
	}
    }
    else
    {
	lock_exclusive(fd,item_file);
	fp= fdopen(fd,"r+");
    }

    /* Seek to the response */
    if (seek_resp(fp,resp) != resp)
    {
    	fclose(fp);
    	return(NULL);
    }

    /* Save the offset */
    off= ftell(fp);

    /* Read rest of ,R line and save rcode */
    if (fgets(bf,BFSZ,fp) == NULL)
    	die("EOF in response header");
    if (rcode != NULL) strncpy(rcode, bf, 4);

    /* Read ,U line and save uid and id */
    if (fgets(bf,BFSZ,fp) == NULL || bf[0] != ',' || bf[1] != 'U' ||
	(a= strchr(bf+2,',')) == NULL)
	die("bad response format");
    *uid= atoi(bf+2);
    if (id != NULL)
    {
	*firstin(a+1,"\n")= '\0';
	strncpy(id,a+1,MAX_LOGIN_LEN);
	id[MAX_LOGIN_LEN]= '\0';
    }

    /* Seek back to just after ,R */
    fseek(fp, off, 0);
    return(fp);
}


/* CONFLOG - Given the conference directory name and some printf() style
 * arguments (there should NOT be a terminating newline on the format string),
 * write a log message to the conference log file.  We don't do any locking,
 * but we do do the write in one call to minimize the damage.
 */

#ifdef CONF_LOG
#if __STDC__
static void conflog(char *confdir, char *fmt, ...)
#else
static void conflog(confdir, fmt, va_alist)
    char *confdir;
    char *fmt;
    va_dcl
#endif
{
    va_list ap;
    char fn[BFSZ+1];
    char *bf;
    FILE *fp;
    time_t now= time(0L);

    VA_START(ap,fmt);

    /* Generate name of log file */
    strcpy(fn,confdir);
    strcat(fn,"/log");

    /* Prefix format string with date and append newline */
    if ((bf= malloc(strlen(fmt)+25)) == NULL)
    	die("malloc failed in conflog()");
    strcpy(bf,ctime(&now)+4);
    bf[20]= ' ';
    strcpy(bf+21,fmt);
    strcat(bf,"\n");

    /* Do the write */
    if ((fp= fopen(fn,"a")) == NULL)
    	die("Cannot open log file %s",fn);
    vfprintf(fp,bf,ap);
    fclose(fp);
    va_end(ap);
}
#endif /*CONF_LOG*/


/* HIDE_RESP - Mark a response hidden or unhidden.  Return 2 if no such item
 * or response, 1 if the response was already set that way or was scribbled,
 * or 0 on success.
 */

int hide_resp(int item, int resp, int hide)
{
    FILE *fp;
    char rcode[4];
    uid_t ruid;
    char rid[MAX_LOGIN_LEN+1];

    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	die("anonymous users cannot hide responses");

    if ((fp= open_resp(item,resp,1,rcode,rid,&ruid)) == NULL)
	return 2;

    if (!bdict(VAR_AMADM))
    {
	struct stat st;
    	int am_author= (ruid == idict(VAR_UID) &&
	                !strncmp(rid,sdict(VAR_ID),MAX_LOGIN_LEN));

	if (!am_author && !bdict(VAR_AMFW))
	    die("attempt to hide a response not your own");
        if (am_author && !bdict(VAR_AMFW) && !bdict(VAR_AUTHOR_HIDE))
	    die("response hiding by authors not enabled.");
	if (bdict(VAR_AMFW) && !bdict(VAR_FW_HIDE))
	    die("response hiding by fairwitnesses not enabled.");
	if (!am_author && (fstat(fileno(fp),&st) || st.st_nlink > 1))
	    die("fairwitness may not censor linked items.");
    }

    if (rcode[3] == '3' || (hide && rcode[3] != '0') ||
			   (!hide && rcode[3] != '1'))
    {
	fclose(fp);
	unlock(item_file);
	return(1);
    }

    rcode[3]= hide ? '1' : '0';

    fwrite(rcode, 1, 4, fp);

    fclose(fp);
    unlock(item_file);
    return(0);
}


/* ERASE_RESP - Erase a response.  Return 2 if there was no such item or
 * response, 1 if the response was already erased,  or 0 on success.
 */

int erase_resp(int item, int resp)
{
    FILE *fp, *lfp;
    char rcode[4];
    uid_t ruid;
    int i,j;
    char bf[BFSZ+1];
    long textoff;
    size_t textlen;
    time_t clock;
    char *scrib[3];
#ifdef HIDE_HTML
    int hidden_html;
#endif

    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	die("anonymous users cannot erase responses");

    if ((fp= open_resp(item,resp,1,rcode,bf,&ruid)) == NULL)
	return(2);

    if (!bdict(VAR_AMADM))
    {
	struct stat st;
    	int am_author= (ruid == idict(VAR_UID) &&
	                !strncmp(bf,sdict(VAR_ID),MAX_LOGIN_LEN));

	if (!am_author && !bdict(VAR_AMFW))
	    die("attempt to erase a response not your own");
        if (am_author && !bdict(VAR_AMFW) && !bdict(VAR_AUTHOR_ERASE))
	    die("response erasing by authors not enabled.");
	if (bdict(VAR_AMFW) && !bdict(VAR_FW_ERASE))
	    die("response erasing by fairwitnesses not enabled.");
	if (!am_author && (fstat(fileno(fp),&st) || st.st_nlink > 1))
	    die("fairwitness may not censor linked items.");
    }

    /* Already erased - go home */
    if (rcode[3] == '3')
    {
	fclose(fp);
	unlock(item_file);
	return(1);
    }

#ifdef CENSOR_LOG
    /* Start writing the logfile entry */
    lfp= fopen(CENSOR_LOG,"a");
    if (lfp)
    {
	clock= time(0L);
	fprintf(lfp,
#ifdef HIDE_HTML
	    "response %s:%d:%d (%.8s) censored by (%s) on %s",
#else
	    "response %s:%d:%d (%.8s) censored by (%s) on %s,T\n",
#endif /* HIDE_HTML */
	    sdict(VAR_CONF), item, resp, bf, sdict(VAR_ID),
	    ctimez(&clock,NULL));
    }
#endif /* CENSOR_LOG */

    /* Detect responses with hidden HTML */
#ifdef HIDE_HTML
    hidden_html= (rcode[2] != '0');
#ifdef CENSOR_LOG
    if (!hidden_html && lfp)
    	fputs(",T\n",lfp);
#endif /* CENSOR_LOG */
#endif /* HIDE_HTML */

    /* Mark it erased and plain-text */
    rcode[2]= '0';
    rcode[3]= '3';
    fwrite(rcode, 1, 4, fp);

    fseek(fp,1L,1);	/* we need a seek between reading and writing */

    /* Find start of response */
    for (;;)
    {
	if (fgets(bf,BFSZ,fp) == NULL)
	    die("Premature EOF in item file");
#ifdef HIDE_HTML
	if (hidden_html && bf[0] == ',' && bf[1] == 'D')
	    break;
#endif
	if (bf[0] == ',' && bf[1] == 'T')
	    break;
#if ATTACHMENTS
	if (bf[0] == ',' && bf[1] == 'X' && bf[2] != ' ')
	{
	    int len= strlen(bf);
	    long off= ftell(fp);
	    /* Delete Index entry for attachmetns */
	    bf[len-1]= '\0';
	    del_attach(bf+2);

	    /* Overwrite handle with 'spaces' to erase it */
	    fseek(fp, -len+2, 1);
	    for (i= 0; i < len-3; i++) fwrite(" ",1,1,fp);
	    fseek(fp, off, 0);
	}
#endif
    }
    textoff= ftell(fp);

    /* Find end of response */
    do {
	if (fgets(bf,BFSZ,fp) == NULL)
	    die("Premature EOF in response text");
#ifdef CENSOR_LOG
	if (lfp) fputs(bf,lfp);
#endif
    } while (bf[0] != ',' || (bf[1] != 'E' && bf[1] != 'R'));
    textlen= ftell(fp) - strlen(bf) - textoff;
#ifdef CENSOR_LOG
    if (lfp) fclose(lfp);
#endif

    /* Make scribble string */
    clock= time(0L);
    scrib[0]= sdict(VAR_ID);
    scrib[1]= ctimez(&clock,NULL); scrib[1][24]= '\0';
    scrib[2]= sdict(VAR_ALIAS);
    for (i= j= 0; i < 75; j= (j+1)%3)
    {
	strcpy(bf+i, scrib[j]);
	i+= strlen(scrib[j]);
	bf[i++]= ' ';
    }
    bf[75]='\n';

    /* Return to start of text */
    fseek(fp,textoff,0);

#ifdef HIDE_HTML
    if (hidden_html)
    {
    	fwrite(",T\n", 1, 3, fp);
	textlen-= 3;
    }
#endif

    /* Overwrite text */
    while (textlen > 76)
    {
	fwrite(bf, 1, 76, fp);
	textlen-= 76;
    }
    if (textlen > 0)
    {
	bf[textlen-1]='\n';
	fwrite(bf, 1, textlen, fp);
    }

    fclose(fp);
    unlock(item_file);
    return(0);
}


/* ITEM_MODE - Mark an item as being retired or frozen.  Return 0 on success,
 * 2 if there is no such item, 3 if you don't have the proper access, 1 if
 * the item already has the given modes.
 */

int item_mode(int item, int im_cmd)
{
    struct stat st;
    FILE *fp;
    char rid[MAX_LOGIN_LEN+1];
    uid_t ruid;
    int rc;

    /* Refuse anonymous users */
    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	return(3);

    if ((fp= open_resp(item,0,0,NULL,rid,&ruid)) == NULL)
	return(2);

    /* Stat the item */
    if (fstat(fileno(fp),&st))
	die("stat failed");

    /* Check if user has permission to do this operation */
    if (bdict(VAR_AMFW))
    {
    	/* Fairwitnesses can do it if the item is not linked */
    	if (!bdict(VAR_AMADM) && st.st_nlink > 1)
	    { rc= 3; goto done; }
    }
    else
    {
	/* Item authors can do it if the configuration says the can */
	if (ruid != idict(VAR_UID) ||
	    strncmp(rid,sdict(VAR_ID),MAX_LOGIN_LEN) || 
	    ((im_cmd == IM_FREEZE || im_cmd == IM_THAW) &&
	     !bdict(VAR_AUTHOR_FREEZE)) ||
	    ((im_cmd == IM_RETIRE || im_cmd == IM_UNRETIRE) &&
	     !bdict(VAR_AUTHOR_RETIRE)))
	    { rc= 3; goto done; }
    }

    rc= 1;
    switch(im_cmd)
    {
    case IM_FREEZE:
	if (!(st.st_mode & 0200)) goto done;
#ifdef CONF_LOG
	conflog(sdict(VAR_CONFDIR),"%s froze item %d",sdict(VAR_ID),item);
#endif
	st.st_mode&= ~0200;
	(void) put_sum(item, -1, SF_FROZEN, 0, 0L, 0L);
	break;
    case IM_THAW:
	if (st.st_mode & 0200) goto done;
#ifdef CONF_LOG
	conflog(sdict(VAR_CONFDIR),"%s thawed item %d",sdict(VAR_ID),item);
#endif
	(void) put_sum(item, -1, 0, SF_FROZEN, 0L, 0L);
	st.st_mode|= 0200;
	break;
    case IM_RETIRE:
	if (st.st_mode & 0100) goto done;
#ifdef CONF_LOG
	conflog(sdict(VAR_CONFDIR),"%s retired item %d",sdict(VAR_ID),item);
#endif
	st.st_mode|= 0100;
	(void) put_sum(item, -1, SF_RETIRED, 0, 0L, 0L);
	break;
    case IM_UNRETIRE:
	if (!(st.st_mode & 0100)) goto done;
#ifdef CONF_LOG
	conflog(sdict(VAR_CONFDIR),"%s unretired item %d",sdict(VAR_ID),item);
#endif
	st.st_mode&= ~0100;
	(void) put_sum(item, -1, 0, SF_RETIRED, 0L, 0L);
	break;
    }
    rc= 0;

    fchmod(fileno(fp), st.st_mode);
	
done:
    fclose(fp);
    unlock(item_file);

    return(rc);
}


/* SCAN_ATTACH - Call the given function for each attachment found in the
 * currently open item whose file descriptor is given.  The attachment
 * handle is passed to the called routine.
 */

#if ATTACHMENTS
void scan_attach(FILE *fp, void (*func)(char *))
{
    char bf[BFSZ+1], *nl, wasnl= 1;
    rewind(fp);

    while (fgets(bf,BFSZ,fp) != NULL)
    {
	nl= strchr(bf,'\n');

	if (wasnl && bf[0] == ',' && bf[1] == 'X' && bf[2] != ' ')
	{
	    if (nl == NULL) die("attachment line too long");
	    *nl= '\0';
	    (*func)(bf+2);
	}
	wasnl= (nl != NULL);
    }
}
#endif /* ATTACHMENTS */


/* KILL_ITEM - Kill an item.  Returns 0 on success, 2 if there is no such
 * item, 3 if you don't have the proper access.
 */

int kill_item(int item)
{
    FILE *fp;
    char rid[MAX_LOGIN_LEN+1];
    uid_t ruid;
    int nresp,flags,rc;
    time_t idate, ldate;
#if ATTACHMENTS
    struct stat st;
#endif

    /* Deny anonymous users */
    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	return(3);

    if ((fp= open_resp(item,0,0,NULL,rid,&ruid)) == NULL)
	return(2);

    /* Can kill if we are FW, or if we are author and there are no responses */
    if (!bdict(VAR_AMFW))
    {
	rc= 3;
	if (ruid != idict(VAR_UID) ||
	    strncmp(rid,sdict(VAR_ID),MAX_LOGIN_LEN) ||
	    !bdict(VAR_AUTHOR_KILL))
	    goto done;
	rc= 2;
	if (get_sum(item, &nresp, &flags, &idate, &ldate))
	    goto done;
	rc= 3;
	if (nresp > 1)
	    goto done;
    }
#ifdef CONF_LOG
    conflog(sdict(VAR_CONFDIR),"%s killed item %d",sdict(VAR_ID),item);
#endif
    rc= 0;
    /* There is some risk of a response slipping in after this check */

    /* Nuke the file */
    unlink(item_file);

    /* Scrub it from sum file */
    (void) put_sum(item, 0, 0, -1, 0L, 0L);

    /* Nuke the index file */
    remove_index(sdict(VAR_CONFDIR),item);

#if ATTACHMENTS
    /* Remove any attachments if we removed the last link */
    if (fstat(fileno(fp),&st))
	die("could not stat item file");
    if (st.st_nlink == 0)
	scan_attach(fp, &del_attach);
#endif

done:
    fclose(fp);
    unlock(item_file);

    return(rc);
}


/* LINK_ITEM:  Create a link from the given item in the given conference to
 * the current conference.  Sets this_item to the new item number in this
 * conference and returns an error code:
 *    0 success
 *    1 cannot access other conference - it is private or doesn't exist.
 *    2 no such item
 *    3 no access - you are not fairwitness
 */

int link_item(int that_item, char *that_conf, int *this_item)
{
    char *that_dir;
    char this_file[BFSZ], that_file[BFSZ];
    int mode;

    /* User must be fairwitness in this conference */
    if (!bdict(VAR_AMFW))
	return(3);

    /* Locate source conference */
    if ((that_dir= cfpath(that_conf)) == NULL)
	return(1);

    /* Check that source conference is not private */
    if (!bdict(VAR_AMADM))
    {
	readconfig(that_dir, NULL, NULL, &mode, NULL);
	if (mode < 0)
	{
	    /* For conference with acl, we instead require that the linker
	     * has read/write access.  This isn't really right.  We really
	     * need conferences to have a fw-settable 'linkout' flag'.  But
	     * this is an acceptable kludge for now, I guess.
	     */
	    int imayread, imayresp;
	    read_acl(that_dir, &imayread, &imayresp, NULL);
	    if (!imayread || !imayresp)
		return 1;
	}
	mode&= 7;
	if (mode >= 4 && mode <= 6)
	    return 1;
    }

    /* Construct name of source item */
    sprintf(that_file,"%s/_%d", that_dir, that_item);

    /* Make the Link */
    *this_item= last_no() + 1;
    for (;;)
    {
	sprintf(this_file,"%s/_%d",sdict(VAR_CONFDIR), *this_item);
	if (!link(that_file,this_file))
		break;

	if (errno != EEXIST) return(2);
	(*this_item)++;
    }

    link_index(that_dir, that_item, sdict(VAR_CONFDIR), *this_item);
    link_sum(that_dir, that_item, sdict(VAR_CONFDIR), *this_item);

#ifdef CONF_LOG
    conflog(sdict(VAR_CONFDIR),"%s linked item %d from %s %d",
    	sdict(VAR_ID),*this_item,that_conf,that_item);
    conflog(that_dir,"%s linked item %d to %s %d",
    	sdict(VAR_ID),that_item,sdict(VAR_CONF),*this_item);
#endif

    return 0;
}


extern int curr_item;
extern FILE *item_fp;
extern char item_path[];

/* RETITLE_ITEM -- Change the title of the item given by the system variable
 * "item" in the conference given by the system variable "confdir" to the
 * given string.  A check is made that you have appropriate access.  Returns
 * 1 if the title is bad (blank mostly), 2 if the item does not exist or is
 * frozen, 3 if access is denyed, and 0 on success.
 */

int retitle_item(char *newtitle)
{
    char buf[BFSZ];
    char temp_path[BFSZ];
    int temp_fd;
    FILE *temp_fp;
    size_t temp_sz= 0L;
    int ch;
    struct stat st;
    int need_authorship= 0;

    /* Anonymous users can't retitle */
    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	return 3;

    /* Open item to read/write with an exclusive lock on it */
    if (openitem(2)) return 2;
    rewind(item_fp);

    if (fstat(fileno(item_fp),&st))
	die("Unable to stat item");
    
    /* Adm always can retitle, fairwitness can if item is not linked */
    if (!(bdict(VAR_AMADM) ||
         (bdict(VAR_AMFW) && bdict(VAR_FW_RETITLE) && st.st_nlink == 1)))
    {
	/* Item author can retitle if option is on */
	if (bdict(VAR_AUTHOR_RETITLE))
	    need_authorship= 1;
	else
	{
	    closeitem();
	    return 3;
        }
    }

    /* Drop leading spaces and clip title at line break */
    newtitle= firstout(newtitle," \t");
    *firstin(newtitle,"\n\r")= '\0';
    if (*newtitle == '\0') return 1;

    /* Create temp file */
    sprintf(temp_path,"%s.temp",item_path);
    temp_fd= open(temp_path, O_RDWR|O_CREAT|O_EXCL, 0600);
    if (temp_fd < 0)
    {
    	if (errno == EEXIST)
    	    die("%s already exists -- I won't overwrite it",temp_path);
	else
	    die("cannot create %s",temp_path);
    }
    temp_fp= fdopen(temp_fd, "r+");

    /* Check and copy magic code number */
    if (fgets(buf,BFSZ,item_fp) == NULL || strcmp(buf,"!<ps02>\n"))
    {
    	unlink(temp_path);
	die("Bad magic number in item file %s",item_path);
    }
    fputs(buf, temp_fp);
    temp_sz+= strlen(buf);

    /* Fetch old header line and write new one */
    if (fgets(buf,BFSZ,item_fp) == NULL || buf[0] != ',' || buf[1] != 'H')
    {
    	unlink(temp_path);
	die("Missing title line in %s",item_path);
    }
    fputs(",H", temp_fp);
    fputs(newtitle, temp_fp);
    fputc('\n',temp_fp);
    temp_sz+= strlen(newtitle) + 3;

    /* Copy up through the the authorship line, and check that */
    if (need_authorship)
    {
    	for(;;)
    	{
	    if (fgets(buf,BFSZ,item_fp) == NULL)
	    {
		unlink(temp_path);
		die("Missing ,U author line in %s",item_path);
	    }

	    fputs(buf, temp_fp);
	    temp_sz+= strlen(buf);

	    if (buf[0] == ',' && buf[1] == 'U')
	    {
	    	char *p;
	    	uid_t uid;
	    	uid= atoi(buf+2);
	    	if ((p= strchr(buf+3,',')) == NULL)
	    	{
		    unlock(item_path);
		    unlink(temp_path);
		    die("Bad ,U author line in %s",item_path);
	    	}
	    	p++;
	    	*firstin(p,"\n")= '\0';
	    	if (uid != idict(VAR_UID) || strcmp(p,sdict(VAR_ID)))
	    	{
		    closeitem();
		    unlink(temp_path);
		    return 3;
	    	}
	    	break;
	    }
	}
    }

    /* Copy over rest of file */
    while ((ch= getc(item_fp)) != EOF)
    {
	putc(ch,temp_fp);
	temp_sz++;
    }

    /* Check that the copy wasn't munged by running out of disk or something */
    fflush(temp_fp);
    if (fstat(temp_fd,&st) || st.st_size != temp_sz)
    {
    	unlock(item_path);
    	unlink(temp_path);
    	die("Item file write failed -- out of disk space?");
    }

    /* "cp" the temp file into the old file -- we don't want to do a "mv" here
     * because we'd lose any links to the old file.
     */
    rewind(item_fp);
    rewind(temp_fp);
    while ((ch= getc(temp_fp)) != EOF)
	putc(ch,item_fp);
    ftruncate(fileno(item_fp),temp_sz);

    /* Check that the copy succeeded */
    if (fstat(fileno(item_fp),&st) || st.st_size != temp_sz)
    {
    	unlock(item_path);
    	/* Don't delete the temp file -- it's our only full copy */
    	die("Item copy failed -- out of disk space?  "
    	    "Full item text is in %s", temp_path);
    }

    /* Rebuild the index file - it's probably out of date.
     * It'd be nicer do this while copying the file in the loop above, but for
     * now we'll go ahead and make yet another pass through it.  Sigh.
     */
    rebuild_index(item_fp,sdict(VAR_CONFDIR),curr_item);

    /* Close stuff, remove the temp file, and unlock the item file */
    fclose(temp_fp);
    unlink(temp_path);
    closeitem();

    return 0;
}


/* EDIT_RESP -- Edit the response designated by the system variables "resp",
 * "item" and "confdir".
 *
 * If repl_text is true, then we change the content of the response to the
 * values given by the system variables "text" and "texttype".  The author's
 * alias is changed to "alias".  If repl_text is false, the text is left
 * unchanged.
 *
 * If delattach is not NULL, any attachment with that handle is deleted.
 * If addattach is not NULL, that attachment is added.  If both are not
 * NULL, the added one replaces the deleted one.  If one is added, but none
 * are deleted, it is added to the end of the list of attachments.
 *
 * A check is made that you have appropriate access.  Returns 2 if the item
 * or response does not exist or is frozen, 3 if access is denyed, and 0
 * on success.  Returns 4 if delattach could not be found.
 */

int edit_resp(int repl_text, char *delattach, char *addattach)
{
    char buf[BFSZ];
    char temp_path[BFSZ];
    int temp_fd;
    FILE *temp_fp, *log_fp;
    size_t temp_sz= 0L;
    int ch, diddel= FALSE;
    struct stat st;
    int need_authorship= 0;
    int nresp, rstyle, new_style;
    char *new_text= NULL, *prime_text, *alias;
    char *p;
    uid_t ruid;
    char *rid;
    time_t rdate, clock;
    size_t roffset, rsize;
    VSTRING attach;
    char *plain_text;

    /* Anonymous users can't edit */
    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	return 3;

    /* Open item to read/write with an exclusive lock on it */
    if (openitem(2))
	return 2;

    if (fstat(fileno(item_fp),&st))
	die("Unable to stat item");
    
    /* Adm always can edit, fairwitness can if item is not linked */
    if (!(bdict(VAR_AMADM) ||
         (bdict(VAR_AMFW) && bdict(VAR_FW_EDIT) && st.st_nlink == 1)))
    {
	/* Item author can edit if option is on */
	if (bdict(VAR_AUTHOR_EDIT))
	    need_authorship= 1;
	else
	{
	    closeitem();
	    return 3;
        }
    }

    /* Create temp file */
    sprintf(temp_path,"%s.temp",item_path);
    temp_fd= open(temp_path, O_RDWR|O_CREAT|O_EXCL, 0600);
    if (temp_fd < 0)
    {
	unlock(item_path);
    	if (errno == EEXIST)
    	    die("%s already exists -- I won't overwrite it",temp_path);
	else
	    die("cannot create %s",temp_path);
    }
    temp_fp= fdopen(temp_fd, "r+");

    /* Check and copy magic code number */
    if (fgets(buf,BFSZ,item_fp) == NULL || strcmp(buf,"!<ps02>\n"))
    {
    	unlink(temp_path);
	unlock(item_path);
	die("Bad magic number in item file %s",item_path);
    }
    fputs(buf, temp_fp);
    temp_sz+= strlen(buf);

    /* copy up through header of response being changed */
    nresp= 0;
    while (fgets(buf,BFSZ,item_fp) != NULL)
    {
	/* Check if this is the start of our response */
	if (buf[0] == ',' && buf[1] == 'R' && nresp++ == idict(VAR_RESP))
	{
	    /* Save style */
	    rstyle= buf[4]-'0';
	    /* New response is hidden if old one was */
	    seti(VAR_HIDDEN,buf[5] == '1');
	    /* Editing an erased response unerases it */
	    seti(VAR_ERASED,0);

	    /* Get ,U line */
	    if (fgets(buf,BFSZ,item_fp) == NULL ||
	        buf[0] != ',' || buf[1] != 'U')
	    {
		unlink(temp_path);
		unlock(item_path);
		die("Failed to find ,U line in %s",item_path);
	    }

	    /* Extract user name and uid from line */
	    ruid= atoi(buf+2);
	    if ((p= strchr(buf+3,',')) == NULL)
	    {
		unlock(item_path);
		unlink(temp_path);
		die("Bad ,U author line in %s",item_path);
	    }
	    p++;
	    *firstin(p,"\n")= '\0';

	    /* Check if user name and uid are correct */
	    if (need_authorship &&
	        (ruid != idict(VAR_UID) || strcmp(p,sdict(VAR_ID))))
	    {
		closeitem();
		unlink(temp_path);
		return 3;
	    }
	    rid= strdup(p);

	    /* Get ,A line - only store it if we aren't editing it */
	    if (fgets(buf,BFSZ,item_fp) == NULL ||
	        buf[0] != ',' || buf[1] != 'A')
	    {
		unlink(temp_path);
		unlock(item_path);
		die("Failed to find ,A line in %s",item_path);
	    }
	    if (repl_text)
		alias= sdict(VAR_ALIAS);
	    else
	    {
		*firstin(buf+2,"\n\r")= '\0';
		alias= strdup(buf+2);
	    }

	    /* Read possible ,X lines, saving the list of attachments to
	     * the comma-separated 'attach' vstring, we modify the attachment
	     * list as we read it in.
	     */
	    vs_new(&attach,100);
	    for (;;)
	    {
		if (fgets(buf,BFSZ,item_fp) == NULL)
		    die("Unexpected EOF after ,A line in %s",item_path);
		if (buf[0] != ',' || buf[1] != 'X')
		    break;
		*firstin(buf+2, "\n\r")= '\0';
		if (delattach == NULL || strcmp(delattach,buf+2))
		    p= buf+2;
		else
		{
		    p= addattach;
		    diddel= TRUE;
		}
		if (p != NULL)
		{
		    if (attach.p > attach.begin) *vs_inc(&attach)= ',';
		    vs_cat(&attach,p);
		}
	    }
	    if (delattach != NULL && !diddel)
	    {
		closeitem();
		unlink(temp_path);
		vs_destroy(&attach);
		return 4;
	    }
	    if (addattach != NULL && delattach == NULL)
	    {
		if (attach.p > attach.begin) *vs_inc(&attach)= ',';
		vs_cat(&attach,addattach);
	    }
	    *vs_inc(&attach)= '\0';

	    /* Get ,D line and extract date from it */
	    if (buf[0] != ',' || buf[1] != 'D')
	    {
		unlink(temp_path);
		unlock(item_path);
		die("Failed to find ,D line in %s",item_path);
	    }
	    rdate= atoth(buf+2);

	    /* Don't have to read ,P pointers.  if repl_text we always reset
	     * them to new value, otherwise, they'll get copied over with
	     * the rest of the file.
	     */

	    clock= time(0L);

#ifdef CENSOR_LOG
	    /* Start writing the logfile entry */
	    log_fp= fopen(CENSOR_LOG,"a");
	    if (log_fp)
	    	fprintf(log_fp, "response %s:%d:%d (%s) edited by (%s) on %s",
			sdict(VAR_CONF), idict(VAR_ITEM), idict(VAR_RESP),
			rid, sdict(VAR_ID), ctimez(&clock,NULL));
#endif

	    if (repl_text)
	    {
		/* Skip rest of old response */
		for (;;)
		{
		    if (fgets(buf,BFSZ,item_fp) == NULL)
			die("Unterminated response in %s",item_path);
		    if (buf[0] == ',' &&  (buf[1] == 'E' || buf[1] == 'R'))
			break;
#ifdef CENSOR_LOG
		    if (log_fp) fputs(buf,log_fp);
#endif
		}
#ifdef CENSOR_LOG
		if (log_fp) { fputs(",E\n",log_fp); fclose(log_fp); }
#endif

		/* Prepare new response */
		new_style= (strcasecmp(sdict(VAR_TEXTTYPE),"text/html") == 0) ?
		    1 : 0;
		if (new_style == 1)
		{
		    /* Sanitize HTML */
		    new_text= format_html(strdup(sdict(VAR_TEXT)),
			(idict(VAR_YAPP_FORMAT)&1) ? " <!-- \n-->" : NULL, TRUE,
			FALSE);
		    if (new_text == NULL)
			die("HTML cleanup failed");
#ifdef HIDE_HTML
		    /* Construct plain text version */
		    plain_text= format_html2text(strdup(new_text));
		    if (plain_text == NULL)
			die("conversion of HTML to plaintext failed");
#endif
		}
		else if (idict(VAR_YAPP_FORMAT)&1)
		{
		    /* If posting a plain text response where webYapp can see it,
		     * we convert it to HTML, enclosing it in <TT> tags, ending
		     * each line with <BR><!-- and starting each following line
		     * with -->.  This is insane, but it makes things work in
		     * both web Yapp and Backtalk.
		     */
#ifdef HIDE_HTML
		    plain_text= strdup(sdict(VAR_TEXT));
#endif
		    new_style= 2;
		    new_text= format(strdup(sdict(VAR_TEXT)), TRUE, FALSE, 79,
			    "<TT>", "</TT>\n", "<BR><!-- \n-->");
		}
#ifdef HIDE_HTML
		else
		    nometa(sdict(VAR_TEXT));
#endif
		prime_text= (new_text == NULL) ? sdict(VAR_TEXT) : new_text;
	    }
	    else
		prime_text= plain_text= NULL;

	    /* Write new response */
#ifdef HIDE_HTML
	    if (new_style != 0)
	    {
		write_text(temp_fp, ruid, rid, alias, new_style,
		    plain_text, prime_text, rdate, clock,
		    repl_text ? idict(VAR_PARENTRESP) : 0,
		    attach.begin, &roffset, &rsize);
		free(plain_text);
	    }
	    else
#endif
		write_text(temp_fp, ruid, rid, alias, new_style,
		    prime_text, NULL, rdate, clock,
		    repl_text ? idict(VAR_PARENTRESP) : 0,
		    attach.begin, &roffset, &rsize);

	    /* Free some stuff */
	    if (new_text != NULL) free(new_text);
	    if (!repl_text) free(alias);
	    vs_destroy(&attach);
	    free(rid);

	    temp_sz+= rsize;
	}
	else
	{
	    /* Lines of other responses and stuff - just copy them */
	    fputs(buf, temp_fp);
	    temp_sz+= strlen(buf);
	}
    }

    /* Check that the copy wasn't munged by running out of disk or something */
    fflush(temp_fp);
    if (fstat(temp_fd,&st) || st.st_size != temp_sz)
    {
    	unlock(item_path);
    	unlink(temp_path);
    	die("Item file write failed -- out of disk space?");
    }

    /* "cp" the temp file into the old file -- we don't want to do a "mv" here
     * because we'd lose any links to the old file.
     */
    rewind(item_fp);
    rewind(temp_fp);
    while ((ch= getc(temp_fp)) != EOF)
	putc(ch,item_fp);
    ftruncate(fileno(item_fp),temp_sz);

    /* Check that the copy succeeded */
    if (fstat(fileno(item_fp),&st) || st.st_size != temp_sz)
    {
    	unlock(item_path);
    	/* Don't delete the temp file -- it's our only full copy */
    	die("Item copy failed -- out of disk space?  "
    	    "Full item text is in %s", temp_path);
    }

    /* Rebuild the index file - it's almost certainly out of date.
     * It'd be nicer do this while copying the file in the loop above, but for
     * now we'll go ahead and make yet another pass through it.  Sigh.
     */
    rebuild_index(item_fp,sdict(VAR_CONFDIR),curr_item);

    /* Close stuff, remove the temp file, and unlock the item file */
    fclose(temp_fp);
    unlink(temp_path);
    closeitem();

    return 0;
}
