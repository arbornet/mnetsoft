/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#define TABSIZE 3000
#define MEMFILE "memchk"


struct memchk {
	int id;
	size_t size;
	void *ptr;
	} memtab[TABSIZE];
int next_slot= 0;
int next_id= 0;

FILE *memfp= NULL;

void xopenfile()
{
    if (memfp == NULL && (memfp= fopen(MEMFILE,"w")) == NULL)
	die("cannot open "MEMFILE);
}


int xfreeslot()
{
    int i;

    for (i= 0; i < next_slot; i++)
	if (memtab[i].ptr == NULL)
	    return(i);
    if (next_slot < TABSIZE)
	return(next_slot++);
    else
	die("out of memtab slots");
}


int xfindslot(void *p)
{
    int i;

    for (i= 0; i < next_slot; i++)
	if (memtab[i].ptr == p)
	    return(i);
    die("freed un-allocated memory");
}

void *xmalloc(size_t size)
{
    int i= xfreeslot();

    xopenfile();
    fprintf(memfp,"%d: malloc %d\n",next_id, size);
    fflush(memfp);

    memtab[i].id= next_id++;
    memtab[i].size= size;
    memtab[i].ptr= malloc(size);

    return (memtab[i].ptr);
}


void *xrealloc(void *ptr, size_t size)
{
    int i= xfindslot(ptr);

    xopenfile();
    fprintf(memfp,"%d: realloc %d->%d \n",
	    memtab[i].id, memtab[i].size, size);
    fflush(memfp);

    memtab[i].size= size;
    memtab[i].ptr= realloc(ptr,size);
    return (memtab[i].ptr);
}


void xfree(void *ptr)
{
    int i= xfindslot(ptr);

    xopenfile();
    fprintf(memfp,"%d: free %d\n", memtab[i].id, memtab[i].size);
    fflush(memfp);

    memtab[i].ptr= NULL;
    free(ptr);
}
