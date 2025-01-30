/* Copyright 1999, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"
#include <errno.h>
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* DBM access routines.  These are implementations of simple DBM operations
 * for each of the different dbm libraries supported by Backtalk.  At most
 * one of the following should be defined:
 *
 *   HASH_ODBM - Old-style dbm library 
 *               open files with dbminit()
 *   HASH_NDBM - New-style dbm library 
 *               open files with dbm_open()
 *   HASH_GDBM - Gnu dbm library
 *               open files with gdbm_open()
 *   HASH_DB   - Berkeley DB version 1 or 2 or 3 or 4
 *               open files with dbopen() or db_open() or DB->open()
 *
 * The following random-access functions are implemented:
 *
 * storedbm(filename,key,keysize,content,contentsize,create)
 *   Store the given key/content pair in the named dbm file.  If create is
 *   true, the file will be created with mode 0600 if it does not exist.
 *   Returns 0 on success, 1 if the file couldn't be opened to write (or
 *   created).
 *
 * fetchdbm(filename,key,keysive,buf,maxbufsize)
 *   Fetch the content associated with a key in the named dbm file.  If
 *   successful, the result is copied into the given buffer, and terminated
 *   with a nil.  If the content is longer than maxbufsize-1, it is truncated.
 *   If there are nulls in the content, you may have problems figuring out
 *   where it really ended.  If successful, returns 0.  If the file could not
 *   be opened to read, returns 1.  If the key was not found, buf[0] is set
 *   to nil, and -1 is returned.
 *
 * deletedbm(filename,key,keysize,contentflag)
 *   Remove an item from the named dbm file.  If contentflag is false, it
 *   removes the item with that key.  If it is true, it removes the first
 *   item found with that content.  If successful, returns 0.  If the file
 *   could not be opened to write, returns 1.  If the item to be deleted
 *   was not found, returns -1.
 *
 * The following sequential-access functions are implemented:
 *
 * walkdbm(file)
 *   Select a dbm file to make a sequential walk through.  Returns 0 on
 *   success, 1 if the dbm file could not be opened to read.  If file is
 *   NULL, just close any open descriptors, ending the walk.
 *
 * firstdbm(key,maxkeysize)
 *   Return the first key (not content) in the database.  Must be preceeded
 *   by a call to walkdbm().  Returns 0 on success.  If there are no keys
 *   in the database, returns -1 and sets key[0] to nil.
 *
 * nextdbm(key,maxkeysize)
 *   Return the next key (not content) in the database.  Must be preceeded
 *   by a call to firstdbm() or seekdbm().  Returns 0 on success.  If there
 *   are no more keys in the database, returns -1 and sets key[0] to nil.
 *   Arbitrary calls to fetchdbm(), storedbm(), and deletedbm() may be mixed
 *   in between nextdbm() calls, and should NOT change our current position
 *   in the walk.
 *
 * seekdbm(key,keysize)
 *   Set things up so that the next call to nextdbm will return the next
 *   key after the given key.  Returns 0 on success.  Returns -1 if the
 *   key is not found in the database.
 *
 * None of these functions allow duplicate keys to be created in the database.
 * 
 * In addition, the following support function is provided.
 *
 * errdbm()
 *  Return an error message after one of the calls above failed.
 */

/* Normally we don't want dbm files (expecially the passwd one) publicly
 * readable, but in suExec installations they must be.
 */
#ifdef READABLE_PASSWD
# define DBM_MODE 0644
#else
# define DBM_MODE 0600
#endif

/* We support both version 1 and version 2 of Berkeley DB 
 * Figure out which we actually have.
 */
#ifdef HASH_DB
# include <db.h>
# if !defined(DB_VERSION_MAJOR) || (DB_VERSION_MAJOR == 1)
#  define HASH_DB_1
# else
#  if DB_VERSION_MAJOR == 2
#   define HASH_DB_2
#  else
#   if DB_VERSION_MAJOR == 3
#    define HASH_DB_3
#   else
#    define HASH_DB_4
#   endif
#  endif
# endif
#endif /* HASH_DB */


/* Header files for the ones that need setlastkey() */
#ifdef HASH_GDBM
#include <gdbm.h>
#endif
#ifdef HASH_ODBM
#include <dbm.h>
#endif


#if defined(HASH_ODBM) || defined(HASH_GDBM)

/* gdbm and old dbm both do walks with a function that returns the successor
 * key of the last key read.  This code maintains the last key in a global
 * variable using malloced memory.  Need to be sure to initialize the
 * lastkey.dptr field to NULL.
 */

datum lastkey;			/* last key read in current walk */

/* SETLASTKEY - set the global lastkey variable to the given key.
 */

