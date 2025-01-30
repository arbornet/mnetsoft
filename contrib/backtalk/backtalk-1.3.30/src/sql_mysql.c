/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SQL Server Interface - MySQL version */

#include "common.h"

#if defined(USE_SQL) && defined(SQL_MYSQL)

#include "sql.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_sql_module= "sql_mysql (MySQL)";

/* Global connection handle for SQL server */
MYSQL *dbh= NULL;


/* SQL_CONNECTED - Return true or false depending on whether there is an
 * active SQL connection.
 */

int sql_connected()
{
    return (dbh != NULL);
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
    int iport;

    if (dbh != NULL) sql_disconnect();

    if ((dbh= mysql_init(NULL)) == NULL)
    	die("Insufficient memory for MYSQL handle");

    if (host != NULL && host[0] == '\0') host= NULL;
    iport= (port == NULL) ? 0 : atoi(port);
    if (iport < 0) iport= 0;

    if (!mysql_real_connect(dbh, host, login, passwd, dbname, iport, NULL, 0))
    	die("Could not connect to MySQL server %s:%d database %s as %s - %s",
	    host ? host : "localhost", iport,
	    dbname ? dbname : "<default>",
	    login ? login : "<default>",
	    mysql_error(dbh));
}


/* SQL_DISCONNECT - Drop the connection to the SQL server.
 */

void sql_disconnect()
{
    if (dbh == NULL) return;
    mysql_close(dbh);
    dbh= NULL;
}


/* SQL_DO - Execute an SQL command that returns no values.  Returns the number
 * of rows effected by the command.  Dies on error unless ignore_error is
 * true, in which case it returns -1.  You can use this to execute a command
 * that returns values, but the values get discarded.
 */

int sql_do(char *cmd, int ignore_error)
{
   int n;

   if (dbh == NULL)
   	die("sql_do(%s) called with no database connection open",cmd);

   if (mysql_query(dbh,cmd))
   {
   	if (ignore_error)
	    return -1;
	else
	    die("mysql_query(%s) failed - %s",cmd, mysql_error(dbh));
   }

   return mysql_affected_rows(dbh);
}


/* SQL_GET - Execute an SQL command that returns values (usually a 'SELECT').
 * Returns a statement handle on success, dies on failure.
 */

SQLhandle sql_get(char *cmd)
{
    SQLhandle s;

    if (dbh == NULL)
   	die("sql_get(%s) called with no database connection open",cmd);

    if (mysql_query(dbh,cmd))
   	die("mysql_query(%s) failed - %s",cmd, mysql_error(dbh));

    s= (SQLhandle)malloc(sizeof(struct sqh));
    s->h= mysql_store_result(dbh);
    if (s->h == NULL)
    {
    	if (mysql_errno(dbh))
	    die("could not fetch MySQL query result - %s",mysql_error(dbh));
	else
	    die("sql_get passed query '%s' which returns no results", cmd);
    }
    else
    {
	s->c= mysql_num_fields(s->h);
    }
    s->r= NULL;

   return s;
}


/* SQL_GOT - Release an SQL statement handle, freeing all memory associated
 * with it.  This should be called once for *every* call to sql_get().
 */

void sql_got(SQLhandle s)
{
   mysql_free_result(s->h);
   free(s);
}


/* SQL_NROWS - Return the number of rows returned by SQL_GET.
 */

int sql_nrows(SQLhandle s)
{
    return mysql_num_rows(s->h);
}


/* SQL_NEXTROW - Get the next result row.  This may optionally be called to get
 * the first result row after an sql_get().  Returns true if there is a next
 * row.
 */

int sql_nextrow(SQLhandle s)
{
    if (s->c == 0) return 0;
    s->r= mysql_fetch_row(s->h);
    if (s->r == NULL) return 0;
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
	s->r= mysql_fetch_row(s->h);
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
#ifdef HAVE_MYSQL_REAL_ESCAPE_STRING
    if (dbh != NULL)
	out+= mysql_real_escape_string(dbh, out, in, strlen(in));
    else
#endif
	out+= mysql_escape_string(out, in, strlen(in));
    *(out++)= '\'';
    *(out++)= '\0';
}

#endif /* USE_SQL && SQL_MYSQL */
