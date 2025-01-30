/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* ROUTINES TO WRITE ITEM FILES
 *
 * This implements code to post new items and responses.
 *
 * A lot of these routines open item files on their own, instead of using
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

#include "sysdict.h"
#include "str.h"
#include "format.h"
#include "ps_post.h"
#include "ps_part.h"
#include "ps_sum.h"
#include "dict.h"
#include "lock.h"
#include "log.h"
#include "baai.h"
#include "entropy.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


char item_file[BFSZ];

/* WRITE_TEXT_BLOCK - write the part of the item or response text that goes
 * between the ",T" and the ",E" lines.  Really just copy text into file,
 * doubling any initial commas, stripping out carriage returns, and making
 * sure it ends with a newline character.  Return size of the block.
 */

static long write_text_block(FILE *fp, char *text,int is_alt)
{
    int nl= 1;
    long size= 0L;

    for (; *text != '\0'; text++)
    {
    	if (*text != '\r')		 /* strip out CR characters */
    	{
#if defined(YAPP_COMPAT) && defined(UNIX_ACCOUNTS)
	    /* For Yapp, prefix each line of an HTML response with ,:
	     * Yapp won't tolerate any line outside ,T and ,E having
	     * an upper case letter as its second character, so we insert the
	     * ,: to make sure that doesn't happen.  We throw out the ,:
	     * when reading.
	     */
	    if (is_alt && nl)
	    {
	    	fputc(',',fp);
	    	fputc(':',fp);
	    	size += 2;
	    }
	    else
#endif /* YAPP_COMPAT */
    	    /* double (triple for Yapp) initial commas */
    	    if (nl && *text == ',')
    	    {
#ifdef YAPP_COMPAT
    	    	fputc(',',fp);
		size++;
#endif
    	    	fputc(',',fp);
		size++;
    	    }

    	    /* nl is true if last character written was a newline */
    	    nl= (*text == '\n');

    	    fputc(*text,fp);
	    size++;
    	}
    }

    /* Make sure we have a trailing newline */
    if (!nl)
    {
    	fputc('\n',fp);
    	size++;
    }

    return size;
}


/* WRITE_TEXT - write item or response text to the given file in PicoSpan
 * format.  <fp> is the open file descriptor to write to.  If there is an
 * alternate version of the text, it is passed in as <alt_text>, otherwise
 * this is NULL.  Return the offset of the ",R" in the file, and the total
 * size of the response.  Attach is a comma-separated list of attachments.
 * If text is NULL, neither the text nor the enclosing ,T and ,E are written
 * (the calling program should write them).
 */