void setlastkey(datum key)
{
    /* Free any old value */
    if (lastkey.dptr != NULL) free(lastkey.dptr);

    /* Copy in the new value, allocating fresh memory */
    lastkey.dsize= key.dsize;
    if (key.dptr == NULL)
    	lastkey.dptr= NULL;
    else
    {
	lastkey.dptr= (char *)malloc(key.dsize);
	memmove(lastkey.dptr,key.dptr,key.dsize);
    }
}

int seekdbm(char *k, int klen)
{
    /* Free any old value */
    if (lastkey.dptr != NULL) free(lastkey.dptr);

    /* Copy in the new value, allocating fresh memory */
    lastkey.dsize= klen;
    lastkey.dptr= (char *)malloc(klen);
    memmove(lastkey.dptr,k,klen);

    return 0;
}

#endif /* HASH_ODBM || HASH_GDBM */



/* ============================ Old dbm version ============================ */


#ifdef HASH_ODBM
char *showopt_hash_lib="old dbm";

/* Does anyone still use this?  It only allows one open database at a time.
 * Some man pages don't mention dbmclose(), but it seems to exist everywhere
 * I've seen this library, so we'll go ahead and use it.  The storedbm(),
 * fetchdbm(), and deletedbm() functions close the dbm file after they are
 * done with it, but the walk routines mostly don't.  We do some extra
 * bookkeeping to keep track of this.
 */

char *open_name= NULL;	/* Name of any dbm file left open */
char *walk_name= NULL;	/* Name of any dbm file being walked */

void close_dbm()   /* ODBM */
{
    if (open_name != NULL)
    {
    	dbmclose();
    	free(open_name);
    	open_name= NULL;
    }
}

int open_dbm(char *file, int create)   /* ODBM */
{
    int omask;

    /* Check if it is already open */
    if (open_name != NULL)
    {
    	if (strcmp(open_name,file) == 0)
    	    return 0;
        close_dbm();
    }

    /* Open database */
    omask= umask(077);
    if (dbminit(file))
    {
	/* Open failed -  most versions of old dbm don't automatically create
	 * files, so the failure might be because it doesn't exist, in which
	 * case we have to create the files manually.
	 */

	if (create)
	{
	    int namelen, fd, rc;
	    char *f;

	    /* Create the .dir file */
	    namelen= strlen(file);
	    f= (char *)malloc(namelen+6);
	    strcpy(f,file);
	    strcpy(f+namelen,".dir");
	    if ((fd= open(f, O_CREAT|O_EXCL, DBM_MODE)) > 0)
		close(fd);
	    else
		return 1;

	    /* Create the .pag file */
	    strcpy(f_namelen,".pag");
	    if ((fd= open(f, O_CREAT|O_EXCL, DBM_MODE)) > 0)
		close(fd);
	    else
		return 1;

	    free(f);

	    /* Try opening it again now */
	    if (dbminit(file))
		return 1;
	}
	else
	    return 1;
    }
    umask(omask);

    /* We got it open - remember that */
    open_name= strdup(file);

    return 0;
}

int storedbm(char *file, char *k, int klen, char *c, int clen, int create)
{
    datum key, content;

    /* Open database */
    if (open_dbm(file,create))
        return 1;

    /* Set up the key */
    key.dptr= k;
    key.dsize= klen;

    /* Set up the content */
    content.dptr= c;
    content.dsize= clen;

    /* Store it */
    store(key,content);

    /* Close database */
    close_dbm();

    return 0;
}

int fetchdbm(char *file, char *k, int klen, char *c, int cmax)   /* ODBM */
{
    datum key, content;

    if (cmax > 1) *c= '\0';

    /* Open database */
    if (open_dbm(file,0))
        return 1;

    /* Set up the key */
    key.dptr= k;
    key.dsize= klen;

    /* Fetch it */
    content= fetch(key);


    /* Check if it was found */
    if (content.dptr == NULL)
    {
        close_dbm();
	return -1;
    }

    /* Copy the result into the buffer */
    if (content.dsize >= cmax) content.dsize= cmax-1;
    strncpy(c, content.dptr, content.dsize);
    if (cmax > 1) c[content.dsize]= '\0';
    /* content is in static memory, so we don't have to free it */

    close_dbm();
    return 0;
}

int deletedbm(char *file, char *k, int klen, int bycontent)   /* ODBM */
{
    datum key;
    int rc;

    /* Open database */
    if (open_dbm(file,0))
        return 1;

    if (bycontent)
    {
    	datum content;
    	/* Search for a key whose content matches */
    	for (key= firstkey(); key.dptr != NULL; key= nextkey(key))
    	{
    	    content= fetch(key);
    	    if (content.dsize == klen && !memcmp(content.dptr, k, klen))
    	    	break;
    	}
    }
    else
    {
	/* Set up the key */
	key.dptr= k;
	key.dsize= klen;
    }

    /* Delete it */
    rc= (key.dptr == NULL) ? 1 : delete(key);

    /* Close database */
    close_dbm();

    return rc ? -1 : 0;
}

