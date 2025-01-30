#ifndef HASH_H
#define HASH_H

#define CONST const

/*
 * Structure definition for an entry in a hash table.  No-one outside
 * Tcl should access any of these fields directly;  use the macros
 * defined below.
 */

typedef struct HashEntry {
    struct HashEntry *nextPtr;		/* Pointer to next entry in this
					 * hash bucket, or NULL for end of
					 * chain. */
    struct HashTable *tablePtr;		/* Pointer to table containing entry. */
    struct HashEntry **bucketPtr;	/* Pointer to bucket that points to
					 * first entry in this entry's chain:
					 * used for deleting the entry. */
    Token t;				/* value of entry. */
    int flag;				/* Boolean combination of DT_* values */
    char key[MAXSYMLEN+1];		/* String for key.  The actual size
					 * will be as large as needed to hold
					 * the key. */
					/* MUST BE LAST FIELD IN RECORD!! */
} HashEntry;

/*
 * Structure definition for a hash table.  Must be in tcl.h so clients
 * can allocate space for these structures, but clients should never
 * access any fields in this structure.
 */

#define SMALL_HASH_TABLE 4
typedef struct HashTable {
    HashEntry **buckets;		/* Pointer to bucket array.  Each
					 * element points to first entry in
					 * bucket's hash chain, or NULL. */
    HashEntry *staticBuckets[SMALL_HASH_TABLE];
					/* Bucket array used for small tables
					 * (to avoid mallocs and frees). */
    int numBuckets;			/* Total number of buckets allocated
					 * at **bucketPtr. */
    int numEntries;			/* Total number of entries present
					 * in table. */
    int rebuildSize;			/* Enlarge table when numEntries gets
					 * to be this large. */
    int downShift;			/* Shift count used in hashing
					 * function.  Designed to use high-
					 * order bits of randomized keys. */
    int mask;				/* Mask value used in hashing
					 * function. */
    int stat;				/* If true, may not add or delete keys */
    int lk;				/* Number of things pointing to this */
} HashTable;

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:
 */

typedef struct HashSearch {
    HashTable *tablePtr;		/* Table being searched. */
    int nextIndex;			/* Index of next bucket to be
					 * enumerated after present one. */
    HashEntry *nextEntryPtr;		/* Next entry to be enumerated in the
					 * the current bucket. */
} HashSearch;

#define GetHashTok(h) ((h)->t)
#define GetHashFlag(h) ((h)->flag)
#define SetHashTok(h, value) ((h)->t= (value))
#define SetHashFlag(h, value) ((h)->flag= (value))
#define GetHashKey(h) (h)->key

/* Function Prototypes */

void InitHashTable(HashTable *tablePtr);
void DeleteHashEntry(HashEntry *entryPtr);
void DeleteHashTable(register HashTable *tablePtr);
HashEntry * FirstHashEntry( HashTable *tablePtr, HashSearch *searchPtr);
HashEntry * NextHashEntry(HashSearch *searchPtr);
char * HashStats(HashTable *tablePtr);
HashEntry * FindHashEntry( HashTable *tablePtr, CONST char *key);
HashEntry *CreateHashEntry( HashTable *tablePtr, CONST char *key, int *newPtr);

#endif /* HASH_H */
