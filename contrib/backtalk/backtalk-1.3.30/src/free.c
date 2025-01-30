/* (c) 1996-2002, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "free.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* FREE_VAL - Deallocate any memory allocated to store the value of a token.
 * This does not deallocate the token itself.
 */

void free_val(Token *t)
{
    if (class(*t) == TKC_STRING && (t->flag & TKF_FREE))
	free(sval(*t));
    else if (class(*t) == TKC_ARRAY)
	free_array(aval(*t));
    else if (class(*t) == TKC_REGEX)
	free_regex(rval(*t));
    else if (class(*t) == TKC_DICT)
	free_dict(hval(*t));
}


/* FREE_ARRAY - Discard an array.  Actually, we just decrement it's link count.
 * If it goes to zero, then we really free it.
 */

void free_array(Array *a)
{
    int i;

    if (--(a->lk) <= 0)
    {
	for (i= 0; i < a->sz; i++)
	    free_val(&(a->a[i]));
	if (a->a != NULL) free(a->a);
	free(a);
    }
}


/* FREE_REGEX - Discard a regular expression.  Actually, we just decrement
 * it's link count.  If it goes to zero, then we really free it.
 */

void free_regex(Regex *r)
{
    if (--(r->lk) <= 0)
    {
	regfree(&(r->re));
	free(r);
    }
}


/* FREE_DICT - Discard a dictionary.  Actually, we just decrement
 * it's link count.  If it goes to zero, then we really free it.
 */

void free_dict(HashTable *h)
{
    if (--(h->lk) <= 0)
    {
	DeleteHashTable(h);
	free(h);
    }
}