int walkdbm(char *file)   /* ODBM */
{
    /* Close any previously open dbm file (if it's not the same one) */
    if (walk_name != NULL)
    {
    	if (file != NULL && strcmp(file,walk_name) == 0)
    	    return 0;

    	close_dbm();

    	free(walk_name);
    	walk_name= NULL;

    	if (lastkey.dptr != NULL) free(lastkey.dptr);
	lastkey.dptr= NULL;
    }

    /* Open a new dbm file */
    if (file != NULL)
    {
	if (open_dbm(walk_name,0))
	    return 1;
	walk_name= strdup(file);
	lastkey.dptr= NULL;
    }

    return 0;
}

int firstdbm(char *k, int kmax)   /* ODBM */
{
    datum key;

    /* Open database */
    if (walk_name == NULL || open_dbm(walk_name,0))
        return 1;

    /* Get the first key */
    setlastkey(key= firstkey());

    /* Check if we found it */
    if (key.dptr == NULL)
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.dsize >= kmax) key.dsize= kmax-1;
    strncpy(k, key.dptr, key.dsize);
    if (kmax > 1) k[key.dsize]= '\0';
    /* key is in static memory, so we don't have to free it */

    return 0;
}


int nextdbm(char *k, int kmax)   /* ODBM */
{
    datum key;

    /* Open database */
    if (walk_name == NULL || open_dbm(walk_name,0))
        return 1;

    /* Already at EOF? */
    if (lastkey.dptr == NULL)
    	return -1;

    /* Get the first key */
    setlastkey(key= nextkey(lastkey));

    /* Check if we found it */
    if (key.dptr == NULL)
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.dsize >= kmax) key.dsize= kmax-1;
    strncpy(k, key.dptr, key.dsize);
    if (kmax > 1) k[key.dsize]= '\0';
    /* key is in static memory, so we don't have to free it */

    return 0;
}


char *errdbm()
{
    return "?";
}

#endif /* HASH_ODBM */


/* ============================= ndbm version ============================= */

/* NDBM variations abound!  Traditionally the library was named -ldbm and
 * the header file was ndbm.h, but these vary.  Redhat 6.1 has the following
 * three ndbm libraries installed:
 *
 * GDBM Emulation of NDBM:
 *   Library:  -lgdbm
 *   Header:   #include <gdbm/ndbm.h>
 *
 * Berkeley DB Version 1 of NDBM (included in glibc2.1):
 *   Library:  -ldb1
 *   Header:   #include <db1/ndbm.h>
 *
 * Berkeley DB Version 2 of NDBM:
 *   Library:  -ldb or -lndbm
 *   Header:   #define DB_DBM_HSEARCH 1
 *             #include <db.h>
 *
 * The important things here are to (1) get the right library with the right
 * header and (2) link Backtalk with the same one that mod_auth_dbm uses.
 */


#ifdef HASH_NDBM

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef DB_EMULATES_NDBM
  char *showopt_hash_lib="Berkeley DB Version 2 emulation of ndbm";
# define DB_DBM_HSEARCH 1
# include <db.h>
#else
# ifdef HAVE_GDBM_NDBM_H
   char *showopt_hash_lib="Gnu DBM emulation of ndbm";
#  include <gdbm/ndbm.h>
# else
#  ifdef HAVE_DB1_NDBM_H
    char *showopt_hash_lib="Berkeley DB Version 1 emulation of ndbm";
#   include <db1/ndbm.h>
#  else
    char *showopt_hash_lib="ndbm";
#   include <ndbm.h>
#  endif
# endif
#endif

#include <errno.h>

DBM *walk_db= NULL;	/* descriptor for dbm being walked */

int storedbm(char *file, char *k, int klen, char *c, int clen, int create)
{
    datum key, content;
    DBM *db;

    /* Open database */
    if ((db= dbm_open(file, create ? O_RDWR|O_CREAT : O_RDWR,
                      DBM_MODE)) == NULL)
        return 1;

    /* Set up the key */
    key.dptr= k;
    key.dsize= klen;

    /* Set up the content */
    content.dptr= c;
    content.dsize= clen;

    /* Store it */
    dbm_store(db,key,content,DBM_REPLACE);

    /* Close database */
    dbm_close(db);

    return 0;
}

