/* This library of hash functions is derived from the Tcl hash code.  This
 * code is not covered in any way by the Backtalk license or copyright.
 * The relevant license and copyright for this file and for hash.h are
 * described below.
 * 
 * It has been stripped down to support only string hashs.  Support of
 * non-ANSI C compilers has also been dropped.  Function names have been
 * renamed without the Tcl_ prefix.  The generic clientData field hash
 * been replaced with a Backtalk Token and flag field.
 */

/* 
 * tclHash.c --
 *
 *	Implementation of in-memory hash tables for Tcl and Tcl-based
 *	applications.
 *
 * Copyright (c) 1991-1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the "Tcl LICENSE" below for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *
 * Tcl LICENSE:
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., Scriptics Corporation,
 * and other parties.  The following terms apply to all files associated
 * with the software unless explicitly disclaimed in individual files.
 * 
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions. No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 * 
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 * FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 * DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
 * IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
 * NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 * MODIFICATIONS.
 * 
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal 
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense, the
 * software shall be classified as "Commercial Computer Software" and the
 * Government shall have only "Restricted Rights" as defined in Clause
 * 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing, the
 * authors grant the U.S. Government and others acting in its behalf
 * permission to use and distribute the software in accordance with the
 * terms specified in this license. 
 */

#include <stdio.h>
#include "backtalk.h"
#include "free.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/*
 * When there are this many entries per bucket, on average, rebuild
 * the hash table to make it larger.
 */

#define REBUILD_MULTIPLIER	3


/*
 * The following macro takes a preliminary integer hash value and
 * produces an index into a hash tables bucket list.  The idea is
 * to make it so that preliminary values that are arbitrarily similar
 * will end up in different buckets.  The hash function was taken
 * from a random-number generator.
 */

#define RANDOM_INDEX(tablePtr, i) \
    (((((long) (i))*1103515245) >> (tablePtr)->downShift) & (tablePtr)->mask)

/*
 * Procedure prototypes for static procedures in this file:
 */

static unsigned int	HashString (CONST char *string);
static void		RebuildTable (HashTable *tablePtr);

/*
 *----------------------------------------------------------------------
 *
 * InitHashTable --
 *
 *	Given storage for a hash table, set up the fields to prepare
 *	the hash table for use.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TablePtr is now ready to be passed to FindHashEntry and
 *	CreateHashEntry.
 *
 *----------------------------------------------------------------------
 */

