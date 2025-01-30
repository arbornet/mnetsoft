/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* PARTIPATION FILE reader/writer.
 */

#include "backtalk.h"
/* We need this for token types for hash tables.  Otherwise we'd just do
 * common.h */

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "user.h"
#include "str.h"
#include "sel.h"
#include "file.h"
#include "lock.h"
#include "partdir.h"
#include "set_uid.h"
#include "tag.h"
#include "partfile.h"

#define SECS_PER_DAY (60*60*24)

#ifdef DMALLOC
#include "dmalloc.h"
#endif


int loadpspart(struct pfc *part);
int loadbtpart(struct pfc *part, int ext);
void writepspart(struct pfc *part);
void writebtpart(struct pfc *part, int ext);


/* LOADPART -- Read in a user's participation file for the current conference
 * given directory name.  Create an empty one (in memory, so far) if he
 * doesn't have one.  If part->snooping is true, we don't create a
 * participation file.  Returns true if we created one instead of loading
 * it.
 *
 * The caller should set up the user, conf, and confdir fields before this
 * call, setting them to a value, or setting them to NULL.  A value for
 * confdir is needed if we are not snooping and if we may need to create
 * a new file and want to initialize it based on conf settings.  A value
 * for user and conf is needed if we are using partutil.  The values in those
 * fields need not remain valid after this call.  They will not be deallocated.
 */

int loadpart(struct pfc *part)
{
    char buf[BFSZ];
    int len;

    /* Initial values for various fields */
    part->ext_fp= NULL;
    part->conf_etc= NULL;
    part->part_date= -1L;
    part->last_part= NULL;

    /* Open, create, and lock the participation file */
    if (openpf(part))
    {
	/* No pf was found, openpf created an empty one.  Set up some data
	 * to put into it when we eventually write it out
	 */
	newpart(part, part->part_fp != NULL);
	return 1;
    }
    else if (part->part_fp == NULL)
    {
	part->part_modified= part->ext_modified= FALSE;
	part->alias= NULL;
	return 0;
    }

    /* Read and check magic code number */
    if (fgets(buf,BFSZ,part->part_fp) == NULL)
    {
    	/* Ignore empty file */
	newpart(part, FALSE);
	return 1;
    }

    if (!strcmp(buf,"!<pr03>\n"))
    {
#if DEFAULT_PART_TYPE == 0
	/* Picospan only - we never do this */
	part->part_type= 0;
        return loadpspart(part);
#else
	/* Picospan with possible extension file */
	part->part_type= 1;
	if (loadpspart(part)) return 1;
	/* Try loading an extension file */
	if (!openpfext(part)) loadbtpart(part,TRUE);
	return 0;
#endif
    }

    if (!strcmp(buf,"!<pr10>\n"))
    {
	part->part_type= 2;
        return loadbtpart(part,FALSE);
    }

    if ((len= strlen(buf)) < 8 &&
        (!strncmp(buf,"!<pr03>",len) || !strncmp(buf,"!<pr10>",len)) )
    {
	/* Ignore truncated participation file */
	newpart(part, FALSE);
	return 1;
    }

    /* Bomb on things that can't have been part files */
    die("Participation file %s has bad magic number", part->part_name);
}


/* LOADPSPART -- Read in a Picospan-style participation file.  The file's name
 * is already in the part->part_name, and it has already been opened on the
 * part_fp file pointer, and the first line has already been read in and
 * processed.  We just finish the job, returning 0 if we were able to read
 * the file, and 0 if we had to concoct a new one.
 */