int fetchdbm(char *file, char *k, int klen, char *c, int cmax)    /* NDBM */
{
    datum key, content;
    DBM *db;

    if (cmax > 1) *c= '\0';

    /* Open database */
    if ((db= dbm_open(file, O_RDONLY, DBM_MODE)) == NULL)
        return 1;

    /* Set up the key */
    key.dptr= k;
    key.dsize= klen;

    /* Fetch it */
    content= dbm_fetch(db,key);

    /* Check if it was found */
    if (content.dptr == NULL)
    {
        dbm_close(db);
	return -1;
    }

    /* Copy the result into the buffer */
    if (content.dsize >= cmax) content.dsize= cmax-1;
    strncpy(c, content.dptr, content.dsize);
    if (cmax > 1) c[content.dsize]= '\0';
    /* content is in static memory, so we don't have to free it */

    dbm_close(db);
    return 0;
}

int deletedbm(char *file, char *k, int klen, int bycontent)    /* NDBM */
{
    datum key;
    DBM *db;
    int rc;

    /* Open database */
    if ((db= dbm_open(file, O_RDWR, DBM_MODE)) == NULL)
        return 1;

    if (bycontent)
    {
    	datum content;
    	/* Search for a key whose content matches */
    	for (key= dbm_firstkey(db); key.dptr != NULL; key= dbm_nextkey(db))
    	{
    	    content= dbm_fetch(db,key);
	    if (content.dsize == klen && !memcmp(content.dptr, k, klen))
	    	break;
    	}
    }
    else
    {
	/* Set up the key */
	key.dptr= k;
	key.dsize= klen;
    }

    /* Delete it */
    rc= (key.dptr == NULL) ? 1 : dbm_delete(db,key);

    /* Close database */
    dbm_close(db);

    return rc ? -1 : 0;
}

int walkdbm(char *file)    /* NDBM */
{
    if (file == NULL)
    {
    	if (walk_db != NULL)
	    dbm_close(walk_db);
    }
    else
    {
	if ((walk_db= dbm_open(file, O_RDONLY, DBM_MODE)) == NULL)
	    return 1;
    }
    return 0;
}

int firstdbm(char *k, int kmax)    /* NDBM */
{
    datum key;

    if (walk_db == NULL)
        return 1;

    /* Get the first key */
    key= dbm_firstkey(walk_db);

    /* Check if we found it */
    if (key.dptr == NULL)
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.dsize >= kmax) key.dsize= kmax-1;
    strncpy(k, key.dptr, key.dsize);
    if (kmax > 1) k[key.dsize]= '\0';
    /* key is in static memory, so we don't have to free it */

    return 0;
}

int seekdbm(char *k, int klen)    /* NDBM */
{
    datum tmpkey;

    if (walk_db == NULL)
        return 1;

    /* ndbm doesn't seem to have a nice seek function, so we just walk
     * through the database until we find what we want.
     */
    tmpkey= dbm_firstkey(walk_db);
    for (;;)
    {
    	if (tmpkey.dptr == NULL) return -1;
    	if (tmpkey.dsize == klen && !memcmp(tmpkey.dptr, k, klen))
    	    return 0;
        tmpkey= dbm_nextkey(walk_db);
    }
}

int nextdbm(char *k, int kmax)    /* NDBM */
{
    datum key;

    if (walk_db == NULL)
        return 1;

    /* Get the next key */
    key= dbm_nextkey(walk_db);

    /* Check if we found it */
    if (key.dptr == NULL)
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.dsize >= kmax) key.dsize= kmax-1;
    strncpy(k, key.dptr, key.dsize);
    if (kmax > 1) k[key.dsize]= '\0';
    /* key is in static memory, so we don't have to free it */

    return 0;
}

char *errdbm()
{
    return "?";
}


#endif /* HASH_NDBM */

/* ============================= gdbm version ============================= */


#ifdef HASH_GDBM
char *showopt_hash_lib= "Gnu DBM";

GDBM_FILE walk_db= NULL;	/* descriptor for dbm being walked */

int storedbm(char *file, char *k, int klen, char *c, int clen, int create)
{
    datum key, content;
    GDBM_FILE db;

    /* Open database */
    if ((db= gdbm_open(file, 0, create ? GDBM_WRCREAT : GDBM_WRITER,
    	               DBM_MODE, 0)) == NULL)
        return 1;

    /* Set up the key */
    key.dptr= k;
    key.dsize= klen;

    /* Set up the content */
    content.dptr= c;
    content.dsize= clen;

    /* Store it */
    gdbm_store(db,key,content,GDBM_REPLACE);

    /* Close database */
    gdbm_close(db);

    return 0;
}

