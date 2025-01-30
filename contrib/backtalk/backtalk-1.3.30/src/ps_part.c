/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Higher level participation file management routines.
 */

#include "backtalk.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "sysdict.h"
#include "user.h"
#include "dict.h"
#include "str.h"
#include "sel.h"
#include "file.h"
#include "lock.h"
#include "partdir.h"
#include "set_uid.h"
#include "partfile.h"
#include "ps_part.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#define INT_MAX 32767
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* These variables describe the state of the particification file for the
 * current conference.  Curr points to the current participation file
 * structure if there isn't one.  If no_part is true, then we shouldn't
 * attempt to open a participation file for the current conference - either
 * because the participation file has already been found to be unreadable,
 * or because the user is anonymous.
 */

struct pfc *curr= NULL;
int no_part= FALSE;


/* CLOSEPART -- Close the currently open participation file if there is one,
 * writing it out if it has been modified.  If any item has been modified,
 * it is very important that this be called to write the pf out.
 */

void closepart()
{
    no_part= FALSE;

    if (curr == NULL) return;

    if ((curr->part_modified || curr->ext_modified) && !curr->snooping)
	writepart(curr);
    else
	freepart(curr);

    closepf(curr);

    free(curr);
    curr= NULL;
}


/* REMOVEPART -- Close and delete the participation file of the currently open
 * conference.
 */

void removepart()
{
    int had_ext= (curr->ext_fp != NULL);

    no_part= FALSE;

    if (curr == NULL) return;

    closepf(curr);

    if (!curr->snooping)
    {
#if defined(PARTUTIL) && !defined(PART_DIR)
	partutil("-d",sdict(VAR_CONF),sdict(VAR_ID));
	if (had_ext)
	    partutil("-D",sdict(VAR_CONF),sdict(VAR_ID));
#else
	as_user();
	unlink(curr->part_name);
	as_cfadm();

	if (had_ext)
	{
	    int pnlen= strlen(curr->part_name);
	    curr->part_name[pnlen]= 'x';
	    curr->part_name[pnlen+1]= '\0';
	    unlink(curr->part_name);
	    curr->part_name[pnlen]= '\0';
	}
#endif
    }

    freepart(curr);
    free(curr);
    curr= NULL;
}


/* OPENPART - Make sure the participation file for the current conference has
 * been loaded in memory.  If the argument is true, we will open it in "snoop"
 * mode, in which participation files are read-only.  You don't normally have
 * to call this unless you want to snoop, because calling most any routine
 * that does anything to a participation file does call this.  Returns true if
 * a new participation file was created.
 */

int openpart(int snoop)
{
    int rc;

    if (curr != NULL || no_part) return 0;

    /* Don't load if user is anonymous */
    no_part= (sdict(VAR_ID)[0] == '\0' || sdict(VAR_HOMEDIR)[0] == '\0');
    if (no_part) return 0;

    curr= (struct pfc *)malloc(sizeof(struct pfc));
    curr->snooping= snoop;

    /* Find the participation file */
#ifdef PART_DIR
    bbspartpath(curr->part_name,sdict(VAR_ID),sdict(VAR_PARTICIP));
#else
    homepartpath(curr->part_name,sdict(VAR_HOMEDIR),sdict(VAR_PARTICIP));
#endif

    /* Load it */
    curr->user= sdict(VAR_ID);
    curr->conf= sdict(VAR_CONF);
    curr->confdir= sdict(VAR_CONFDIR);
    rc= loadpart(curr);
    curr->user= curr->conf= curr->confdir= NULL;

    /* Save alias in dictionary */
    sets(VAR_ALIAS, (curr->alias != NULL) ? curr->alias : "", SM_COPY);

    return rc;
}


/* LAST_READ -- Return the number of the last response to item that the user
 * has read.  Return resp_no = -1 if it is unread.  The flag forgotten is
 * set to true if the item has been forgotten.
 */