int loadpspart(struct pfc *part)
{
    char buf[BFSZ], *t;
    struct pfe *npf,*lpf=NULL;

    /* Read alias name */
    if (fgets(buf,BFSZ,part->part_fp) == NULL)
    {
	/* EOF - Assume participation file was truncated */
	newpart(part, FALSE);
	return 1;
    }
    buf[BFSZ-2]= '\n'; buf[BFSZ-1]= '\0';
    if ((t= strchr(buf,'\n')) == NULL)
    {
	/* No terminating newline - Assume file was truncated */
	newpart(part, FALSE);
	return 1;
    }
    *t= '\0';
    part->alias= strdup(buf);

    part->pflist= NULL;
    while (fgets(buf,BFSZ,part->part_fp) != NULL)
    {
	if (index(buf,'\n') == NULL)
	    /* Discard lines with no terminating newline */
	    continue;

	npf= (struct pfe *)malloc(sizeof(struct pfe));
#ifdef YAPP_COMPAT
	if (sscanf(buf,"%d %d %lx",&npf->item_no,
				   &npf->resp_no,
				   (unsigned long *)&npf->date) != 3)
	    die("syntax error in participation file %s: %s",
		    part->part_name,buf);
	if (npf->forgotten= (npf->resp_no < 0))
	    npf->resp_no= -npf->resp_no;
#else
	/* Fancier scanning to handle case of "-0" responses */
	if (!isascii(buf[0]) || !isdigit(buf[0]))
	    die("syntax error in participation file %s: '%s'",
		part->part_name,buf);
	npf->item_no= atoi(buf);
	t= firstout(firstin(buf," ")," ");
	if (npf->forgotten= (*t == '-')) t++;
	if (sscanf(t,"%d %lx",&npf->resp_no,
			      (unsigned long *)&npf->date) != 2)
	    die("syntax error in participation file %s: %s",
		    part->part_name,buf);
#endif /* YAPP_COMPAT */

	npf->delete= 0;
	npf->favorite= 0;
	npf->my_title= NULL;
	npf->etc= NULL;
	npf->next= NULL;
	if (lpf == NULL)
	    part->pflist= npf;
	else
	    lpf->next= npf;
	lpf= npf;
    }

    part->part_modified= FALSE;
    part->ext_modified= FALSE;
    return 0;
}


/* NEW_ETC_INT - Store a new integer value in the given hashtable.  If the
 * value is zero, undefine it.
 */

void etc_int(HashTable **ht, char *tag, int value)
{
    int isnew;
    HashEntry *he;

    if (value == 0)
    {
	/* Undefine it */
	if (*ht == NULL) return;
	he= FindHashEntry(*ht, tag);
	if (he == NULL) return;
	DeleteHashEntry(he);
	return;
    }

    /* Create a hash table, if we didn't have one */
    if (*ht == NULL)
    {
	*ht= (HashTable *)malloc(sizeof(HashTable));
	InitHashTable(*ht);
    }
    /* Add or find the entry */
    he= CreateHashEntry(*ht, tag, &isnew);

    /* ...freeing any old value */
    if (!isnew) free_val(&(he->t));

    /* Store the new value */
    he->t.flag= TK_INTEGER;
    he->t.val= (void *)(long)value;
}


/* NEW_ETC_STR - Store a new string value in the given hashtable.
 */

void etc_str(HashTable **ht, char *tag, char *value, int copy)
{
    int isnew;
    HashEntry *he;

    /* Create a hash table, if we didn't have one */
    if (*ht == NULL)
    {
	*ht= (HashTable *)malloc(sizeof(HashTable));
	InitHashTable(*ht);
    }
    /* Add or find the entry */
    he= CreateHashEntry(*ht, tag, &isnew);

    /* ...freeing any old value */
    if (!isnew) free_val(&(he->t));

    /* Store the new value */
    if (copy) value= strdup(value);
    he->t.flag= TK_STRING|TKF_FREE;
    he->t.val= (void *)value;
}


/* FIND_PF - Get a pointer to the structure describing the participation
 * file entry for the given item in the current conference.  Returns 0
 * if an entry existed before this call.  Returns 1 if no entry existed,
 * and all entries have lower item numbers.  Returns 2 if no entry existed,
 * but there are entries with higher item numbers.  If create is true, and
 * entry is created if none existed before.  Should never be called if
 * no_part is true.
 * 
 * This does some optimization to work faster when items are searched for
 * in sequential order, which they usually are.
 */