int fetchdbm(char *file, char *k, int klen, char *c, int cmax)   /* GDBM */
{
    datum key, content;
    GDBM_FILE db;

    if (cmax > 1) *c= '\0';

    /* Open database */
    if ((db= gdbm_open(file, 0, GDBM_READER, DBM_MODE, 0)) == NULL)
        return 1;

    /* Set up the key */
    key.dptr= k;
    key.dsize= klen;

    /* Fetch it */
    content= gdbm_fetch(db,key);


    /* Check if it was found */
    if (content.dptr == NULL)
    {
        gdbm_close(db);
	return -1;
    }

    /* Copy the result into the buffer */
    if (content.dsize >= cmax) content.dsize= cmax-1;
    strncpy(c, content.dptr, content.dsize);
    if (cmax > 1) c[content.dsize]= '\0';
    free(content.dptr);

    gdbm_close(db);
    return 0;
}

int deletedbm(char *file, char *k, int klen, int bycontent)   /* GDBM */
{
    datum key;
    GDBM_FILE db;
    int rc;

    /* Open database */
    if ((db= gdbm_open(file, 0, GDBM_WRITER, DBM_MODE, 0)) == NULL)
        return 1;

    if (bycontent)
    {
    	datum content, nkey;
    	/* Search for a key whose content matches */
    	for (key= gdbm_firstkey(db); key.dptr != NULL;
    	     nkey= gdbm_nextkey(db,key), free(key.dptr), key= nkey)
    	{
    	    content= gdbm_fetch(db,key);
    	    if (content.dptr != NULL)
    	    {
		if (content.dsize == klen && !memcmp(content.dptr, k, klen))
		    break;
	    	free(content.dptr);
	    }
	}
    }
    else
    {
	/* Set up the key */
	key.dptr= k;
	key.dsize= klen;
    }

    /* Delete it */
    rc= (key.dptr == NULL) ? 1 : gdbm_delete(db,key);

    /* Close database */
    gdbm_close(db);

    return rc ? -1 : 0;
}

int walkdbm(char *file)   /* GDBM */
{
    /* Close any previously open walk file */
    if (walk_db != NULL)
    {
	gdbm_close(walk_db);
	walk_db= NULL;

	if (lastkey.dptr != NULL) free(lastkey.dptr);
	lastkey.dptr= NULL;
    }

    /* Open the new file to walk */
    if (file != NULL)
    {
	if ((walk_db= gdbm_open(file, 0, GDBM_READER, DBM_MODE, 0)) == NULL)
	    return 1;
	lastkey.dptr= NULL;
    }

    return 0;
}

int firstdbm(char *k, int kmax)   /* GDBM */
{
    datum key;

    if (walk_db == NULL)
        return 1;

    /* Get the first key */
    setlastkey(key= gdbm_firstkey(walk_db));

    /* Check if we found it */
    if (key.dptr == NULL)
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.dsize >= kmax) key.dsize= kmax-1;
    strncpy(k, key.dptr, key.dsize);
    if (kmax > 1) k[key.dsize]= '\0';
    free(key.dptr);

    return 0;
}

int nextdbm(char *k, int kmax)   /* GDBM */
{
    datum key;

    if (walk_db == NULL)
        return 1;

    /* Get the next key */
    setlastkey(key= gdbm_nextkey(walk_db,lastkey));

    /* Check if we found it */
    if (key.dptr == NULL)
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.dsize >= kmax) key.dsize= kmax-1;
    strncpy(k, key.dptr, key.dsize);
    if (kmax > 1) k[key.dsize]= '\0';
    free(key.dptr);

    return 0;
}


char *errdbm()
{
    return (char *)gdbm_strerror(gdbm_errno);
}

#endif /* HASH_GDBM */


/* ======================== Berkeley db version 1 ======================== */


#ifdef HASH_DB_1
char *showopt_hash_lib= "Berkeley DB version 1";

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif


DB *walk_dbp= NULL;	/* descriptor for dbm being walked */

int storedbm(char *file, char *k, int klen, char *c, int clen, int create)
{
    DBT key, content;
    DB *dbp;

    /* Open database */
    if ((dbp= dbopen(file, create ? O_RDWR|O_CREAT : O_RDWR,
                     DBM_MODE, DB_HASH, NULL)) == NULL)
        return 1;

    /* Set up the key */
    key.data= k;
    key.size= klen;

    /* Set up the content */
    content.data= c;
    content.size= clen;

    /* Store it */
    dbp->put(dbp, &key, &content, 0);

    /* Close database */
    dbp->close(dbp);

    return 0;
}