void write_text(FILE *fp, uid_t uid, char *id, char *alias,
		int style, char *text, char *alt_text, time_t postdate,
		time_t editdate, int parent, char *attach,
		size_t *offset, size_t *size)
{
    char *p;
    int nl;
#ifndef FPRINTF_RETURNS_SIZE
    char bf[BFSZ];
#endif

    /* Stir response into entropy pool (if we have one) */
    make_noise(text, -1, getenv("HTTP_USER_AGENT"), -1);

    /* Write response header */
    fputc(',',fp);
    *offset= (size_t)ftell(fp)-1;
    fprintf(fp,"R00%c%c\n",
	'0' + style,
	(bdict(VAR_HIDDEN) ? '1' : (bdict(VAR_ERASED) ? '3' : '0')));
    *size= 7;

#ifdef FPRINTF_RETURNS_SIZE

    *size+= fprintf(fp,",U%d,%s\n",uid,id);
    *size+= fprintf(fp,",A%s\n",alias);
#if ATTACHMENTS
    if (attach != NULL && *attach != '\0')
    {
	char *e, tmp;
	for(;;)
	{
	    e= firstin(attach,",");
	    tmp= *e; *e= '\0';
	    *size+= fprintf(fp,",X%s\n",attach);
	    baai_link(attach);
	    if ((*e= tmp) == '\0') break;
	    attach= e+1;
	}
    }
#endif /*ATTACHMENTS */
#ifndef NO_EDIT_DATE
    if (editdate != 0)
    	*size+= fprintf(fp,",D%lx %lx\n",postdate,editdate);
    else
#endif /*NO_EDIT_DATE*/
    	*size+= fprintf(fp,",D%lx\n",postdate);
#ifdef PARENT_POINTERS
    if (parent > 0)
    	*size+= fprintf(fp,",P%d\n",parent);
#endif /*PARENT_POINTER*/

#else

    sprintf(bf,",U%d,%.900s\n",uid,id);
    *size+= strlen(bf);
    fputs(bf,fp);
    fprintf(fp,",A%s\n",alias);
    *size+= 3 + strlen(alias);
#if ATTACHMENTS
    if (attach != NULL && *attach != '\0')
    {
	char *e, tmp;
	for(;;)
	{
	    e= firstin(attach,",");
	    tmp= *e; *e= '\0';
	    sprintf(bf,",X%s\n",attach);
	    *size+= strlen(bf);
	    fputs(bf,fp);
	    baai_link(attach);
	    if ((*e= tmp) == '\0') break;
	    attach= e+1;
	}
    }
#endif /*ATTACHMENTS */
#ifndef NO_EDIT_DATE
    if (editdate != 0)
	sprintf(bf,",D%lx %lx\n",postdate,editdate);
    else
#endif /*NO_EDIT_DATE*/
	sprintf(bf,",D%lx\n",postdate);
    *size+= strlen(bf);
    fputs(bf,fp);
#ifdef PARENT_POINTERS
    if (parent > 0)
    {
    	sprintf(bf,",P%d\n",parent);
        *size+= strlen(bf);
        fputs(bf,fp);
    }
#endif /*PARENT_POINTER*/

#endif /*FPRINTF_RETURNS_SIZE */

    if (alt_text != NULL)
	*size+= write_text_block(fp,alt_text,1);

    if (text == NULL) return;

    /* Write text */
    fprintf(fp,",T\n");
    *size+= 3;
    *size+= write_text_block(fp,text,0);
    fprintf(fp,",E\n");
    *size+= 3;
    return;
}


/* POST_RESP - Append a response to an item and update the sum file.
 * Item text, text type, user identification, and erased or hidden status are
 * taken from the system dictionary.  Returns new response number or 0 on
 * failure, or -1 if "expect_resp" is set to a positive value that does not
 * match the number the new response would have gotten.
 */

