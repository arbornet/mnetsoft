/* (c) 1996-2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* These routines handle growable buffers, mostly used for strings.  Steve
 * Weiss originally wrote these for use in format.c.  Jan Wolter split them
 * out as a separate module to use throughout Backtalk.  There's a couple
 * dozen places we should probably be using this, and maybe as things evolve
 * we'll actually do so.
 */

#include "common.h"
#include "vstr.h"
#include "str.h"


/* VS_NEW - Initialize a new VSTRING object, in empty state, with given
 * initial size.  Returns FALSE if out of memory.
 */

int vs_new(VSTRING *vs, size_t size)
{
    /* Default size to BFSZ if called with size zero */
    if (size <= 0) size= BFSZ;

    vs->begin= (char *)malloc(size);
    if (!vs->begin) return FALSE;

    vs->size= size;
    vs->p= vs->begin;
    vs->last= NULL;
    return TRUE;
}


/* VS_EMPTY - Reset a VSTRING for reuse, emptying it without discarding
 * memory. */

void vs_empty(VSTRING *vs)
{
    vs->p= vs->begin;
    vs->last= NULL;
}


/* VS_START - this is vs_new, but with an initial string value.  We trust the
 * calling program to pass in a correct length for the string value, but the
 * size will be increased if the length is larger.  Returns FALSE if out of
 * Memory.
 */

int vs_start(VSTRING *vs, size_t size, char *val, int len)
{
    if (len > size) size= len*2+1;
    if (size <= 0) size= BFSZ;

    vs->begin= (char *)malloc(size);
    if (!vs->begin) return FALSE;

    vs->size= size;
    memmove(vs->begin, val, len);
    vs->p= vs->begin+len;
    vs->last= NULL;
    return TRUE;
}


/* VS_DESTROY - Free memory allocated to a VSTRING. */

void vs_destroy(VSTRING *vs)
{
    if (vs->begin) free(vs->begin);
    vs->begin= NULL;
}


/* VS_REALLOC - Increase the size of a VSTRING to the given value.  This is
 * only save if the new size is >= the old size.  Returns FALSE if out of
 * memory.
 */

int vs_realloc(VSTRING *vs, size_t size)
{
    int pi, lasti;
    pi= vs->p - vs->begin;
    if (vs->last != NULL) lasti= vs->last - vs->begin;

    vs->begin= (char *)realloc(vs->begin, size);
    if (!vs->begin) return FALSE;

    vs->size= size;
    vs->p= vs->begin + pi;
    vs->last= vs->begin + lasti;
    return TRUE;
}


/* VS_MORE - Increase the size of a buffer - doubling it normally but might
 * add less if low on memory.  Dies if out of memory.
 */

void vs_more(VSTRING *vs)
{
    /* try to realloc double - if not, realloc +256 */
    if (!vs_realloc(vs,(vs->size+1)*2) && !vs_realloc(vs,vs->size+256))
	die("not enough memory to hold expanding string of length %d.\n",
	    vs->size+256);
}


/* VS_INC - Call this before saving a value at vs->p.  Returns the old value
 * of p, so you can append a character with:   *vs_inc(vs)= 'x';
 */

char *vs_inc(VSTRING *vs)
{
    if (vs->p >= vs->begin + vs->size) vs_more(vs); /* moves p */
    return vs->p++;
}


/* VS_INC_N - Call this before adding n characters at vs->p.  Returns old
 * value of p, so you can do memmove(vs_inc_n(vs,5),"Hello",5).
 */

char *vs_inc_n(VSTRING *vs, size_t n)
{
    char *ret;
    while (vs->p+n-1 >= vs->begin + vs->size) vs_more(vs);
    ret= vs->p;
    vs->p+= n;
    return ret;
}


/* VS_CAT - Append all characters of a nil-terminated string to a VSTRING */
void vs_cat(VSTRING *vs, char *psz)
{
    size_t len= strlen(psz);
    vs_cat_n(vs,psz,len);
}


/* VS_RETURN - prepare for returning the result - adds a NUL, then resizes
 * it smaller if it is worth the trouble.  Returns a pointer to the value to
 * return.  Older versions only resized if we were using less than 80% of
 * the allocated memory, but realloc()ing to a smaller size should be a cheap
 * operation, so we do it if 32 or more bytes of memory are wasted.  The
 * VSTRING could be broken after calling this.
 */

char *vs_return(VSTRING *vs)
{
    int len;

    if (vs->begin == NULL) return NULL;

    /* write a NUL */
    *vs_inc(vs)= '\0';

    /* if 32 or more bytes wasted, realloc to a smaller size */
    len= vs->p - vs->begin;
    if (vs->size >= len + 32)
	return (char *)realloc(vs->begin, len);
    else
	return vs->begin;
}