int fetchdbm(char *file, char *k, int klen, char *c, int cmax)   /* DB 1 */
{
    DBT key, content;
    DB *dbp;
    int rc;

    if (cmax > 1) *c= '\0';

    /* Open database */
    if ((dbp= dbopen(file, O_RDONLY, DBM_MODE, DB_HASH, NULL)) == NULL)
        return 1;

    /* Set up the key */
    key.data= k;
    key.size= klen;

    /* Fetch it */
    rc= dbp->get(dbp, &key, &content, 0);

    if (rc)
    {
	dbp->close(dbp);
    	return -1;
    }

    /* Copy the result into the buffer */
    if (content.size >= cmax) content.size= cmax-1;
    strncpy(c, content.data, content.size);
    if (cmax > 1) c[content.size]= '\0';
    /* apparantly you don't have to free data */

    dbp->close(dbp);
    return 0;
}

int deletedbm(char *file, char *k, int klen, int bycontent)   /* DB 1 */
{
    DBT key;
    DB *dbp;
    int found;

    /* Open database */
    if ((dbp= dbopen(file, O_RDWR, DBM_MODE, DB_HASH, NULL)) == NULL)
        return 1;

    if (bycontent)
    {
	DBT content;
	int eof;

	/* Search for a key whose content matches */
	for (found= !dbp->seq(dbp, &key, &content, R_FIRST); found;
	     found= !dbp->seq(dbp, &key, &content, R_NEXT))
	{
	    if (content.size == klen && !memcmp(content.data, k, klen))
	    {
		/* Delete it */
		dbp->del(dbp, &key, R_CURSOR);
	    	break;
	    }
	}
    }
    else
    {
	/* Set up the key */
	key.data= k;
	key.size= klen;

	/* Delete it */
	found= !dbp->del(dbp, &key, 0);
    }

    /* Close database */
    dbp->close(dbp);

    return found ? 0 : -1;
}

int walkdbm(char *file)   /* DB 1 */
{
    /* Close any previously open walk file */
    if (walk_dbp != NULL)
    {
	walk_dbp->close(walk_dbp);
	walk_dbp= NULL;
    }

    /* Open the new file to walk */
    if (file != NULL)
    {
	if ((walk_dbp= dbopen(file, O_RDONLY, DBM_MODE, DB_HASH, NULL)) == NULL)
	    return 1;
    }

    return 0;
}

int firstdbm(char *k, int kmax)   /* DB 1 */
{
    DBT key, content;

    if (walk_dbp == NULL)
        return 1;

    /* Get the first key */
    if (walk_dbp->seq(walk_dbp, &key, &content, R_FIRST))
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.size >= kmax) key.size= kmax-1;
    strncpy(k, key.data, key.size);
    if (kmax > 1) k[key.size]= '\0';
    /* apparantly you don't have to free data */

    return 0;
}

int nextdbm(char *k, int kmax)   /* DB 1 */
{
    DBT key, content;

    if (walk_dbp == NULL)
        return 1;

    /* Get the next key */
    if (walk_dbp->seq(walk_dbp, &key, &content, R_NEXT))
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.size >= kmax) key.size= kmax-1;
    strncpy(k, key.data, key.size);
    if (kmax > 1) k[key.size]= '\0';
    /* apparantly you don't have to free data */

    return 0;
}

int seekdbm(char *k, int klen)   /* DB 1 */
{
    DBT key, content;

    if (walk_dbp == NULL)
        return 1;

    /* Set up the key */
    memset(&key, 0, sizeof(DBT));
    key.data= k;
    key.size= klen;

    /* Get the next key */
    if (walk_dbp->seq(walk_dbp, &key, &content, R_CURSOR))
    	return -1;
    else
	return 0;
}

char *errdbm()
{
    return strerror(errno);
}

#endif /* HASH_DB_1 */


/* ===================== Berkeley db version 2 & 3 & 4 =================== */

#if defined(HASH_DB_2) || defined(HASH_DB_3) || defined(HASH_DB_4)

#ifdef HASH_DB_2
  char *showopt_hash_lib= "Berkeley DB version 2";
#endif

#ifdef HASH_DB_3
  char *showopt_hash_lib= "Berkeley DB version 3";
#endif

#ifdef HASH_DB_4
  char *showopt_hash_lib= "Berkeley DB version 4";
#endif

/* Defines to unify API differences in various versions */

#ifdef HASH_DB_2
# define Open(n,t,f,m,d) (db_errno= db_open(n, t, f, m, NULL, NULL, &d))
#else
# if defined(HASH_DB_3) || (defined(HASH_DB_4) && (DB_VERSION_MINOR < 1))
#  define Open(n,t,f,m,d) ((db_errno= db_create(&d, NULL, 0)) || (db_errno= d->open(d, n, NULL, t, f, m)))
# else
#  define Open(n,t,f,m,d) ((db_errno= db_create(&d, NULL, 0)) || (db_errno= d->open(d, NULL, n, NULL, t, f, m)))
# endif
#endif