int find_pf(struct pfc *part, int item, int create, struct pfe **pf)
{
    struct pfe *pfp, *pfn;
    int rc;
	
    /* Check if we found it on the last scan */
    if (part->last_part == NULL)
    {
	/* No previous scan - start scan from front of list */
	*pf= part->pflist;
    }
    else if (part->last_part->item_no == item)
    {
	/* Yup, no scan needed */
	*pf= part->last_part;
	return 0;
    }
    else if (part->last_part->item_no < item)
    {
	/* Nope, but item is after last one, so start scan from there */
	*pf= part->last_part;
    }
    else
    {
	/* Nope, old scan is useless - start from front of list */
	*pf= part->pflist;
    }

    /* walk the list, starting wherever */
    for (pfp= NULL;
	 *pf != NULL && (*pf)->item_no < item;
	 pfp= *pf, *pf= (*pf)->next)
    		;

    /* If we found it, get out of here */
    if (*pf != NULL && (*pf)->item_no == item)
    {
	part->last_part= *pf;
	return 0;
    }

    /* Item not in participation file... */
    rc= (*pf == NULL) ? 1 : 2;

    if (!create)
    {
	/* ...give it up */
	int rc= (*pf == NULL) ? 1 : 2;
	*pf= NULL;
	return rc;
    }

    /* ...put it in */
    pfn= (struct pfe *)malloc(sizeof(struct pfe));
    pfn->item_no= item;
    pfn->resp_no= 0;
    pfn->forgotten= 0;
    pfn->date= 0;
    pfn->favorite= 0;
    pfn->my_title= NULL;
    pfn->etc= NULL;
    pfn->delete= 0;
    pfn->next= *pf;

    if (pfp == NULL)
	part->pflist= pfn;
    else
	pfp->next= pfn;
    part->part_modified= TRUE;

    part->last_part= *pf= pfn;
    return rc;
}


/* LOADBTPART -- Read in a Backtalk-style participation file.  The file's name
 * is already in the part->part_name, and it has already been opened on the
 * part_fp file pointer, and the first line has already been read in and
 * processed.  We just finish the job, returning 0 if we were able to read
 * the file, and 0 if we had to concoct a new one.  If ext is true, load an
 * extension file instead, assuming we have already loaded the corresponding
 * Picospan-style participation file.
 */

