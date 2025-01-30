#include "common.h"

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <regex.h>

#include "utils.h"
#include "regex2.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* Flat format consists of the following structures in the following sequence:
 *    - one 'struct flathead' with info from regex_t and re_guts.
 *    - array of strips (converted to longs in network byte order).
 *    - array of sets[]->smultis (converted to longs in  network byte order).
 *    - array of other sets[] data (mask char, hash char, ptr array, multis
 *      array).
 *    - array of setbits[] arrays.
 *    - must character array.
 *    - catspace character array with length given by flathead.ncategories.
 * All integer/long values are stored in network byte order.
 */

struct flathead {
	/*** fields from regex_t ***/
	/* re_magic - not saved */
	unsigned long re_nsub;
	/* re_endp - not saved */
	/* re_guts - not saved here */
	/*** fields from re_guts ***/
	/* magic - not saved */
	unsigned long csetsize;
	unsigned long ncsets;
	/* sets - not saved here */
	/* setbits - not saved here */
	unsigned long cflags;
	unsigned long nstates;
	unsigned long firststate;
	unsigned long laststate;
	unsigned long iflags;
	unsigned long nbol;
	unsigned long neol;
	unsigned long ncategories;
	/* categories not saved */
	/* must not saved here */
	unsigned long mlen;
	unsigned long nsub;
	unsigned long backrefs;
	unsigned long nplus;
	/* catspace not saved here */
	};

/*
 - regtoflat - build a flat representation of a regular expression, suitable for
 - writing out to a file.
 = extern char *regtoflat(regex_t *, int *);
 */

char *
regtoflat(regex_t *r, int *n)
{
	int nstrips, nsets, tsetbits, nmust, tmultis;
	int i,j;
	char *b, *p;
	struct flathead *f;
	int err;

	if (r == NULL || r->re_magic != MAGIC1 ||
	    r->re_g == NULL || r->re_g->magic != MAGIC2)
		return NULL;
	
	/* Numbers of various elements */
	nstrips= r->re_g->strip ? r->re_g->nstates : 0;
	nsets= r->re_g->sets ?  r->re_g->ncsets : 0;
	tsetbits= r->re_g->setbits ?  nsets / CHAR_BIT * r->re_g->csetsize : 0;
	nmust= r->re_g->must ? r->re_g->mlen : 0;
	
	/* Total size of all multis */
	tmultis= 0;
	for (i= 0; i < nsets; i++)
		tmultis+= r->re_g->sets[i].smultis;

	*n= sizeof(struct flathead) + nstrips*4 +
		nsets*4 + nsets*(2+r->re_g->csetsize) + tmultis +
		tsetbits*r->re_g->csetsize +
		nmust + r->re_g->ncategories;

	b= (char *)malloc(*n);

	f= (struct flathead *)b;
	f->re_nsub= htonl(r->re_nsub);
	f->csetsize= htonl(r->re_g->csetsize);
	f->ncsets= htonl(nsets);
	f->cflags= htonl(r->re_g->cflags);
	f->nstates= htonl(nstrips);
	f->firststate= htonl(r->re_g->firststate);
	f->laststate= htonl(r->re_g->laststate);
	f->iflags= htonl(r->re_g->iflags);
	f->nbol= htonl(r->re_g->nbol);
	f->neol= htonl(r->re_g->neol);
	f->ncategories= htonl(r->re_g->ncategories);
	f->mlen= htonl(nmust);
	f->nsub= htonl(r->re_g->nsub);
	f->backrefs= htonl(r->re_g->backrefs);
	f->nplus= htonl(r->re_g->nplus);
	p= b+sizeof(struct flathead);

	for (i= 0; i < nstrips; i++)
	{
	     *((unsigned long *)p)= htonl(r->re_g->strip[i]);
	     p+= 4;
	}

	for (i= 0; i < nsets; i++)
	{
	     *((unsigned long *)p)= htonl(r->re_g->sets[i].smultis);
	     p+= 4;
	}

	for (i= 0; i < nsets; i++)
	{
	     *(p++)= r->re_g->sets[i].mask;
	     *(p++)= r->re_g->sets[i].hash;
	     memmove(p, r->re_g->sets[i].ptr, r->re_g->csetsize);
	     p+= r->re_g->csetsize;
	     memmove(p, r->re_g->sets[i].multis, r->re_g->sets[i].smultis);
	     p+= r->re_g->sets[i].smultis;
	}

        memmove(p, r->re_g->setbits, tsetbits);
	p+= tsetbits;

        memmove(p, r->re_g->must, nmust);
	p+= nmust;

        memmove(p, r->re_g->catspace, r->re_g->ncategories);

	return b;
}