#if defined(HASH_DB_1) || (defined(HASH_DB_2) && (DB_VERSION_MINOR < 6))
# define Cursor(a,b,c) cursor(a,b,c)
#else
# define Cursor(a,b,c) cursor(a,b,c,0)
#endif


DB *walk_dbp= NULL;	/* descriptor for dbm being walked */
DBC *walk_cursor= NULL;	/* cursor marking current position in walk */
static int db_errno;

int storedbm(char *file, char *k, int klen, char *c, int clen, int create)
{
    DBT key, content;
    DB *dbp;

    /* Open database */
    if (Open(file, DB_HASH, create ? DB_CREATE : 0, DBM_MODE, dbp))
        return 1;

    /* Set up the key */
    memset(&key, 0, sizeof(key));
    key.data= k;
    key.size= klen;

    /* Set up the content */
    memset(&content, 0, sizeof(content));
    content.data= c;
    content.size= clen;

    /* Store it */
    if (db_errno= dbp->put(dbp, NULL, &key, &content, 0))
        return 1;

    /* Close database */
    dbp->close(dbp,0);

    return 0;
}

void stuff(DB_ENV *dbenv, const char *errpfx, const char *msg)
{
printf("XXXX: %s %s\n",errpfx,msg);
}

int fetchdbm(char *file, char *k, int klen, char *c, int cmax)  /* DB 2&3 */
{
    DB *dbp;
    DBT key, content;

    if (cmax > 1) *c= '\0';

    /* Open database */
    if (Open(file, DB_UNKNOWN, DB_RDONLY, DBM_MODE, dbp))
        return 1;

    /* Set up the key */
    memset(&content, 0, sizeof(content));
    memset(&key, 0, sizeof(key));
    key.data= k;
    key.size= klen;

    /* Fetch it */
    db_errno= dbp->get(dbp, NULL, &key, &content, 0);

    if (db_errno)
    {
	dbp->close(dbp,0);
    	return -1;
    }

    /* Copy the result into the buffer */
    if (content.size >= cmax) content.size= cmax-1;
    strncpy(c, content.data, content.size);
    if (cmax > 1) c[content.size]= '\0';
    /* apparantly you don't have to free data */

    dbp->close(dbp,0);
    return 0;
}

int deletedbm(char *file, char *k, int klen, int bycontent)  /* DB 2&3 */
{
    DBT key;
    DB *dbp;
    int found;

    /* Open database */
    if (Open(file, DB_UNKNOWN, 0, DBM_MODE, dbp))
        return 1;

    memset(&key, 0, sizeof(key));

    if (bycontent)
    {
    	DBT content;
    	DBC *cursor;

        memset(&content, 0, sizeof(content));

    	if (db_errno= dbp->Cursor(dbp, NULL, &cursor))
	    return 1;

    	for (found= !cursor->c_get(cursor, &key, &content, DB_FIRST); found;
	     found= !cursor->c_get(cursor, &key, &content, DB_NEXT))
	{
	    if (content.size == klen && !memcmp(content.data, k, klen))
	    {
	    	/* Delete it */
	    	cursor->c_del(cursor, 0);
	    	break;
	    }
	}
    }
    else
    {
	/* Set up the key */
	memset(&key, 0, sizeof(DBT));
	key.data= k;
	key.size= klen;

	/* Delete it */
	found= !dbp->del(dbp, NULL, &key, 0);
    }

    /* Close database */
    dbp->close(dbp,0);

    return found ? 0 : -1;
}

int walkdbm(char *file)  /* DB 2&3 */
{
    /* Close any previously open walk file */
    if (walk_dbp != NULL)
    {
	walk_cursor->c_close(walk_cursor);
	walk_dbp->close(walk_dbp,0);
	walk_dbp= NULL;
    }

    /* Open the new file to walk, and set up cursor */
    if (file != NULL)
    {
	if (Open(file, DB_HASH, DB_RDONLY, DBM_MODE, walk_dbp))
	    return 1;
	if (db_errno= walk_dbp->Cursor(walk_dbp, NULL, &walk_cursor))
	    return 1;
    }

    return 0;
}

int firstdbm(char *k, int kmax)  /* DB 2&3 */
{
    DBT key, content;

    if (walk_dbp == NULL)
        return 1;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    /* Get the first key */
    if (walk_cursor->c_get(walk_cursor, &key, &content, DB_FIRST))
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.size >= kmax) key.size= kmax-1;
    strncpy(k, key.data, key.size);
    if (kmax > 1) k[key.size]= '\0';
    /* apparantly you don't have to free data */

    return 0;
}

