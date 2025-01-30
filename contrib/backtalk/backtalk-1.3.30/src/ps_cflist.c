/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include "ps_cflist.h"
#include "sysdict.h"
#include "file.h"
#include "str.h"
#include "stack.h"
#include "free.h"
#include "user.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* A handle for reading through a .cflist */
struct read_cflist {
    FILE *fp;
    char buf[BFSZ+1];
    char *ptr;
};


/* NEXT_CFLIST - Given a .cflist read handle, return the next space-deliminted
 * word from that .cflist.  Lines with # signs in the first column are returned
 * if comments is true, otherwise they are returned whole, with the leading
 * # sign.  The returned strings are always in malloced memory.
 */

char *next_cflist(struct read_cflist *h, int comment)
{
    char *b,*e,*cf;
    int len;

    if (h->ptr == NULL || *(b= firstout(h->ptr," \t\n")) == '\0')
    {
	/* Don't have a current line - get one with something useful in it */
	for (;;)
	{
	    if (fgets(h->buf,BFSZ,h->fp) == NULL)
		return NULL;
	    /* Skip comment lines */
	    if (h->buf[0] == '#')
	    {
		if (comment)
		{
		    *firstin(h->buf+1,"\n")= '\0';
		    h->ptr= NULL;
		    return strdup(h->buf);
		}
		else
		    continue;
	    }
	    b= firstout(h->buf," \t\n");
	    /* Skip blank lines */
	    if (*b != '\0')
		break;
	}
    }
    h->ptr= firstin(b," \t\n");
    len= h->ptr - b;

    cf= (char *)malloc(len+1);
    strncpy(cf, b, len);
    cf[len]= 0;
    return cf;
}


/* CF_GROUP - Given a conference name which may be like <group>:<conf> or just
 * <conf>, return <conf> if we are in the group (or if there is no group).
 * The passed in value is assumed to be in malloced memory, and will be freed.
 * The returned value should be freed by the caller.
 */

char *cf_group(char *cf)
{
    char *p;
    int in;

    /* If no conf or no colon, do nothing */
    if (cf == NULL || (p= strchr(cf,':')) == NULL)
	return cf;

    /* If we are anonymous, then we are in no groups */
    if (idict(VAR_UID) < 0 || sdict(VAR_ID)[0] == '\0')
    {
	free(cf);
	return NULL;
    }

    /* Check if we are in the group */
    *p= '\0';
    if (*cf >= '0' && *cf <= '9')
	in= ingroupno(atoi(cf), user_gid(), user_id());
    else
	in= ingroupname(cf, user_gid(), user_id());

    if (in)
    {
	/* We are in, return just the conference name */
	strcpy(cf, p+1);
	return cf;
    }
    else
    {
	/* We aren't in, swallow it up */
	free(cf);
	return NULL;
    }
}


/* FUNC_CFLIST_ARRAY:  <flag> cflist_array <array>
 *
 * Push an array containing the user's .cflist onto the stack.  
 * The flag can have the following values:
 *   0     read dflt.cflist file, omit comments
 *   1     read user's .cflist file, omit comments
 *   2     read dflt.cflist file, include comments
 *   3     read user's .cflist file, include comments
 * If reading the dflt.cflist file <group>:<conf> type entries are returned
 * whole.  Otherwise they are resolved, and only the conference name is
 * returned, only if the user is in that group.
 */

void func_cflist_array()
{
    struct read_cflist h;
    char *cf;
    int flag= pop_boolean();
    int mine= flag & 1;
    int comments= flag & 2;

    /* Push a [ mark */
    push_array_mark();
    
    if ((h.fp= open_cflist(mine)) != NULL)
    {
	h.ptr= NULL;

	/* Push all the conferences */
	while ((cf= next_cflist(&h,comments)) != NULL)
	    if (!mine || (cf= cf_group(cf)))
		push_string(cf,FALSE);
    }

    /* form up the array, executing the ] function */
    make_array(0);

    if (h.fp != NULL) fclose(h.fp);
};


/* NTH_CONF - return the name of the nth conference in the user's .cflist
 * file in confname.  We count from zero, so the first conference is 0.
 * <group>:<conf> entries are ignored if we are not in the groups.  Uses
 * the dflt.cflist if the user hasn't one of his own.  Returned value is
 * in malloced memory.
 */

char *nth_conf(int n)
{
    struct read_cflist h;
    char *cf;

    /* Get ready to read */
    if ((h.fp= open_cflist(1)) == NULL)
	return NULL;
    h.ptr= NULL;

    /* Fetch conferences, discarding them all until we reach the nth */
    for (;;)
    {
	if ((cf= cf_group(next_cflist(&h,0))) == NULL)
	{
	    fclose(h.fp);
	    return NULL;
	}
	if (n-- == 0)
	{
	    fclose(h.fp);
	    return cf;
	}
	free(cf);
    }
}