int post_resp()
{
    FILE *ifp;
    int item,ifd,rn;
    int style;
    time_t filetime, clocktime= time(0L);
    struct stat st;
    size_t ioffset,size;
    char *new_text= NULL;
#ifdef HIDE_HTML
    char *plain_text;
#endif

    /* If text is HTML, sanitize it and possible generate plaintext version */
    style= (strcasecmp(sdict(VAR_TEXTTYPE),"text/html") == 0) ? 1 : 0;
    if (style == 1)
    {
    	/* Sanitize HTML */
	new_text= format_html(strdup(sdict(VAR_TEXT)),
	    (idict(VAR_YAPP_FORMAT)&1) ? " <!-- \n-->" : NULL, TRUE, FALSE);
    	if (new_text == NULL) die("HTML cleanup failed");
#ifdef HIDE_HTML
	/* Construct plain text version */
	plain_text= format_html2text(strdup(new_text));
	if (plain_text == NULL)
		die("conversion of HTML to plaintext failed");
#endif
    }
    else if (idict(VAR_YAPP_FORMAT)&1)
    {
	/* If posting a plain text response where webYapp can see it, we
	 * convert it to HTML, enclosing it in <TT> tags, ending each line
	 * with <BR><!-- and starting each following line with -->.  This is
	 * insane, but it makes things work in both web Yapp and Backtalk.
	 */
#ifdef HIDE_HTML
	plain_text= strdup(sdict(VAR_TEXT));
#endif
	style= 2;
	new_text= format(strdup(sdict(VAR_TEXT)), TRUE, FALSE, 79,
		"<TT>", "</TT>\n", "<BR><!-- \n-->");
    }
#ifdef HIDE_HTML
    else
	/* If we are sharing plain-text responses with command-line users,
	 * then we don't want meta-characters in plain text responses either.
	 */
	nometa(sdict(VAR_TEXT));
#endif

    /* Make sure no other item is open, to avoid deadlocking against ourself */
    closeitem();

    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	die("Anonymous user attempted to post response");

    item= idict(VAR_ITEM);
    sprintf(item_file,"%.900s/_%d",sdict(VAR_CONFDIR),item);

    /* Check that item is not frozen */
    if (stat(item_file,&st))
	return 0;
    if (!(st.st_mode & 0200))
	die("Item is frozen");

    /* Open and Lock the item file */
    if ((ifd= open(item_file,O_WRONLY|O_APPEND)) < 0)
	return 0;
    lock_exclusive(ifd,item_file);
    ifp= fdopen(ifd,"a");

    /* If "expect_resp" is postive, but not equal to next response, abort */
    if (idict(VAR_EXPECT_RESP) > 0)
    {
    	if (get_sum(item, &rn, NULL, NULL, NULL))
    	    die("item vanished while responding");
        if (idict(VAR_EXPECT_RESP) < rn)
        {
            fclose(ifp);
	    unlock(item_file);
            return -1;
        }
    }

    /* Write response */
#ifdef HIDE_HTML
    if (style != 0)
    {
	write_text(ifp, idict(VAR_UID), sdict(VAR_ID), sdict(VAR_ALIAS),
	    style, plain_text, (new_text == NULL) ? sdict(VAR_TEXT) : new_text,
	    clocktime, 0, idict(VAR_PARENTRESP), sdict(VAR_ATTACHMENTS),
	    &ioffset, &size);
	free(plain_text);
    }
    else
#endif
	write_text(ifp, idict(VAR_UID), sdict(VAR_ID), sdict(VAR_ALIAS),
	    style, (new_text == NULL) ? sdict(VAR_TEXT) : new_text, NULL,
	    clocktime, 0, idict(VAR_PARENTRESP), sdict(VAR_ATTACHMENTS),
	    &ioffset, &size);
    if (new_text != NULL) free(new_text);

    fflush(ifp);	/* Get it written out before we do the put_sum */

    /* Get file modification date */
    if (fstat(ifd,&st))
    {
    	fclose(ifp);
    	unlock(item_file);
        die("stat of item failed: %s",strerror(errno));
    }
    filetime= st.st_mtime;

    /* Check that it all got written out */
    if (st.st_size < ioffset+size)
    {
    	/* Chop off any incomplete response */
    	ftruncate(ifd, ioffset);

	fclose(ifp);
	unlock(item_file);
    	die("could not write response - disk full?");
    }

    /* Update Sum file (using time from stat) */
    rn= put_sum(item, -2, 0, 0, 0L, filetime);

    /* Release file */
    fclose(ifp);
    unlock(item_file);

    /* Update index file */
    index_new_resp(sdict(VAR_CONFDIR), item, rn-1, ioffset-2);

    /* Update Participation file (using time from stat) */
    update_item(item, -3, filetime, FALSE);
    pf_rfav(item);

    /* If logging, append to log file - resp_log_format defines format */
    if (sdict(VAR_POST_LOG_FILE)[0] != '\0')
    {
	push_integer(rn-1);
	macro_log(sdict(VAR_POST_LOG_FILE), "resp_log_format", 1, 0);
    }

    return rn-1;
}


/* WRITE_ITEM - Write out a new picospan-format item.  Item text, title,
 * user identification, and erased or hidden status are taken from the system
 * dictionary.  Dies on failure.  Returns offset of comma in ",R".
 */

static size_t write_item(FILE *fp, int style, char *text, char *alt_text,
	time_t intime, time_t *extime)
{
    struct stat st;
    size_t offset,size,hsize;
	
    /* Write item file */
    fprintf(fp,"!<ps02>\n");
    fprintf(fp,",H%s\n",sdict(VAR_TITLE));
    hsize= 11 + strlen(sdict(VAR_TITLE));
    write_text(fp,idict(VAR_UID),sdict(VAR_ID),sdict(VAR_ALIAS),
	       style,text,alt_text,intime,0,0,sdict(VAR_ATTACHMENTS),
	       &offset,&size);
    fflush(fp);

    /* Get file modification date */
    if (fstat(fileno(fp),&st))
    {
    	fclose(fp);
    	unlock(item_file);
        die("stat of item failed: %s",strerror(errno));
    }

    /* Check that it all got written out */
    if (st.st_size < size + hsize)
    {
    	/* Delete any incompletely posted item */
    	unlink(item_file);

    	fclose(fp);
    	unlock(item_file);
    	die("could not write item %s - disk full?",item_file);
    }

    /* Return last modification date of file */
    *extime= st.st_mtime;

    return offset;
}