void last_read(int item, int *resp_no, int *forgotten, time_t *date)
{
    struct pfe *pf;
	
    /* Set default values */
    *forgotten= FALSE;
    *resp_no= -1;
    *date= 0;

    if (curr == NULL) return;

    if (find_pf(curr, item, FALSE, &pf))
	/* No such item */
	return;

    /* found item */
    *forgotten= pf->forgotten;
    *resp_no= pf->resp_no-1;
    *date= pf->date;
}


/* UPDATE_ITEM -- Mark an item read up through the given response.  If the
 * response is -1, the entire item is unseen.  If the response is -2, forget
 * the item.  If response is -3, don't update the response number -- only
 * update the time.  If the response is -4, remember (unforget) the item.
 * If the item is forgotten, marking it seen does not unforget it.
 * This version is fancied up so that multiple calls on the same item in
 * rapid succession can be handled efficiently.  Unless rewind is set, the
 * this will only increase the number of responses seen.  If resp is -1, it
 * will not be changed.
 */

void update_item(int item, int resp, time_t now, int rewind)
{
    struct pfe *pf;

    if (curr == NULL) return;

    if (find_pf(curr, item, TRUE, &pf))
    {
	/* A newly created participation file entry */
	if (resp == -2)
	{
	    /* forget an unseen item */
#ifdef YAPP_COMPAT
	    pf->resp_no= 1;
#else
	    pf->resp_no= 0;
#endif
	    pf->forgotten= 1;
	    pf->date= 0;
	}
	else
	{
	    /* See the item */
	    pf->resp_no= (resp < 0) ? 0 : resp+1;
	    pf->forgotten= 0;
	    pf->date= now;
	}
	curr->part_modified= TRUE;
    }
    else
    {
	/* Update existing entry */
	if (resp == -2 || resp == -4)
	{
	    /* Remembering and Forgetting */
	    if ((resp == -4) == (pf->forgotten)) 
	    {
#ifdef YAPP_COMPAT
		/* Yapp participation file format can't handle forgetting
		 * unread items so mark the first response read in that case.
		 */
		if (pf->resp_no == 0) pf->resp_no= 1;
#endif
		pf->forgotten= !pf->forgotten;
		curr->part_modified= TRUE;
		/* If we are forgetting, automatically disfavor the item */
		if (resp == -2 || pf->favorite)
		{
		    pf->favorite= 0;
		    curr->ext_modified= TRUE;
		}
	    }
	}
	else
	{
	    /* Plain old looking at things */
	    if (resp != -3 && (rewind || pf->resp_no <= resp))
	    {
		pf->resp_no= resp+1;
		/* Disfavor temporary favorites */
		if (pf->favorite == 2)
		{
		    pf->favorite= 0;
		    curr->ext_modified= TRUE;
		}
	    }
	    pf->date= now;
	    curr->part_modified= TRUE;
	}
    }
}


/* PF_CONF_SETI - Set one of the the conference-wide, post-Picospan fields in
 * the participation file to an integer value.  This is the main entry point
 * for other modules to call if setting a value.
 */

void pf_conf_seti(char *tag, int value)
{
    if (curr == NULL) return;
    etc_int(&(curr->conf_etc), tag, value);
    curr->ext_modified= TRUE;
}


/* PF_CONF_SETS - Set one of the the conference-wide, post-Picospan fields in
 * the participation file to an string value.  This is the main entry point
 * for other modules to call if setting a value.
 */

void pf_conf_sets(char *tag, char *value, int copy)
{
    if (curr == NULL)
    {
	if (!copy) free(value);
	return;
    }

    etc_str(&(curr->conf_etc), tag, value, copy);
    curr->ext_modified= TRUE;
}


/* PF_ITEM_SETI - Set one of the post-Picospan fields in the participation file
 * entry for an item to an integer value.
 */

void pf_item_seti(int item, char *tag, int value)
{
    struct pfe *pf;

    if (curr == NULL) return;

    find_pf(curr, item, TRUE, &pf);

    if (!strcmp(tag,"fav"))
	pf->favorite= value;
    else
	etc_int(&(pf->etc), tag, value);

    curr->ext_modified= TRUE;
}