int loadbtpart(struct pfc *part, int ext)
{
    char buf[BFSZ], *var, *val, *ch, *save;
    int save_size, save_i;
    long ival;
    struct pfe *npf= NULL, *lpf= NULL;
    int is_string, skip= FALSE;
    FILE *fp= ext ? part->ext_fp : part->part_fp;

    if (!ext)
    {
	part->alias= NULL;
	part->pflist= NULL;
    }

    /* Set up our growable buffer for reading in strings */
    save= (char *)malloc(BFSZ);
    save_size= BFSZ;

    while (fgets(buf,BFSZ,fp) != NULL)
    {
	/* Lines starting with numbers point to a new item number. */
    	if (isascii(buf[0]) && isdigit(buf[0]))
	{
	    if (ext)
	    {
		/* Loading extension - find existing pf entry for the item.
		 * If there is none, skip this entire item, don't create one.
		 */
		skip= find_pf(part, atoi(buf), FALSE, &npf);
	    }
	    else
	    {
		/* Loading standard file - create a the entry */
		npf= (struct pfe *)malloc(sizeof(struct pfe));

		npf->item_no= atoi(buf);
		npf->delete= 0;
		npf->favorite= 0;
		npf->forgotten= 0;
		npf->my_title= NULL;
		npf->resp_no= 0;
		npf->date= 0;
		npf->etc= NULL;
		npf->next= NULL;

		if (part->pflist == NULL)
		    part->pflist= npf;
		else
		    lpf->next= npf;

		lpf= npf;
		skip= FALSE;
	    }
	    continue;
	}
	else if (skip)
	    continue;

	/* Ignore blanks */
	if (buf[0] == '\n') continue;

	/* Find start of value */
	if ((val= strchr(buf,'=')) != NULL)
	{
	    *val= '\0';
	    *val++;
	    is_string= (val[0] == '(');
	}
	else
	{
	    char *t= strchr(buf,'\n');
	    if (t != NULL) *t= '\0';
	    is_string= 0;
	}

	/* Save variable name */
	var= strdup(buf);

	if (is_string)
	{
	    /* Read string values - can run across multiple lines */
	    int esc= 0;
	    save_i= 0;
	    ch= val+1;
	    for (;;)
	    {
	    	if (ch == '\0')
		{
		    /* Long line broken by fgets or truncated file */

		    if (fgets(buf,BFSZ,fp) == NULL)
			break;  /* truncated - stop */
		    
		    ch= buf;
		}

		if (*ch == ')' && !esc)
		    break;	/* Normal end of string */
		
		if (esc= (*ch == '\\' && !esc))
		{
		    ch++;
		    continue;
		}

		if (save_i+1 >= save_size)
		{
		    save_size+= BFSZ;
		    save= (char *)realloc(save, save_size);
		}
		save[save_i++]= *(ch++);
	    }
	    save[save_i]= '\0';
	}
	else if (val == NULL)
	    /* Boolean value */
	    ival= 1;
	else if (isascii(val[0]) && (isdigit(val[0]) || val[0] == '-'))
	    /* Integer value */
	    ival= atol(val);
	else
	    die("syntax error in participation file %s: %s",
		    part->part_name,buf);

	/* Handle special fields */
	if (npf == NULL)
	{
	    if (!strcmp(var,"alias"))
	    {
		if (is_string)
		    part->alias= strdup(save);
	    }
	    else if (is_string)
		etc_str(&part->conf_etc,var,save,TRUE);
	    else
		etc_int(&part->conf_etc,var,ival);
	}
	else
	{
	    if (!strcmp(var,"r"))
	    {
	    	/* This gets much fancier in the future */
		if (!is_string) npf->resp_no= ival;
	    }
	    else if (!strcmp(var,"d"))
	    {
		if (!is_string) npf->date= (time_t)ival;
	    }
	    else if (!strcmp(var,"forgot"))
	    {
		if (!is_string) npf->forgotten= ival;
	    }
	    else if (!strcmp(var,"fav"))
	    {
		if (!is_string) npf->favorite= ival;
	    }
	    else if (!strcmp(var,"title"))
	    {
		if (is_string) npf->my_title= strdup(save);
	    }
	    else if (is_string)
		etc_str(&(npf->etc),var,save,TRUE);
	    else
		etc_int(&(npf->etc),var,ival);
	}
	free(var);
    }

    part->part_modified= FALSE;
    part->ext_modified= FALSE;
    free(save);
    return 0;
}


/* GET_SETTINGS - This loads certain settings from the conference
 * settings file.  It returns:
 *
 *      new_isel   - item selector for items which should be new on
 *                   first joining.  Defaults to "0,$" if not defined.
 *                   Should point to a BFSZ buffer.
 *
 *      new_since  - if positive, items that have had new responses in
 *                   this number of days should be new on first joining.
 *                   Defaults to 0 if not defined.
 *
 * Any other tags in the conference settings file are ignored.
 */

void get_settings(char *confdir, char *new_isel, int *new_since)
{
    rtag *rt;
    char path[BFSZ];
    char *var, *val;
    int rc;

    /* Set defaults */
    strcpy(new_isel,"1,$");
    *new_since= 0;

    sprintf(path,"%s/settings",confdir);
    if ((rt= open_tag(path,TRUE)) == NULL) return;

    while ((rc= get_tag(rt,&var,&val)) > 0)
    {
	if (!strcmp(var, "new_isel"))
	{
	    if (rc == 1 || rc == 3)
		strcpy(new_isel,val);
	}
	else if (!strcmp(var, "new_since"))
	{
	    if (rc == 2 || rc == 4)
		*new_since= atoi(val);
	}
    }

    close_tag(rt);
}