/* POST_ITEM - Enter a new item and update the sum file.  Item text, title,
 * user identification, and erased or hidden status are taken from the system
 * dictionary.   The number of the item created is saved in the "item"
 * system variable.
 */

void post_item()
{
    extern int private_item;
    int item,ifd,style;
    FILE *ifp;
    time_t clocktime= time(0L), filetime;
    size_t offset;
    char *p;
    char *new_text= NULL;
#ifdef HIDE_HTML
    char *plain;
#endif
	
    /* Make sure no other item is open, just in case of deadlock potential */
    closeitem();

    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
	die("Anonymous user attempted to post item");

    /* Convert any newlines in title to spaces */
    while ((p= strchr(sdict(VAR_TITLE),'\n')) != NULL)
	*p= ' ';

    /* Response is plaintext if texttype is anything but "text/html" */
    style= (strcasecmp(sdict(VAR_TEXTTYPE),"text/html") == 0) ? 1 : 0;

    if (style == 1)
    {
        /* For HTML responses, always run them through format_html */
	new_text= format_html(strdup(sdict(VAR_TEXT)),
	    (idict(VAR_YAPP_FORMAT)&1) ? " <!-- \n-->" : NULL, TRUE, FALSE);
	if (new_text == NULL)
	    die("html cleanup failed");
#ifdef HIDE_HTML
        /* If we are running text compatible generate a text version */
	if ((plain= format_html2text(strdup(new_text))) == NULL)
	    die("html to plain text conversion failed");
#endif
    }
    else if (idict(VAR_YAPP_FORMAT)&1)
    {
	/* If posting a plain text response where webYapp can see it, we
	 * convert it to HTML, enclosing it in <TT> tags, ending each line
	 * with <BR><!-- and starting each following line with -->.  This is
	 * insane, but it makes things work in both web Yapp and Backtalk.
	 */
#ifdef HIDE_HTML
	plain= strdup(sdict(VAR_TEXT));
#endif
	style= 2;
	new_text= format(strdup(sdict(VAR_TEXT)), TRUE, FALSE, 79,
		"<TT>", "</TT>\n", "<BR><!-- \n-->");
    }
#ifdef HIDE_HTML
    else
	/* If we are sharing plain-text responses with command-line users,
	 * then we don't want meta-characters in plain text responses either
	 */
	nometa(sdict(VAR_TEXT));
#endif

    /* Find and create the next available item file */
    item= last_no() + 1;
    if (item == 1 && !bdict(VAR_AMFW))
	die("Only fairwitness can post first item");

    for (;;)
    {
	/* Try opening the standard item file first */
	sprintf(item_file,"%.900s/_%d",sdict(VAR_CONFDIR),item);
	ifd= open(item_file, O_WRONLY|O_CREAT|O_EXCL,
		private_item ? 0600 : 0644);
	if (ifd < 0 && errno != EEXIST)
	    die("Cannot create item file %s",item_file);
	if (ifd >= 0)
	{
		/* Was able to create item file */
		break;
	}
	item++;
    }
    lock_exclusive(ifd,item_file);
    ifp= fdopen(ifd,"w");

    /* Write item file */
#ifdef HIDE_HTML
    if (style != 0)
    {
    	offset= write_item(ifp,style,
	    plain, (new_text == NULL) ? sdict(VAR_TEXT) : new_text,
	    clocktime, &filetime);
    	free(plain);
    }
    else
#endif
        offset= write_item(ifp, style,
	    (new_text == NULL) ? sdict(VAR_TEXT) : new_text, NULL,
	    clocktime, &filetime);
    if (new_text != NULL) free(new_text);

    /* Write sumfile */
    (void)put_sum(item, 1, SF_ACTIVE, ~SF_ACTIVE, filetime, filetime);

    /* Release file */
    fclose(ifp);
    unlock(item_file);

    /* Update index file */
    index_new_resp(sdict(VAR_CONFDIR), item, 0, offset);

    /* Update Participation file */
    update_item(item, -1, filetime, FALSE);
    if (idict(VAR_IFAV) > 0)
	pf_item_seti(item, "fav", idict(VAR_IFAV));

    seti(VAR_ITEM,item);

    /* If logging, append to log file - item_log_format defines format */
    if (sdict(VAR_POST_LOG_FILE)[0] != '\0')
	macro_log(sdict(VAR_POST_LOG_FILE), "item_log_format", 0, 0);
}
