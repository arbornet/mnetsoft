/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */


#include "common.h"

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "nextuid.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


#if !defined(NEXTUID_TEXT) && !defined(NEXTUID_SQL)
#define NEXTUID_TEXT
#endif


/* NEXT_UID - Return the next available uid number.  If <inc> is true,
 * increment it.
 */

#ifdef NEXTUID_TEXT

char *showopt_nextuid_module="file (file="NEXT_UID_FILE")";

#include "lock.h"

uid_t next_uid(int inc)
{
    FILE *uid_fp;
    uid_t uid;

    /* Get a lock on the next_uid file */
    if ((uid_fp= fopen(NEXT_UID_FILE,"r+")) == NULL)
	die("cannot open next-uid-file "NEXT_UID_FILE);
    lock_exclusive(fileno(uid_fp),NEXT_UID_FILE);

    /* Get the uid number to use */
    if (fscanf(uid_fp,sizeof(uid_t)==sizeof(short)?"%hd":"%d",&uid) != 1)
    {
	fclose(uid_fp);
	unlock(NEXT_UID_FILE);
	die("cannot read next uid from "NEXT_UID_FILE);
    }

    if (inc)
    {
	/* Increment the uid number file */
	rewind(uid_fp);
	fprintf(uid_fp,"%d\n",uid+1);
    }

    /* Release the file */
    fclose(uid_fp);
    unlock(NEXT_UID_FILE);

    return uid;
}
#endif /* NEXTUID_TEXT */


#ifdef NEXTUID_SQL

char *showopt_nextuid_module="sql";

#include "sql.h"
#include "sqlqry.h"
#include "sqlutil.h"

uid_t next_uid(int inc)
{
    SQLhandle sqlh;
    int nrow;

    make_sql_connection();

    if (inc)
    	sqlh= qry_uid_next();
    else
    	sqlh= qry_uid_curr();

    if ((nrow= sql_nrows(sqlh)) != 1)
    	die("next_uid() got %d results - expected exactly one", nrow);

    return atoi(sql_fetchstr(sqlh, 0));
}
#endif /* NEXTUID_SQL */