/* NEWPART -- Create a new participation file for the conference in memory.
 * If the "newuser" flag is true, we construct a participation file with some
 * unseen items and some new items, as determined by the "new_isel" and
 * "new_since" settings in the conference settings file.  Otherwise, we
 * just make all items new.
 */

void newpart(struct pfc *part, int newuser)
{
    part->pflist= NULL;
    part->last_part= NULL;

    /* Set alias name */
    part->alias= strdup(user_fname());

    part->part_modified= newuser;
    part->ext_modified= FALSE;
    part->part_type= DEFAULT_PART_TYPE;
    part->conf_etc= NULL;
    part->last_part= NULL;

    if (newuser && part->confdir != NULL)
    {
    	char new_isel[BFSZ+1];
    	int new_since, item, nresp, flags;
    	time_t idate, rdate, sdate, now;
	struct pfe *pfn, *pfp;

	part->part_date= 0L;

	/* Get instructions for which should be new from the settings file  */
	get_settings(part->confdir,new_isel,&new_since);

	/* Everything is new - do nothing */
	if (!strcmp(new_isel,"1-$")) return;

	now= time(0L);
	sdate= now - new_since * SECS_PER_DAY;

	pfn= pfp= NULL;
	for (seek_item(1); !next_item(&item,&nresp,&flags,&idate,&rdate); )
	{
	    pfn= (struct pfe *)malloc(sizeof(struct pfe));
	    pfn->item_no= item;
	    pfn->delete= 0;
	    pfn->forgotten= 0;
	    pfn->favorite= 0;
	    pfn->resp_no= 0;
	    pfn->my_title= NULL;
	    pfn->etc= NULL;

	    if ((new_since > 0 && rdate >= sdate) || in_sel(item,new_isel))
		pfn->date= 0;	/* Item New */
	    else
		pfn->date= now;	/* Item Unseen */

	    pfn->next= NULL;
	    if (pfp == NULL)
	    	part->pflist= pfn;
	    else
		pfp->next= pfn;
	    pfp= pfn;
	}

	/* Special handling for last item and $ sign in isels */
	if (pfn != NULL && pfn->date != 0 && strchr(new_isel,'$') != NULL)
	    pfn->date= 0;
    }
}


/* WRITEPART -- Write out the participation file.  A loadpart() or newpart()
 * call must preceed this.  This deallocates the memory that it was stored
 * in as a side effect.
 */

void writepart(struct pfc *part)
{
    struct pfe *pf, *npf;

    if (part == NULL) return;

#ifdef CONVERT_PART
#if DEFAULT_PART_TYPE != 1
    if (part->part_type == 1 && part->ext_fp != NULL)
    {
	/* If converting from a format with extension file, remove it */
	int pnlen= strlen(part->part_name);
	part->part_name[pnlen]= 'x';
	part->part_name[pnlen+1]= '\0';
	unlink(part->part_name);
	part->part_name[pnlen]= '\0';
	fclose(part->ext_fp);
	part->ext_fp= NULL;
    }
#endif
    part->part_type= DEFAULT_PART_TYPE;
#endif

    if (part->part_type == 0)
	writepspart(part);
    else if (part->part_type == 1)
    {
	if (part->ext_modified)
	{
	    if (part->ext_fp == NULL)
		openpfext(part);
	    else
		rewind(part->ext_fp);
	    if (part->ext_fp != NULL)
		writebtpart(part, TRUE);
	}
	if (part->part_modified)
	    writepspart(part);
	else
	    freepart(part);
    }
    else
	writebtpart(part, FALSE);
}