/* PF_ITEM_SETS - Set one of the post-Picospan fields in the participation file
 * entry for an item to a string value.  If 'copy' is false, the string is
 * not already in malloced memory.
 */

void pf_item_sets(int item, char *tag, char *value, int copy)
{
    struct pfe *pf;

    if (curr == NULL)
    {
	if (!copy) free(value);
	return;
    }

    find_pf(curr, item, TRUE, &pf);

    if (!strcmp(tag,"title"))
	pf->my_title= value;
    else
	etc_str(&(pf->etc), tag, value, copy);

    curr->ext_modified= TRUE;
}

/* PF_CONF_GET - Get a value from the header.  Returns true if not defined.
 */

int pf_conf_get(char *tag, Token *val)
{
    if (curr == NULL) return 1;

    if (!strcmp(tag,"alias"))
    {
    	val->val= (void *)strdup(curr->alias);
	val->flag= TK_STRING | TKF_FREE;
	return 0;
    }
    else if (curr->conf_etc == NULL)
	return 1;
    else
    {
	HashEntry *h= FindHashEntry(curr->conf_etc, tag);
	if (h == NULL)
	    return 1;
	*val= h->t;
	uniquify_token(val);
	return 0;
    }
}


/* PF_ITEM_GET - Get a value from an item in the participation file.  Returns
 * true if not defined.
 */

int pf_item_get(int item, char *tag, Token *val)
{
    struct pfe *pf;

    if (curr == NULL) return 1;

    find_pf(curr, item, FALSE, &pf);
    if (pf == NULL) return 1;

    if (!strcmp(tag,"title"))
    {
    	val->val= (void *)strdup(pf->my_title);
	val->flag= TK_STRING | TKF_FREE;
	return 0;
    }
    else if (!strcmp(tag,"fav"))
    {
    	val->val= (void *)(long)pf->favorite;
	val->flag= TK_INTEGER;
	return 0;
    }
    else if (!strcmp(tag,"r"))
    {
    	val->val= (void *)(long)pf->resp_no;
	val->flag= TK_INTEGER;
	return 0;
    }
    else if (!strcmp(tag,"d"))
    {
    	val->val= (void *)(long)pf->date;
	val->flag= TK_TIME;
	return 0;
    }
    else if (!strcmp(tag,"forgot"))
    {
    	val->val= (void *)(long)pf->forgotten;
	val->flag= TK_INTEGER;
	return 0;
    }
    else if (pf->etc == NULL)
	return 1;
    else
    {
	HashEntry *h= FindHashEntry(pf->etc, tag);
	if (h == NULL)
	    return 1;
	*val= h->t;
	uniquify_token(val);
	return 0;
    }
}

/* PF_RFAV -- update an item's "fav" note based on the current setting of the
 * rfav flag.
 */

void pf_rfav(int item)
{
    struct pfe *pf;

    if (curr == NULL) return;

    switch (idict(VAR_RFAV))
    {
    case 0:
	break;

    case 1:
	find_pf(curr, item, TRUE, &pf);
	pf->favorite= 1;
	curr->ext_modified= TRUE;
	break;
	
    case 2:
	find_pf(curr, item, TRUE, &pf);
	if (pf->favorite != 1)
	{
	    curr->ext_modified= TRUE;
	    pf->favorite= 2;
	}
	break;
    }
}


/* LOST_ITEMS -- The items between first and last are missing.  Delete them from
 * the partipation file.  Really just mark them for deletion.  If last is -1,
 * then delete through the end of the conference.
 */

