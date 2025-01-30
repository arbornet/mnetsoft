/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* String search routines. */

#include "backtalk.h"

#include <ctype.h>

#include "sysdict.h"
#include "stack.h"
#include "search.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif


/* BM_COMPILE - Do compilation for a Boyer-Moore Search.  The pattern is
 * expected to be a malloced string, and should *NOT* be freed by the caller
 * afterwards.
 */

char *prev_search= NULL;
int charjump[256];
int *matchjump= NULL;
int prev_len, prev_icase;

int bm_compile(char *pattern, int ignore_case)
{
    int *bck;
    int i, q, r, k;
    /* Fake one-based arrays so I don't have to rewrite algorithm */
    #define patn (pattern - 1)
    #define back (bck - 1)
    #define mjmp (matchjump - 1)

    /* FIRST: Save pattern */
    if (pattern[0] == '\0')
    {
	if (prev_search == NULL) die("no previous search");
	free(pattern);
	pattern= prev_search;
    }
    else
    {
	if (prev_search != NULL) free(prev_search);
	prev_search= pattern;
	prev_len= strlen(pattern);
    }
    prev_icase= ignore_case;

    /* SECOND: Build charjump array */

    for (i= 0; i < 256; i++)
	charjump[i]= prev_len;

    for (k= 1; k <= prev_len; k++)
    {
	if (prev_icase && isascii(patn[k]) && isupper(patn[k]))
	    patn[k]= tolower(patn[k]);
	charjump[(unsigned char)patn[k]]= prev_len - k;
    }
    
    /* FINALLY: Build matchjump array */

    if (matchjump != NULL) free(matchjump);
    matchjump= (int *)malloc(prev_len * sizeof(int));
    bck= (int *)malloc(prev_len * sizeof(int));

    for (k= 1; k <= prev_len; k++)
	mjmp[k]= 2*prev_len - k;
    
    for (k= prev_len, q= prev_len+1; k > 0; k--, q--)
    {
	back[k]= q;
	while (q <= prev_len && patn[k] != patn[q])
	{
	    if (mjmp[q] > prev_len - k)
		mjmp[q]= prev_len - k;
	    q= back[q];
	}
    }

    for (k= 1; k <= q; k++)
	if (mjmp[k] > prev_len + q - k)
	    mjmp[k]= prev_len + q - k;

    r= back[q];
    while (q <= prev_len)
    {
	if (q <= r)
	{
	    if (mjmp[q] > r - q + prev_len)
		mjmp[q]= r - q + prev_len;
	    q++;
	}
	else
	    r= back[r];
    }

    free(bck);

#undef patn
#undef back
#undef mjmp
}


/* BM_SEARCH - Search the given text for the last compiled pattern.  This is
 * the old reliable Boyer-Moore search.  Return -1 if no match, or the offset
 * of the match in the string otherwise.
 */

int bm_search(char *txt)
{
    register char *txtp, *patp;
    char *txt_end, *pat_end;
    char tch;

    txt_end= txt + strlen(txt) - 1;
    pat_end= prev_search + prev_len - 1;

    txtp= txt + prev_len - 1;
    patp= pat_end;

    while (txtp <= txt_end && patp >= prev_search)
    {
	tch= (prev_icase && isascii(*txtp) && isupper(*txtp)) ?
	    tolower(*txtp) : *txtp;

	if (tch == *patp)
	{
	    txtp--;
	    patp--;
	}
	else
	{
	    txtp+= max(charjump[(unsigned char)tch],
		       matchjump[(unsigned char)(patp-prev_search)]);
	    patp= pat_end;
	}
    }
    return ( (patp < prev_search) ? txtp - txt + 1 : -1 );
}


/* FUNC_SEARCH:   <text> <pattern> <ignore_case> search <offset>
 * Search a string for a fixed pattern.  Returns the offset into the text at
 * which the pattern was found.  If the pattern is not found, an offset of
 * -1 is pushed.  If the pattern is null, the previous pattern is reused.
 */

void func_search()
{
    int icase= pop_integer();
    char *pat= pop_string();
    char *txt= pop_string();

    bm_compile(pat,icase);
    push_integer(bm_search(txt));
    free(txt);
    /* do not free(pat) - it gets stuck in prev_search global */
}


/* FUNC_REPLACE:  <text> <pattern> <replacement> <ignorecase> replace <newtext>
 * Find all occurances of <pattern> in <text> and replace each one with
 * <replacement> resulting in the new string <newtext>.
 */

void func_replace()
{
    int icase= pop_integer();
    char *rep= pop_string();
    char *pat= pop_string();
    char *txt= pop_string();
    char *cur= txt;
    size_t outsize= strlen(txt)*2;
    size_t outlen= 0;
    size_t replen= strlen(rep);
    int patlen= strlen(pat);
    char *out= (char *)malloc(outsize);
    int i;

    bm_compile(pat,icase);
    while((i= bm_search(cur)) != -1)
    {
	/* Enlarge output buffer if necessary */
	if (outlen + i + replen + 1 > outsize)
	{
	    outsize*= 2;
	    out= (char *)realloc(out, outsize);
	}

	/* Copy over start of string */
	strncpy(out+outlen, cur, i);
	outlen+= i;
	/* Copy over replacement */
	strcpy(out+outlen, rep);
	outlen+= replen;

	/* Advance past end of pattern */
	cur+= i + patlen;
    }

    /* Re-allocate buffer to correct size */
    out= (char *)realloc(out, outlen + strlen(cur) + 1);
    /* Copy over last bit */
    strcpy(out+outlen,cur);

    free(txt);
    /* do not free(pat) */
    free(rep);

    push_string(out,FALSE);
}


#ifdef CLEANUP
/* FREE_SEARCH - Cleanup routine used only when debugging memory.
 */

void free_search()
{
    if (prev_search != NULL) free(prev_search);
    prev_search= NULL;
    if (matchjump != NULL) free(matchjump);
    matchjump= NULL;
}
#endif