/* WRITEPSPART -- Write out a PicoSpan-style participation file.  This
 * deallocates the memory that it was stored in as a side effect.
 */

void writepspart(struct pfc *part)
{
    struct pfe *pf, *npf;
    rewind(part->part_fp);

    fputs("!<pr03>\n",part->part_fp);
    fputs(part->alias,part->part_fp);
    fputc('\n',part->part_fp);
    free(part->alias);

    if (part->conf_etc != NULL)
    {
	DeleteHashTable(part->conf_etc);
	free(part->conf_etc);
	part->conf_etc= NULL;
    }

    for (pf= part->pflist; pf != NULL; pf= npf)
    {
	if (!pf->delete)
	{
#ifdef YAPP_COMPAT
	    fprintf(part->part_fp,"%d %d %lX\n",
		    pf->item_no,
		    pf->resp_no * (pf->forgotten ? -1 : 1),
		    (unsigned long)pf->date);
#else
	    /* This version allows a "-0" to be printed, which Picospan
	     * understands just fine, but which Yapp considers freakish.
	     */
	    fprintf(part->part_fp,
		    pf->forgotten ? "%d -%d %lX\n" : "%d %d %lX\n",
		    pf->item_no,
		    pf->resp_no,
		    (unsigned long)pf->date);
#endif /* YAPP_COMPAT */
	    if (pf->my_title != NULL)
		free(pf->my_title);
	    if (pf->etc != NULL)
	    {
		DeleteHashTable(pf->etc);
		free(pf->etc);
	    }
	}
	npf= pf->next;
	free(pf);
    }

    /* We need this here because the file can get smaller, and we can't
     * destroy and recreate it because we'd lose our lock.
     */

    ftruncate(fileno(part->part_fp),(size_t)ftell(part->part_fp));
}


/* WRITEBTPART -- Write out a Backtalk-style participation file.  This
 * deallocates the memory that it was stored in as a side effect.
 *
 * If ext is true, then write an extenstion file instead, and do not deallocate
 * anything.
 */

void writebtpart(struct pfc *part, int ext)
{
    HashEntry *h;
    HashSearch hs;
    struct pfe *pf,*npf;
    FILE *fp= ext ? part->ext_fp : part->part_fp;

    rewind(fp);

    if (ext)
	fputs("!<px01>\n",fp);
    else
    {
	fputs("!<pr10>\n",fp);
	wrtstrtag(fp, "alias", part->alias, FALSE);
	free(part->alias);
    }

    if (part->conf_etc != NULL)
    {
	for (h= FirstHashEntry(part->conf_etc, &hs);
		h != NULL;
		h= NextHashEntry(&hs))
	    wrttag(fp, h->key, &(h->t));
	if (!ext)
	{
	    DeleteHashTable(part->conf_etc);
	    free(part->conf_etc);
	    part->conf_etc= NULL;
	}
    }

    for (pf= part->pflist; pf != NULL; pf= npf)
    {
	if (!pf->delete)
	{
	    if (ext)
	    {
		if (pf->favorite || pf->my_title != NULL || pf->etc != NULL)
		    fprintf(fp,"%d\n",pf->item_no);
	    }
	    else
	    {
		fprintf(fp,"%d\nr=%d\nd=%ld\n",
			pf->item_no,
			pf->resp_no,
			(unsigned long)pf->date);
		if (pf->forgotten) fputs("forgot\n",fp);
	    }
	    if (pf->favorite == 1)
		fputs("fav\n",fp);
	    else if (pf->favorite)
		wrtinttag(fp, "fav", pf->favorite, FALSE);
	    if (pf->my_title != NULL)
	    {
		wrtstrtag(fp, "title", pf->my_title, FALSE);
		if (!ext) free(pf->my_title);
	    }
	    if (pf->etc != NULL)
	    {
		for (h= FirstHashEntry(pf->etc, &hs); h != NULL;
		     h= NextHashEntry(&hs))
		    wrttag(fp, h->key, &(h->t));
		if (!ext)
		{
		    DeleteHashTable(pf->etc);
		    free(pf->etc);
		}
	    }
	}
	npf= pf->next;
	if (!ext) free(pf);
    }

    /* We need this here because the file can get smaller, and we can't
     * destroy and recreate it because we'd lose our lock.
     */

    ftruncate(fileno(fp),(size_t)ftell(fp));
}