/*
 - flattoreg - reconstruct the regular expression structure from
 - the flattened version.
 -
 - The memory allocation appears to be a bit different from the original, so
 - that regfree() will not completely deallocate it.  For backtalk this is not
 - a big issue, since we never deallocate the regular expressions loaded with
 - the program anyway (unless we are doing memory tests with CLEANUP defined).
 - It'd be nice to get this right, but not worth the effort at this time.
 -
 = extern  void flattoreg(char *, regex_t *r);
 */

void
flattoreg(char *b, regex_t *r)
{
	int nn, nc,i;
	struct re_guts *g;
	struct flathead *f= (struct flathead *)b;
	
	r->re_magic= MAGIC1;
	r->re_nsub= ntohl(f->re_nsub);
	r->re_endp= NULL;

	nc= ntohl(f->ncategories);
	r->re_g= g= (struct re_guts *)malloc(sizeof(struct re_guts) +
		(nc - 1) * sizeof(cat_t));

	g->magic= MAGIC2;
	g->csetsize= ntohl(f->csetsize);
	g->ncsets= ntohl(f->ncsets);
	g->cflags= ntohl(f->cflags);
	g->nstates= ntohl(f->nstates);
	g->firststate= ntohl(f->firststate);
	g->laststate= ntohl(f->laststate);
	g->iflags= ntohl(f->iflags);
	g->nbol= ntohl(f->nbol);
	g->neol= ntohl(f->neol);
	g->ncategories= ntohl(f->ncategories);
	g->mlen= ntohl(f->mlen);
	g->nsub= ntohl(f->nsub);
	g->backrefs= ntohl(f->backrefs);
	g->nplus= ntohl(f->nplus);

	b+= sizeof(struct flathead);
	
	/* Get strips */
	if (g->nstates == 0)
		g->strip= NULL;
	else
		g->strip= (sop *)malloc(g->nstates * sizeof(sop));

	for (i= 0; i < g->nstates; i++)
	{
	     g->strip[i]= ntohl(*((unsigned long *)b));
	     b+= 4;
	}

	/* Get csets */
	if (g->ncsets == 0)
		g->sets= NULL;
	else
		g->sets= (cset *)malloc(g->ncsets * sizeof(cset));

	for (i= 0; i < g->ncsets; i++)
	{
	     g->sets[i].smultis= ntohl(*((unsigned long *)b));
	     b+= 4;
	}

	for (i= 0; i < g->ncsets; i++)
	{
	    g->sets[i].mask= *(b++);
	    g->sets[i].hash= *(b++);
	    if (g->csetsize == 0)
		g->sets[i].ptr= NULL;
	    else
	    {
		g->sets[i].ptr= malloc(g->csetsize);
		memmove(g->sets[i].ptr, b, g->csetsize);
		b+= g->csetsize;
	    }
	    if (g->sets[i].smultis == 0)
	    	g->sets[i].multis= NULL;
	    else
	    {
		g->sets[i].multis= malloc(g->sets[i].smultis);
		memmove(g->sets[i].multis, b, g->sets[i].smultis);
		b+= g->sets[i].smultis;
	    }
	}

	nn= g->ncsets / CHAR_BIT * g->csetsize * sizeof(uch);
	if (nn == 0)
		g->setbits= NULL;
	else
	{
		g->setbits= (uch *)malloc(nn);
		memmove(g->setbits, b, nn);
		b+= nn;
	}

	if (g->mlen == 0)
		g->must= NULL;
	else
	{
		r->re_g->must= (char *)malloc(g->mlen);
		memmove(g->must, b, g->mlen);
		b+= g->mlen;
	}

	if (g->ncategories > 0)
		memmove(g->catspace, b, g->ncategories);

	g->categories= &g->catspace[-(CHAR_MIN)];
}
