/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Array manipulation functions
 */

#include "backtalk.h"
#include "array.h"
#include "stack.h"
#include "free.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* FUNC_IN	<value> <array> in <index>
 * Returns the index of the first thing in the array which is 'eq' to value,
 * or -1 if nothing is.
 */

void func_in()
{
    Array *a;
    Token t;
    int i;

    a= pop_array();
    pop_any(&t);

    for (i= 0; i < a->sz; i++)
    {
    	if (type(t) == type(a->a[i]))
	{
	    switch (type(t))
	    {
	    case TK_INTEGER:
	    case TK_BOUND_LITERAL:
	    	if (ival(t) == ival(a->a[i])) {push_integer(i); return; }
		break;
	    case TK_TIME:
	    	if (tval(t) == tval(a->a[i])) {push_integer(i); return; }
		break;
	    case TK_STRING:
	    case TK_UNBOUND_LITERAL:
	    	if (!strcmp(sval(t),sval(a->a[i]))) {push_integer(i); return; }
		break;
	    }
    	}
	else if (type(t) == TK_UNBOUND_LITERAL &&
		 type(a->a[i]) == TK_BOUND_LITERAL &&
		 !strcmp(sval(t),sysdict[ival(a->a[i])].key))
	{
	    push_integer(i); return;
	}
	else if (type(t) == TK_BOUND_LITERAL &&
		 type(a->a[i]) == TK_UNBOUND_LITERAL &&
		 !strcmp(sysdict[ival(t)].key,sval(a->a[i])))
	{
	    push_integer(i); return;
	}
    }
    push_integer(-1);
}


/* FUNC_ALOAD:  <array> aload <a0>...<an-1>
 * Push the elements of an array onto the stack.
 */

void func_aload()
{
    Array *a= pop_array();
    int i, discard= (--(a->lk) <= 0);

    for (i= 0; i < a->sz; i++)
	push_token(&(a->a[i]),!discard);
    
    if (discard)
    {
	if (a->a != NULL) free(a->a);
	free(a);
    }
}


/* KEYCMP - Support routine for the sorts below.  Pointers to two subarrays
 * are given.  This compares the (key)th values in the arrays to each other,
 * returning a negative, zero, or positive value as the first key is less,
 * equal or greater than the other (except that if dir is -1 instead of 1,
 * we reverse this).  Assumes all the sanity checks in func_asort() have been
 * done.
 */

int keycmp(Array *a, Array *b, int key, int dir)
{
    Token *ak= a->a+key;
    Token *bk= b->a+key;

    switch (type(*ak))
    {
    case TK_INTEGER:
        return (ival(*ak) - ival(*bk)) * dir;
    case TK_TIME:
        return (tval(*ak) - tval(*bk)) * dir;
    case TK_STRING:
        return strcmp(sval(*ak),sval(*bk)) * dir;
    }
}


/* ISORT - Insertion sort an array of Tokens.  Each token in the array is
 * expected to be an array.  Key is the index into the subarrays to sort by.
 * Assumes all the sanity checks in func_asort() have been done.
 */

void insort(Token *a, int n, int key, int dir)
{
    int i,j;

    for (i= 1; i < n; i++)
    {
    	Array *tmp= aval(a[i]);
    	for (j= i; j > 0 && keycmp(tmp,aval(a[j-1]),key,dir) < 0; j--)
    	    a[j].val= a[j-1].val;
        a[j].val= (void *)tmp;
    }
}


/* QKSORT - Quicksort an array of Tokens.  Each token in the array is expected
 * to be an array.  Key is the index into the subarrays to sort by.  Assumes
 * all the sanity checks in func_asort() have been done.
 */

void qksort(Token *a, int lo, int hi, int key, int dir)
{
    Array *tmp, *pivot;
    int i,j;

    if (lo + 10 > hi)
    	/* Use insertion sort on arrays smaller than 10 */
    	insort(a+lo, hi-lo+1, key, dir);
    else
    {
    	/* First, do median of three pivot selection */
    	int mid= (lo + hi) / 2;
    	/* sort mid, hi, and lo */
    	if (keycmp( aval(a[mid]), aval(a[lo]),  key, dir) < 0)
	    tmp= aval(a[mid]), a[mid].val= a[lo].val, a[lo].val=  (void *)tmp;
    	if (keycmp( aval(a[hi]),  aval(a[lo]),  key, dir) < 0)
	    tmp= aval(a[hi]),  a[hi].val= a[lo].val,  a[lo].val=  (void *)tmp;
    	if (keycmp( aval(a[hi]),  aval(a[mid]), key, dir) < 0)
	    tmp= aval(a[hi]),  a[hi].val= a[mid].val, a[mid].val= (void *)tmp;

	/* Swap pivot into hi-1 position */
	pivot= aval(a[mid]);
	a[mid].val= a[hi-1].val;
	a[hi-1].val= (void *)pivot;

	/* Partition */
	i= lo;
	j= hi-1;
	for (;;)
	{
	    while(keycmp(aval(a[++i]), pivot, key, dir) <  0)
	    	;
	    while(keycmp(pivot, aval(a[--j]), key, dir) <  0)
	    	;
	    if (i >= j) break;
	    tmp= aval(a[i]);  a[i].val= a[j].val;  a[j].val= (void *)tmp;
	}

	/* Swap pivot back where it belongs */
	a[hi-1].val= a[i].val;  a[i].val= (void *)pivot;

	/* Quicksort the rest */
	qksort(a, lo, i-1, key, dir);
	qksort(a, i+1, hi, key, dir);
    }
}


/* FUNC_ASORT:  <array> <i> <ascending> asort <array>
 * Given an array of arrays, sort the array buy the ith keys of the subarrays.
 * If ascending is true, sort in increasing order, otherwise sort in
 * decreasing order.
 */

void func_asort()
{
    int ascending= pop_boolean();
    int key= pop_integer();
    Array *a= pop_array();
    Array *sa;
    int i,t;

    /* First a sanity check, so we can assume all is sane while sorting */
    for (i= 0; i < a->sz; i++)
    {
    	if (type(a->a[i]) != TK_ARRAY)
    	    die("element %d of array being sorted is not an array",i);
	sa= aval(a->a[i]);
	if (sa->sz <= key)
	    die("element %d of array being sorted has no element %d",i,key);
	if (i == 0)
	{
	    t= type(sa->a[key]);
	    if (t != TK_INTEGER && t != TK_TIME && t != TK_STRING)
	        die("can only sort by integer, time, or string keys\n");
	}
	else if (type(sa->a[key]) != t)
	    die("subarrays 0 and %d of the array being sorted have "
	        "different types of keys in position %d",i,key);
    }

    qksort(a->a, 0, a->sz-1, key, ascending ? 1 : -1);
    /*insort(a->a, a->sz, key, ascending ? 1 : -1);*/

    push_array(a,FALSE);
}


#if 0	/* Decided this wasn't that useful */

/* FUNC_APUSH:   <array> <any> apush -
 * Append a value onto the end of an array, lengthening it.
 * Not a super efficient implementation, since we realloc for
 * each push.
 */

void func_apush()
{
    Token t;
    int i;
    Array *a;

    pop_any(&t);
    a= pop_array();

    i= a->sz - 1;
    a->sz++;
    a->a= (Token *)realloc(a->a, a->sz * sizeof(Token));
    a->a[i]= t;
}

#endif