/* FREEPART -- Deallocate the memory we have the participation file stored
 * in.  Note that freepart() does NOT deallocate the pfc structure itself,
 * only the stuff it points to.
 */

void freepart(struct pfc *part)
{
    struct pfe *pf, *npf;

    if (part->alias != NULL) free(part->alias);

    if (part->conf_etc != NULL)
    {
	DeleteHashTable(part->conf_etc);
	free(part->conf_etc);
	part->conf_etc= NULL;
    }

    for (pf= part->pflist; pf != NULL; pf= npf)
    {
	if (pf->my_title != NULL)
	    free(pf->my_title);
	if (pf->etc != NULL)
	{
	    DeleteHashTable(pf->etc);
	    free(pf->etc);
	}
	npf= pf->next;
	free(pf);
    }
}


/* HOMEPARTPATH -- Construct the full path name of the participation file,
 * given the file name.  Save the result in the passed-in buffer.
 * Returns a pointer to "path" -- Normal Version, but also used with Yapp 3.0
 * to find .cflist files.
 */

char *homepartpath(char *path, char *dir, char *name)
{
    int homelen;

    strcpy(path,dir);
    homelen= strlen(path);
#ifdef ALLOW_CFDIR
    strcpy(path+homelen,"/.cfdir/");
    if (access(path,0))
	strcpy(path+homelen+1,name);
    else
	strcat(path+homelen,name);
#else
    path[homelen]= '/';
    strcpy(path+homelen+1,name);
#endif /* ALLOW_CFDIR */
    return(path);
}


/* OPENPF - Open the current participation file and store the filepointer in
 * the part_fp field of the given structure.  If there was no previous
 * participation file return 1, otherwise return 0.  If part->snooping is
 * true, never create the file.
 */

int openpf(struct pfc *part)
{
    /* If we are in direct_exec mode, all this must be done as the user */
    as_user();

    /* Try opening an existing particiption file */
    part->part_fp= fopen(part->part_name, part->snooping ? "r" : "r+");

#ifdef PART_DIR
    if (!part->snooping && part->part_fp == NULL && errno == ENOENT)
    {
	/* User directory missing - create it */
	mkpartdir(part->part_name);
	part->part_fp= fopen(part->part_name, "r+");
    }
#else

#ifdef PARTUTIL
    /* If we fail because of permissions, try using partutil to fix them */
    if (part->part_fp == NULL && errno == EACCES && part->conf != NULL &&
	    part->user != NULL)
    {
	partutil("-p", part->conf, part->user);
	part->part_fp= fopen(part->part_name, part->snooping ? "r" : "r+");
    }
#endif /* PARTUTIL */
#endif /* !PART_DIR */

    /* If we opened file, lock it and return */
    if (part->part_fp != NULL)
    {
	lock_exclusive(fileno(part->part_fp),part->part_name);
	as_cfadm();
	return 0;
    }

    if (part->snooping)
    {
	as_cfadm();
	return 1;
    }

    /* If file exists but isn't readable then don't try to create it and
     * just run without one.
     */
    if (errno == EACCES)
    {
	as_cfadm();
	return 1;
    }

    /* File doesn't exist - create one - just an empty file at this point */
#ifdef PARTUTIL
    if (part->conf != NULL && part->user != NULL)
	partutil("-c", part->conf, part->user);
#endif
    part->part_fp= fopen(part->part_name,"w+");
    if (part->part_fp != NULL)
        lock_exclusive(fileno(part->part_fp),part->part_name);
    as_cfadm();
    return 1;
}