int nextdbm(char *k, int kmax)  /* DB 2&3 */
{
    DBT key, content;

    if (walk_dbp == NULL)
        return 1;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    /* Get the next key */
    if (walk_cursor->c_get(walk_cursor, &key, &content, DB_NEXT))
    {
    	if (kmax > 1) k[0]= '\0';
    	return -1;
    }

    /* Copy the result into the buffer */
    if (key.size >= kmax) key.size= kmax-1;
    strncpy(k, key.data, key.size);
    if (kmax > 1) k[key.size]= '\0';
    /* apparantly you don't have to free data */

    return 0;
}

int seekdbm(char *k, int klen)  /* DB 2&3 */
{
    DBT key, content;

    if (walk_dbp == NULL)
        return 1;

    memset(&key, 0, sizeof(key));
    memset(&content, 0, sizeof(content));

    /* Set up the key */
    memset(&key, 0, sizeof(DBT));
    key.data= k;
    key.size= klen;

    /* Get the next key */
    if (walk_cursor->c_get(walk_cursor, &key, &content, DB_SET))
    	return -1;
    else
	return 0;
}

char *errdbm()
{
    return strerror(db_errno);
}

#endif /* HASH_DB_2 || HASH_DB_3 || HASH_DB_4 */


#if !defined(HASH_ODBM) && !defined(HASH_NDBM) && !defined(HASH_GDBM) && !defined(HASH_DB)
char *showopt_hash_lib="none";
#endif


#ifdef TEST
/* Test driver */
main(int argc, char **argv)
{
    int i,j,rc;
    int fflag= 0;
    int sflag= 0;
    int nflag= 0;
    int cflag= 0;
    int dflag= 0;
    char *dbname= NULL;
    char *key= NULL;
    char *value= NULL;
    char bf[1024];

    for (i= 1; i < argc; i++)
    {
    	if (argv[i][0] == '-')
	    for (j= 1; argv[i][j] != '\0'; j++)
		switch (argv[i][j])
		{
		case 'c':
		    cflag= 1;
		    break;
		case 'd':
		    dflag= 1;
		    break;
		case 'f':
		    fflag= 1;
		    break;
		case 'n':
		    nflag= 1;
		    break;
		case 's':
		    sflag= 1;
		    break;
		default:
		    goto usage;
		}
	else if (dbname == NULL)
	    dbname = argv[i];
	else if ((sflag || fflag || dflag) && key == NULL)
	    key = argv[i];
	else if (sflag && (value == NULL))
	    value = argv[i];
	else
	    goto usage;
    }
    if (dbname == NULL || ((dflag || fflag) && (key == NULL)) ||
        (sflag && (value == NULL)))
	goto usage;

    if (sflag)
    {
 	rc= storedbm(dbname,key,strlen(key)+nflag,value,strlen(value)+1,cflag);
	if (rc)
	    printf("Failed (%s)\n",errdbm());
	else
	    printf("OK\n");
	exit(0);
    }

    if (fflag)
    {
        rc= fetchdbm(dbname,key,strlen(key)+nflag,bf,1024);
	if (rc > 0)
	    printf("Failed (%s)\n",errdbm());
	else if (rc < 0)
	    printf("Not Found\n");
	else 
	    printf("Value: %s\n",bf);
	exit(0);
    }

    if (dflag)
    {
        rc= deletedbm(dbname,key,strlen(key)+nflag,cflag);
	if (rc > 0)
	    printf("Failed (%s)\n",errdbm());
	else if (rc < 0)
	    printf("Not Found\n");
	else 
	    printf("Deleted\n",bf);
	exit(0);
    }

    rc= walkdbm(dbname);
    if (rc > 0)
    {
	printf("walkdbm Failed (%s)\n",errdbm());
	exit(1);
    }
    rc= firstdbm(bf,1024);
    if (rc > 0)
    {
	printf("firstdbm Failed (%s)\n",errdbm());
	exit(1);
    }
    else if (rc < 0)
    {
	printf("db empty\n");
	exit(0);
    }
    do
    {   printf("Key: %s\n",bf);
    } while ((rc =nextdbm(bf,1024)) == 0);
    if (rc > 0)
    {
	printf("nextdbm Failed (%s)\n",errdbm());
	exit(1);
    }

    exit(0);

usage:
    printf("usage: %s <dbname>                        - Dump db\n",
        argv[0]);
    printf("       %s <dbname> -f [-n] <key>          - fetch entry\n",
        argv[0]);
    printf("       %s <dbname> -s [-nc] <key> <value> - save entry\n",
        argv[0]);
    printf("       %s <dbname> -d [-nc] <key>         - delete entry\n",
        argv[0]);
    exit(1);
}
#endif /*TEST*/