void lost_items(int first,int last)
{
    struct pfe *pf;
    int state=1;

    if (curr == NULL || last < first) return;

    if (last == -1) last= INT_MAX;
	
    for (pf= curr->pflist; pf != NULL; pf= pf->next)
    {
	if (state == 1 && pf->item_no >= first)
	    state= 2;
		
	if (state == 2)
	{
	    if (pf->item_no > last) break;

#if 0
	    /* If an item was deleted by Picospan, its index file
	     * may still be there.  But the following test means
	     * every user whoever saw the item before it was
	     * deleted would try to delete the index file once.
	     * That's a lot of overhead.  A cron script cleaning
	     * up leftover index files is a better solution.
	     */
	    if (!pf->delete)
		remove_index(sdict(VAR_CONFDIR),pf->item_no);
#endif
	    pf->delete= 1;
	}
    }
}


/* SET_CONF_ALIAS - Change the default full name for the user for the
 * currently open conference.  "newname" should be malloc'ed memory.
 * Aliases with newlines are clipped at the newline.
 */

void set_conf_alias(char *newname)
{
    char *p;
    if (curr == NULL) return;
    if ((p= strchr(newname,'\n')) != NULL) *p= '\0';
    if (curr->alias != NULL) free(curr->alias);
    curr->alias= newname;
    curr->part_modified= TRUE;
}

/* PARTDATE - Return the last modification date of the currently conference's
 * participation file.  If it didn't exist before this session, return 0.
 */

time_t partdate()
{
    struct stat st;
    if (curr == NULL) return 0L;
    if (curr->part_date == -1L)
	curr->part_date= stat(curr->part_name,&st) ? 0L : st.st_mtime;
    return curr->part_date;
}


/* FAV_SEL -- Given an item selector, split it into selectors for favorite,
 * brandnew, and newresp items based on participation file info.  Returns
 * up to three pointers to strings in malloced memory.  Pointers may be NULL.
 * if there is nothing in a category.
 *    0 - no-op - just return what was passed in.
 *    1 - return favorites and others
 *    2 - return brandnew and newresp
 *    3 - return favorites, non-favorite brandnew, and non-favorite newresp.
 * If you call it with flag=0, you just get the copy of the original selector,
 * but you don't want to do that, because it is slow.  Function returns a
 * count of things returned.  Passed in string is mangled.
 */

#define FF_LASTITEM 2147483647

long ff_inum(char *s)
{
    return (*s == '$') ? FF_LASTITEM : atol(s);
}

struct ff_tmp {
    char *sel;		/* A selector constructed so far */
    int sz;		/* Amount of memory allocated for sel */
    int i;		/* Our current index in sel */
    long first;		/* Begining of current range */
    long last;		/* End of current range - negative if none */
};


/* Append an item to the selector.  Call with item=-1 to wrap up.  */

void ff_app(struct ff_tmp *s, long item)
{
    /* Never been called before - initialize */
    if (s->sel == NULL)
    {
	if (item == -1) return;
	s->sel= (char *)malloc(s->sz= BFSZ);
	s->i= 0;
	s->first= s->last= item;
	return;
    }

    /* Check if we are extending the current range */
    if (s->last > 0 && (item == s->last + 1 || item == FF_LASTITEM))
    {
	s->last= item;
	return;
    }

    if (s->last == -1)
    {
	s->first= s->last= item;
	return;
    }

    /* Make sure we have some memory */
    if (s->sz - s->i <= 25)
	s->sel= (char *)realloc(s->sel, s->sz+= BFSZ);

    /* Write out a completed range */
    if (s->last > 0)
    {
	if (s->i > 0) s->sel[(s->i)++]= ',';
	if (s->first == s->last)
	    sprintf(s->sel + s->i, "%ld", s->first);
	else if (s->last == FF_LASTITEM)
	    sprintf(s->sel + s->i, "%ld-$", s->first);
	else
	    sprintf(s->sel + s->i, "%ld-%ld", s->first, s->last);
	s->i+= strlen(s->sel + s->i);
    }

    /* Start a new range */
    s->first= s->last= item;
}


