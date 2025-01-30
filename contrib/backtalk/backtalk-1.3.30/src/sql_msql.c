/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SQL Server Interface - mSQL 2.0 libmsql version */

#include "common.h"

#if defined(USE_SQL) && defined(SQL_MSQL)

#include "sql.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_sql_module= "sql_msql (Mini SQL 2.0)";

/* Global connection handle for SQL server */
int dbh= -1;


/* SQL_CONNECTED - Return true or false depending on whether there is an
 * active SQL connection.
 */

int sql_connected()
{
    return (dbh != -1);
}


/* SQL_CONNECT - Connect to the SQL server.  Dies on failure.
 *  host = host name or IP address.  NULL or empty for local host.
 *  port = port number for server (as string).  NULL for default.
 *  login = user login name.  NULL if none needed.
 *  passwd = user password.  NULL if none needed.
 *  dbname = database name.  NULL if no database.
 */

void sql_connect(char *host, char *port, char *login,
      char *passwd, char *dbname)
{
    if (dbh >= 0) sql_disconnect();

    if (host != NULL && host[0] == '\0') host= NULL;
    if ((dbh= msqlConnect(host)) == -1)
    	die("Could not connect to mSQL server %s - %s",
	    host ? host : "localhost", msqlErrMsg);

    if (dbname != NULL && dbname[0] != '\0')
    {
	if (msqlSelectDB(dbh, dbname) == -1)
	    die("Could not open mSQL database %s - %s", dbname, msqlErrMsg);
    }
}


/* SQL_DISCONNECT - Drop the connection to the SQL server.
 */

void sql_disconnect()
{
    if (dbh < 0) return;
    msqlClose(dbh);
    dbh= -1;
}


/* SQL_DO - Execute an SQL command that returns no values.  Returns the number
 * of rows effected by the command.  Dies on error unless ignore_error is
 * true, in which case it returns -1.  You can use this to execute a command
 * that returns values, but the values get discarded.
 */

int sql_do(char *cmd, int ignore_error)
{
    int n;

    if (dbh < 0)
   	die("sql_do(%s) called with no database connection open",cmd);

    n= msqlQuery(dbh,cmd);
    if (n < 0)
    {
	if (ignore_error)
	    return -1;
       	else
	    die("msqlQuery(%s) failed - %s",cmd,msqlErrMsg);
    }

    return n;
}


/* SQL_GET - Execute an SQL command that returns values (usually a 'SELECT').
 * Returns a statement handle on success, dies on failure.
 */

SQLhandle sql_get(char *cmd)
{
   SQLhandle s;

   if (dbh < 0)
   	die("sql_get(%s) called with no database connection open",cmd);


   n= msqlQuery(dbh,cmd);
   if (n < 0)
       die("msqlQuery(%s) failed - %s",cmd,msqlErrMsg);
   
   s= (SQLhandle)malloc(sizeof(struct sqh));
   s->h= msqlStoreResult();
   s->r= NULL;
   s->c= -1;

   return sth;
}


/* SQL_GOT - Release an SQL statement handle, freeing all memory associated
 * with it.  This should be called once for *every* call to sql_get().
 */

void sql_got(SQLhandle s)
{
   msqlFreeResult(s->h);
   free(s);
}


/* SQL_NROWS - Return the number of rows returned by SQL_GET.
 */

int sql_nrows(SQLhandle s)
{
    return msqlNumRows(s->h);
}


/* SQL_NEXTROW - Get the next result row.  This may optionally be called to get
 * the first result row after an sql_get().  Returns true if there is a next
 * row.
 */

int sql_nextrow(SQLhandle s)
{
    s->r= msqlFetchRow(s->h);
    if (s->r == NULL) return 0;
    if (s->c == -1) s->c= msqlNumFields(s->h)
    return 1;
}

/* SQL_FETCHSTR - Return a pointer to a string representation of the value
 * in column col (zero based index) of the current row.  Returns NULL if
 * the value is null, or if the column number is out of range or if there
 * is no row.  The returned value should not be editted.  Copy them out if
 * you want to change them.
 */

char *sql_fetchstr(SQLhandle s, int col)
{
    /* If we haven't advanced to the first row yet, then do so */
    if (s->r == NULL)
    {
	s->r= msqlFetchRow(s->h);
	if (s->r == NULL) return NULL;
    }

    /* if column is out of range */
    if (col < 0 || col >= s->c) return NULL;

    return s->r[col];
}


/* SQL_QUOTE - Given a string input and a buffer whose size is at least
 * 2*strlen(input)+3, store a SQL quoted copy of the string, with enclosing
 * quote marks, into the buffer.  If the input is NULL, store "NULL".
 */

void sql_quote(char *in, char *out)
{
    if (in == NULL)
    {
	strcpy(out,"NULL");
	out+= 4;
	return;
    }
    *(out++)= '\'';
    while (*in != '\0')
    {
	*(out++)= *in;
    	if (*in == '\'')
	    *(out++)= '\'';
        in++;
    }
    *(out++)= '\'';
    *(out++)= '\0';
}


#endif /* USE_SQL && SQL_MSQL */
