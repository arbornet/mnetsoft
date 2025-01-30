/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#if ATTACHMENTS
#include "baai.h"
#include "str.h"
#include "sql.h"
#include "sqlutil.h"
#include "sqlqry.h"

#include <ctype.h>

/* Backtalk Attachment Archive Index - SQL version


/* BAAI_SET - Write an attachment index entry.  Creates a new one if it wasn't
 * there yet, overwrites the old one if it was.  Dies on failure.
 */

void baai_set(char *fname, int link, char *type, off_t size, char *user,
	uid_t uid, char *conf, int access, time_t date, char *desc)
{
    SQLhandle sqlh;
    int nrow;

    make_sql_connection();

    sqlh= qry_baai_get(fname);
    if ((nrow= sql_nrows(sqlh)) == 0)
	qry_baai_add(fname,type,size,user,uid,conf,access,date,desc,link);
    else
	qry_baai_edit(fname,type,size,user,uid,conf,access,date,desc,link);
}


void baai_save(SQLhandle sqlh, int *link, char **type, off_t *size,
	char **user, uid_t *uid, char **conf, int *access, time_t *date,
	char **desc)
{
    char *d;

    if (type != NULL)
    {
	d= sql_fetchstr(sqlh, 0);
	*type= strdup(d == NULL ? "" : d);
    }

    if (size != NULL)
    {
	d= sql_fetchstr(sqlh, 1);
	*size= atol(d);
    }

    if (user != NULL)
    {
	d= sql_fetchstr(sqlh, 2);
	*user= strdup(d == NULL ? "" : d);
    }

    if (uid != NULL)
    {
	d= sql_fetchstr(sqlh, 3);
	*uid= atoi(d);
    }

    if (conf != NULL)
    {
	d= sql_fetchstr(sqlh, 4);
	*conf= strdup(d == NULL ? "" : d);
    }

    if (access != NULL)
    {
	d= sql_fetchstr(sqlh, 5);
	*access= atoi(d);
    }

    if (date != NULL)
    {
	d= sql_fetchstr(sqlh, 6);
	*date= atol(d);
    }

    if (desc != NULL)
    {
	d= sql_fetchstr(sqlh, 7);
	*desc= strdup(d == NULL ? "" : d);
    }

    if (link != NULL)
    {
	d= sql_fetchstr(sqlh, 8);
	*link= atoi(d);
    }
}


/* BAAI_GET - Get values associated with a key.  Returns -1 if not found.
 * String value are in dynamic memory.  Any of the arguments can be NULL if
 * we don't want them.
 */

int baai_get(char *fname, int *link, char **type, off_t *size, char **user,
	uid_t *uid, char **conf, int *access, time_t *date, char **desc)
{
    SQLhandle sqlh;
    int nrow;

    make_sql_connection();

    sqlh= qry_baai_get(fname);

    if ((nrow= sql_nrows(sqlh)) == 0)
    {
	sql_got(sqlh);
	return -1;
    }
    if (nrow > 1)
	die("baai_get found more than one attachment with handle %s",fname);

    baai_save(sqlh,link,type,size,user,uid,conf,access,date,desc);

    return 0;
}


/* BAAI_DEL - Delete an attachment index entry.  Returns -1 if key was not
 * found.  Dies for other errors.
 */

int baai_del(char *fname)
{
    SQLhandle sqlh;
    int nrow;

    make_sql_connection();

    /* Can we get effected rows back from DELETE and dispense with this? */
    sqlh= qry_baai_get(fname);
    if ((nrow= sql_nrows(sqlh)) == 0)
	return -1;

    qry_baai_del(fname);
    return 0;
}


/* BAAI_LINK - Mark an attachment as being linked to an item.
 */

int baai_link(char *fname)
{
    qry_baai_link(fname);
}


/* BAAI_WALK - get the first or next index entry.  Returns 1 if something
 * was found, or 0 if we reached the end of the index.
 */

static SQLhandle bw_sqlh= NULL;

int baai_walk(int next, char **fname, int *link, char **type, off_t *size,
	char **user, uid_t *uid, char **conf, int *access, time_t *date,
	char **desc)
{
    if (!next)
    {
	make_sql_connection();
	if (bw_sqlh != NULL) sql_got(bw_sqlh);
	bw_sqlh= qry_baai_list(0);
    }
    else if (bw_sqlh == NULL)
	die("baai_walk asked for next entry before first entry");

    if (sql_nextrow(bw_sqlh))
    {
	baai_save(bw_sqlh,link,type,size,user,uid,conf,access,date,desc);
	if (fname != NULL)
	{
	    char *d= sql_fetchstr(bw_sqlh, 9);
	    *fname= strdup(d == NULL ? "" : d);
	}
	return 1;
    }

    sql_got(bw_sqlh);
    bw_sqlh= NULL;
    return 0;
}

#endif /*ATTACHMENTS */