int fav_sel(char *isel, int flag, char **is1, char **is2, char **is3)
{
    char *next_range, *dash;
    int reverse, rc, favi,newi,otheri, len;
    long i,i1,i2;
    struct pfe *pf;
    struct ff_tmp fav, new, other;
    struct ff_tmp *pnew= (flag & 2) ? &new : &other;

    /* Flag zero: just return what we were give */
    if (flag == 0) {*is1= strdup(isel); return 1;}

    /* Empty isel: return a lot of empty stuff */
    if (isel == NULL || *isel == '\0')
    {
	*is1= *is2= *is3= NULL;
	return (flag==3) ? 3 : 2;
    }

    /* User not logged in - everything is brandnew */
    if (curr == NULL)
    {
	if (flag == 2)
	{
	    *is1= NULL;
	    *is2= strdup(isel);
	}
	else
	{
	    *is1= strdup(isel);
	    *is2= NULL;
	}
	*is3= NULL;
	return (flag==3) ? 3 : 2;
    }

    fav.sel= new.sel= other.sel= NULL;

    for (;;)
    {
	/* First parse into comma-separated hunks */
	next_range= strchr(isel,',');
	if (next_range != NULL)
	    *(next_range++)= '\0';

	/* Get begining and end of range */
	dash= strchr(isel,'-');
	if (dash == NULL)
	{
	    /* Isolated $ will be converted into current last item number */
	    i1= i2= (*isel == '$') ? idict(VAR_MAXITEM) : atol(isel);
	}
	else
	{
	    *dash= '\0';
	    i1= ff_inum(isel);
	    i2= ff_inum(dash+1);
	}

	/* Reverse range if it is non-increasing */
	if (i1 > i2) 
	{
	    reverse= i1;
	    i1= i2;
	    i2= reverse;
	    favi= (fav.sel == NULL) ? 0 : fav.i;
	    newi= (new.sel == NULL) ? 0 : new.i;
	    otheri= (other.sel == NULL) ? 0 : other.i;
	}
	else
	    reverse= 0;

	/* Walk the range */
	for (i= i1; i <= i2; i++)
	{
	    rc= find_pf(curr, i, FALSE, &pf);
	    if (rc == 1)
	    {
		/* end of participation file - anything after it is brandnew */
		if (i < i2 || pnew->sel == NULL || pnew->first == -1)
		    ff_app(pnew,i);
		pnew->last= i2;
		break;
	    }
	    else if ((flag & 1) && rc == 0 && pf->favorite)
	    {
		/* found a favorite */
		ff_app(&fav,i);
	    }
	    else if ((flag & 2) && (rc == 2 || pf->resp_no == 0))
	    {
		/* found a brandnew */
		ff_app(&new,i);
	    }
	    else
	    {
		/* found something else */
		ff_app(&other,i);
	    }
	}

	/* Finish the fav, new, and other selectors up */
	if (flag & 1) ff_app(&fav,-1);
	if (flag & 2) ff_app(&new,-1);
	ff_app(&other,-1);

	/* Reverse the range, if needed */
	if (reverse)
	{
	    if ((flag & 1) && fav.sel != NULL)
	    {
		if (fav.sel[favi] == ',') favi++;
		rev_sel(fav.sel+favi);
	    }
	    if ((flag & 2) && new.sel != NULL)
	    {
		if (new.sel[newi] == ',') newi++;
		rev_sel(new.sel+newi);
	    }
	    if (other.sel != NULL)
	    {
		if (other.sel[otheri] == ',') otheri++;
		rev_sel(other.sel+otheri);
	    }
	}

	/* Get ready for the next iteration */
	if (next_range == NULL) break;
	isel= next_range;
    }

    /* Return things in suitable slots */
    switch (flag)
    {
    case 1:
	*is1= fav.sel;
	*is2= other.sel;
	*is3= NULL;
	return 2;

    case 2:
	*is1= new.sel;
	*is2= other.sel;
	*is3= NULL;
	return 2;

    case 3:
	*is1= fav.sel;
	*is2= new.sel;
	*is3= other.sel;
	return 3;
    }
}