void
InitHashTable(tablePtr)
    register HashTable *tablePtr;	/* Pointer to table record, which
					 * is supplied by the caller. */
{
#if (SMALL_HASH_TABLE != 4) 
    die("InitHashTable: SMALL_HASH_TABLE is %d, not 4\n",
	    SMALL_HASH_TABLE);
#endif
    
    tablePtr->buckets = tablePtr->staticBuckets;
    tablePtr->staticBuckets[0] = tablePtr->staticBuckets[1] = 0;
    tablePtr->staticBuckets[2] = tablePtr->staticBuckets[3] = 0;
    tablePtr->numBuckets = SMALL_HASH_TABLE;
    tablePtr->numEntries = 0;
    tablePtr->rebuildSize = SMALL_HASH_TABLE*REBUILD_MULTIPLIER;
    tablePtr->downShift = 28;
    tablePtr->mask = 3;
    tablePtr->stat = 0;
    tablePtr->lk = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteHashEntry --
 *
 *	Remove a single entry from a hash table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entry given by entryPtr is deleted from its table and
 *	should never again be used by the caller.
 *
 *----------------------------------------------------------------------
 */

void
DeleteHashEntry(entryPtr)
    HashEntry *entryPtr;
{
    register HashEntry *prevPtr;

    if (entryPtr->tablePtr->stat)
	die("attempt to delete key from static hash");
    if (*entryPtr->bucketPtr == entryPtr) {
	*entryPtr->bucketPtr = entryPtr->nextPtr;
    } else {
	for (prevPtr = *entryPtr->bucketPtr; ; prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		die("malformed bucket chain in DeleteHashEntry");
	    }
	    if (prevPtr->nextPtr == entryPtr) {
		prevPtr->nextPtr = entryPtr->nextPtr;
		break;
	    }
	}
    }
    entryPtr->tablePtr->numEntries--;
    free_val(&(entryPtr->t));
    free((char *) entryPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteHashTable --
 *
 *	Free up everything associated with a hash table except for
 *	the record for the table itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table is no longer useable.
 *
 *----------------------------------------------------------------------
 */

void
DeleteHashTable(tablePtr)
    register HashTable *tablePtr;		/* Table to delete. */
{
    register HashEntry *hPtr, *nextPtr;
    int i;

    if (tablePtr->stat)
	die("attempt to delete static hash table");

    /*
     * Free up all the entries in the table.
     */

    for (i = 0; i < tablePtr->numBuckets; i++) {
	hPtr = tablePtr->buckets[i];
	while (hPtr != NULL) {
	    nextPtr = hPtr->nextPtr;
	    free_val(&(hPtr->t));
	    free((char *) hPtr);
	    hPtr = nextPtr;
	}
    }

    /*
     * Free up the bucket array, if it was dynamically allocated.
     */

    if (tablePtr->buckets != tablePtr->staticBuckets) {
	free((char *) tablePtr->buckets);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FirstHashEntry --
 *
 *	Locate the first entry in a hash table and set up a record
 *	that can be used to step through all the remaining entries
 *	of the table.
 *
 * Results:
 *	The return value is a pointer to the first entry in tablePtr,
 *	or NULL if tablePtr has no entries in it.  The memory at
 *	*searchPtr is initialized so that subsequent calls to
 *	NextHashEntry will return all of the entries in the table,
 *	one at a time.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HashEntry *
FirstHashEntry(tablePtr, searchPtr)
    HashTable *tablePtr;		/* Table to search. */
    HashSearch *searchPtr;		/* Place to store information about
					 * progress through the table. */
{
    searchPtr->tablePtr = tablePtr;
    searchPtr->nextIndex = 0;
    searchPtr->nextEntryPtr = NULL;
    return NextHashEntry(searchPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * NextHashEntry --
 *
 *	Once a hash table enumeration has been initiated by calling
 *	FirstHashEntry, this procedure may be called to return
 *	successive elements of the table.
 *
 * Results:
 *	The return value is the next entry in the hash table being
 *	enumerated, or NULL if the end of the table is reached.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HashEntry *
NextHashEntry(searchPtr)
    register HashSearch *searchPtr;	/* Place to store information about
					 * progress through the table.  Must
					 * have been initialized by calling
					 * FirstHashEntry. */
{
    HashEntry *hPtr;

    while (searchPtr->nextEntryPtr == NULL) {
	if (searchPtr->nextIndex >= searchPtr->tablePtr->numBuckets) {
	    return NULL;
	}
	searchPtr->nextEntryPtr =
		searchPtr->tablePtr->buckets[searchPtr->nextIndex];
	searchPtr->nextIndex++;
    }
    hPtr = searchPtr->nextEntryPtr;
    searchPtr->nextEntryPtr = hPtr->nextPtr;
    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * HashStats --
 *
 *	Return statistics describing the layout of the hash table
 *	in its hash buckets.
 *
 * Results:
 *	The return value is a malloc-ed string containing information
 *	about tablePtr.  It is the caller's responsibility to free
 *	this string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
HashStats(tablePtr)
    HashTable *tablePtr;		/* Table for which to produce stats. */
{
#define NUM_COUNTERS 10
    int count[NUM_COUNTERS], overflow, i, j;
    double average, tmp;
    register HashEntry *hPtr;
    char *result, *p;

    /*
     * Compute a histogram of bucket usage.
     */

    for (i = 0; i < NUM_COUNTERS; i++) {
	count[i] = 0;
    }
    overflow = 0;
    average = 0.0;
    for (i = 0; i < tablePtr->numBuckets; i++) {
	j = 0;
	for (hPtr = tablePtr->buckets[i]; hPtr != NULL; hPtr = hPtr->nextPtr) {
	    j++;
	}
	if (j < NUM_COUNTERS) {
	    count[j]++;
	} else {
	    overflow++;
	}
	tmp = j;
	average += (tmp+1.0)*(tmp/tablePtr->numEntries)/2.0;
    }

    /*
     * Print out the histogram and a few other pieces of information.
     */

    result = (char *) malloc((unsigned) ((NUM_COUNTERS*60) + 300));
    sprintf(result, "%d entries in table, %d buckets\n",
	    tablePtr->numEntries, tablePtr->numBuckets);
    p = result + strlen(result);
    for (i = 0; i < NUM_COUNTERS; i++) {
	sprintf(p, "number of buckets with %d entries: %d\n",
		i, count[i]);
	p += strlen(p);
    }
    sprintf(p, "number of buckets with %d or more entries: %d\n",
	    NUM_COUNTERS, overflow);
    p += strlen(p);
    sprintf(p, "average search distance for entry: %.1f", average);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * HashString --
 *
 *	Compute a one-word summary of a text string, which can be
 *	used to generate a hash index.
 *
 * Results:
 *	The return value is a one-word summary of the information in
 *	string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
HashString(string)
    register CONST char *string;/* String from which to compute hash value. */
{
    register unsigned int result;
    register int c;

    /*
     * I tried a zillion different hash functions and asked many other
     * people for advice.  Many people had their own favorite functions,
     * all different, but no-one had much idea why they were good ones.
     * I chose the one below (multiply by 9 and add new character)
     * because of the following reasons:
     *
     * 1. Multiplying by 10 is perfect for keys that are decimal strings,
     *    and multiplying by 9 is just about as good.
     * 2. Times-9 is (shift-left-3) plus (old).  This means that each
     *    character's bits hang around in the low-order bits of the
     *    hash value for ever, plus they spread fairly rapidly up to
     *    the high-order bits to fill out the hash value.  This seems
     *    works well both for decimal and non-decimal strings.
     */

    result = 0;
    while (1) {
	c = *string;
	string++;
	if (c == 0) {
	    break;
	}
	result += (result<<3) + c;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * FindHashEntry --
 *
 *	Given a hash table with string keys, and a string key, find
 *	the entry with a matching key.
 *
 * Results:
 *	The return value is a token for the matching entry in the
 *	hash table, or NULL if there was no matching entry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HashEntry *
FindHashEntry(tablePtr, key)
    HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find matching entry. */
{
    register HashEntry *hPtr;
    register CONST char *p1, *p2;
    int index;

    index = HashString(key) & tablePtr->mask;

    /*
     * Search all of the entries in the appropriate bucket.
     */

    for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	    hPtr = hPtr->nextPtr) {
	for (p1 = key, p2 = hPtr->key; ; p1++, p2++) {
	    if (*p1 != *p2) {
		break;
	    }
	    if (*p1 == '\0') {
		return hPtr;
	    }
	}
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateHashEntry --
 *
 *	Given a hash table with string keys, and a string key, find
 *	the entry with a matching key.  If there is no matching entry,
 *	then create a new entry that does match.
 *
 * Results:
 *	The return value is a pointer to the matching entry.  If this
 *	is a newly-created entry, then *newPtr will be set to a non-zero
 *	value;  otherwise *newPtr will be set to 0.  If this is a new
 *	entry the value stored in the entry will initially be 0.
 *
 * Side effects:
 *	A new entry may be added to the hash table.
 *
 *----------------------------------------------------------------------
 */

HashEntry *
CreateHashEntry(tablePtr, key, newPtr)
    HashTable *tablePtr;	/* Table in which to lookup entry. */
    CONST char *key;		/* Key to use to find or create matching
				 * entry. */
    int *newPtr;		/* Store info here telling whether a new
				 * entry was created. */
{
    register HashEntry *hPtr;
    register CONST char *p1, *p2;
    int index;

    index = HashString(key) & tablePtr->mask;

    /*
     * Search all of the entries in this bucket.
     */

    for (hPtr = tablePtr->buckets[index]; hPtr != NULL;
	    hPtr = hPtr->nextPtr) {
	for (p1 = key, p2 = hPtr->key; ; p1++, p2++) {
	    if (*p1 != *p2) {
		break;
	    }
	    if (*p1 == '\0') {
		*newPtr = 0;
		return hPtr;
	    }
	}
    }

    if (tablePtr->stat)
    	die("attempt to add key to static hash table");

    /*
     * Entry not found.  Add a new one to the bucket.
     */

    *newPtr = 1;
    hPtr = (HashEntry *) malloc((unsigned)
	    (sizeof(HashEntry) + strlen(key) - (sizeof(hPtr->key) - 1) ));
    hPtr->tablePtr = tablePtr;
    hPtr->bucketPtr = &(tablePtr->buckets[index]);
    hPtr->nextPtr = *hPtr->bucketPtr;
    hPtr->t.flag = TK_UNDEF;
    hPtr->flag = 0;
    strcpy(hPtr->key, key);
    *hPtr->bucketPtr = hPtr;
    tablePtr->numEntries++;

    /*
     * If the table has exceeded a decent size, rebuild it with many
     * more buckets.
     */

    if (tablePtr->numEntries >= tablePtr->rebuildSize) {
	RebuildTable(tablePtr);
    }
    return hPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * RebuildTable --
 *
 *	This procedure is invoked when the ratio of entries to hash
 *	buckets becomes too large.  It creates a new table with a
 *	larger bucket array and moves all of the entries into the
 *	new table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory gets reallocated and entries get re-hashed to new
 *	buckets.
 *
 *----------------------------------------------------------------------
 */

static void
RebuildTable(tablePtr)
    register HashTable *tablePtr;	/* Table to enlarge. */
{
    int oldSize, count, index;
    HashEntry **oldBuckets;
    register HashEntry **oldChainPtr, **newChainPtr;
    register HashEntry *hPtr;

    oldSize = tablePtr->numBuckets;
    oldBuckets = tablePtr->buckets;

    /*
     * Allocate and initialize the new bucket array, and set up
     * hashing constants for new array size.
     */

    tablePtr->numBuckets *= 4;
    tablePtr->buckets = (HashEntry **) malloc((unsigned)
	    (tablePtr->numBuckets * sizeof(HashEntry *)));
    for (count = tablePtr->numBuckets, newChainPtr = tablePtr->buckets;
	    count > 0; count--, newChainPtr++) {
	*newChainPtr = NULL;
    }
    tablePtr->rebuildSize *= 4;
    tablePtr->downShift -= 2;
    tablePtr->mask = (tablePtr->mask << 2) + 3;

    /*
     * Rehash all of the existing entries into the new bucket array.
     */

    for (oldChainPtr = oldBuckets; oldSize > 0; oldSize--, oldChainPtr++) {
	for (hPtr = *oldChainPtr; hPtr != NULL; hPtr = *oldChainPtr) {
	    *oldChainPtr = hPtr->nextPtr;
	    index = HashString(hPtr->key) & tablePtr->mask;
	    hPtr->bucketPtr = &(tablePtr->buckets[index]);
	    hPtr->nextPtr = *hPtr->bucketPtr;
	    *hPtr->bucketPtr = hPtr;
	}
    }

    /*
     * Free up the old bucket array, if it was dynamically allocated.
     */

    if (oldBuckets != tablePtr->staticBuckets) {
	free((char *) oldBuckets);
    }
}