/* CLOSEPF - If the part_fp or ext_fp fields of the given pfc structure are
 * non-null, close them.
 */

void closepf(struct pfc *part)
{
    /* Locking is only on main file, so close any extension first */
    if (part->ext_fp != NULL)
    {
	fclose(part->ext_fp);
	part->ext_fp= NULL;
    }

    /* Close the real participation file */
    if (part->part_fp != NULL)
    {
	fclose(part->part_fp);
	part->part_fp= NULL;
	unlock(part->part_name);
    }
}


/* OPENPFEXT - Open the extension file to the current participation file and
 * store the filepointer in the ext_fp global variable.  If there was no
 * previous participation file return 1, otherwise return 0.  If part->snooping
 * is true, never create the file.  The main participation file should already
 * be open.
 */

int openpfext(struct pfc *part)
{
    int pnlen= strlen(part->part_name);

    /* Append an x to partname */
    part->part_name[pnlen]= 'x';
    part->part_name[pnlen+1]= '\0';

    /* If we are in direct_exec mode, all this must be done as the user */
    as_user();

    /* Try opening an existing extension file */
    part->ext_fp= fopen(part->part_name, part->snooping ? "r" : "r+");

#ifdef PARTUTIL
    /* If we fail because of permissions, try using partutil to fix them */
    if (part->ext_fp == NULL && errno == EACCES && part->conf != NULL &&
	    part->user != NULL)
    {
	partutil("-P", part->conf, part->user);
	part->ext_fp= fopen(part->part_name, part->snooping ? "r" : "r+");
    }
#endif /* PARTUTIL */

    /* If we opened file, return (don't lock) */
    if (part->ext_fp != NULL || part->snooping)
    {
	if (part->ext_fp != NULL)
	{
	    char buf[BFSZ];
	    /* If the file is empty, go ahead and use it, but if it has
	     * something other than the normal header, decline to use it.
	     * */
	    if (fgets(buf,BFSZ,part->ext_fp) != NULL &&
		 strcmp(buf,"!<px01>\n"))
	    {
		/* weird file - close it */
		fclose(part->ext_fp);
		part->ext_fp= NULL;
	    }
	}
        part->part_name[pnlen]= '\0';
	as_cfadm();
	return (part->ext_fp == NULL);
    }

    /* If file exists but isn't readable then don't try to create it and
     * just run without one.
     */
    if (errno == EACCES)
    {
	part->part_name[pnlen]= '\0';
	as_cfadm();
	return 1;
    }

    /* File doesn't exist - create one */
#ifdef PARTUTIL
    if (part->conf != NULL && part->user != NULL)
	partutil("-C", part->conf, part->user);
#endif
    part->ext_fp= fopen(part->part_name,"w+");

    part->part_name[pnlen]= '\0';
    as_cfadm();
    return 1;
}


#ifdef PARTUTIL

#include "waittype.h"

/* PARTUTIL - execute the partutil program to create, permit, or destory
 * user's participation file for a conference.  Flag should be "-c" to
 * create, "-p" to permit, or "-d" to destroy a participation file.  A flag
 * of "-l" permits (or creates, if necessary) a .cflist for the user.  In
 * this case conf should be NULL.
 */

int partutil(char *flag, char *conf, char *user)
{
    pid_t cpid,wpid;
    waittype status;

    /* If we are executing directly, we don't need partutil */
    if (direct_exec) return 0;

    if ((cpid= fork()) == 0)
    {
	/* Child process: do the exec */
	if (conf == NULL)
	    execl(PARTUTIL,"partutil",flag,user,(char *)NULL);
	else
	    execl(PARTUTIL,"partutil",flag,conf,user,(char *)NULL);
	exit(127);
    }

    while ((wpid= wait(&status)) != -1 && wpid != cpid)
	;

    return (WIFEXITED(status)) ? WEXITSTATUS(status) : 127;
}
#endif /* PARTUTIL */
