/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SQL Server Interface - PostgreSQL libpq version */

#include "common.h"
#include "str.h"

#if defined(USE_SQL) && defined(SQL_PGSQL)

#include "sql.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_sql_module= "sql_pgsql (PostgreSQL)";

/* SIZE_SPRINTF3 is like sprintf but always takes exactly three arguments and
 * is guaranteed to return the size even if the standard sprintf on the system
 * does not.
 */

#ifdef FPRINTF_RETURNS_SIZE
#define SIZE_SPRINTF3 sprintf
#else
#define SIZE_SPRINTF3(b,f,a) (sprintf(b,f,a),strlen(b))
#endif


/* Global connection handle for SQL server */
PGconn *dbh= NULL;


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
    int lho,lpo,llo,lpa,ldb;
    char *conninfo, *ci;

    if (dbh != NULL) sql_disconnect();

    /* Figure out how big a buffer we'll need */
    lho= host ? strlen(host)+11 : 0;
    lpo= port ? strlen(port)+13 : 0;
    llo= login ? strlen(login)+9 : 0;
    lpa= passwd ? strlen(passwd)+13 : 0;
    ldb= dbname ? strlen(dbname)+12 : 0;

    ci= conninfo= malloc(20+lho+lpo+llo+lpa+ldb);

    /* Write the argument string into the buffer */
    if (lho > 0)
    {
	if (*firstout(host,"0123456789.") == '\0')
	    ci+= SIZE_SPRINTF3(ci,"hostaddr=%s ",host);
	else
	    ci+= SIZE_SPRINTF3(ci,"host='%s' ",host);
    }
    if (lpo > 0) ci+= SIZE_SPRINTF3(ci,"port='%s' ",port);
    if (llo > 0) ci+= SIZE_SPRINTF3(ci,"user='%s' ",login);
    if (lpa > 0) ci+= SIZE_SPRINTF3(ci,"password='%s' ",passwd);
    if (ldb > 0) ci+= SIZE_SPRINTF3(ci,"dbname='%s' ",dbname);
    if (ci > conninfo && ci[-1] == ' ') *(--ci)= '\0';

    dbh= PQconnectdb(conninfo);
    if (PQstatus(dbh) == CONNECTION_BAD)
    	die("Cannot connect to PostgreSQL database server (%s): %s",
	    conninfo, PQerrorMessage(dbh));

    free(conninfo);
}


/* SQL_DISCONNECT - Drop the connection to the SQL server.
 */

void sql_disconnect()
{
    if (dbh == NULL) return;
    PQfinish(dbh);
    dbh= NULL;
}


/* SQL_DO - Execute an SQL command that returns no values.  Returns the number
 * of rows effected by the command.  Dies on error unless ignore_error is
 * true, in which case it returns -1.  You can use this to execute a command
 * that returns values, but the values get discarded.
 */

int sql_do(char *cmd, int ignore_error)
{
   PGresult *sth;
   int rc, n;

   if (dbh == NULL)
   	die("sql_do(%s) called with no database connection open",cmd);

   sth= PQexec(dbh,cmd);
   if (sth == NULL || ( (rc= PQresultStatus(sth)) != PGRES_COMMAND_OK &&
                        rc != PGRES_TUPLES_OK))
   {
       if (ignore_error)
	   return -1;
       else
	   die("PQexec(%s) failed - %s",cmd,PQerrorMessage(dbh));
   }

   n= atoi(PQcmdTuples(sth));
   PQclear(sth);

   return n;
}


/* SQL_GET - Execute an SQL command that returns values (usually a 'SELECT').
 * Returns a statement handle on success, dies on failure.
 */

SQLhandle sql_get(char *cmd)
{
   PGresult *sth;
   int rc;
   SQLhandle s;

   if (dbh == NULL)
   	die("sql_get(%s) called with no database connection open",cmd);

   s= (SQLhandle)malloc(sizeof(struct sqh));

   s->r= -1;
   s->h= PQexec(dbh,cmd);
   if (s->h == NULL || PQresultStatus(s->h) != PGRES_TUPLES_OK)
      die("PQexec(%s) failed - %s",cmd,PQerrorMessage(dbh));
   s->n= PQntuples(s->h);
   s->c= PQnfields(s->h);

   return s;
}


/* SQL_GOT - Release an SQL statement handle, freeing all memory associated
 * with it.  This should be called once for *every* call to sql_get().
 */

void sql_got(SQLhandle s)
{
    if (s == NULL) return;
    PQclear(s->h);
    free(s);
}


/* SQL_NROWS - Return the number of rows returned by SQL_GET.
 */

int sql_nrows(SQLhandle s)
{
    return s->n;
}


/* SQL_NEXTROW - Get the next result row.  This may optionally be called to get
 * the first result row after an sql_get().  Returns true if there is a next
 * row.
 */

int sql_nextrow(SQLhandle s)
{
    if (s->r + 1 >= s->n) return 0;
    s->r++;
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
    if (s->r < 0)
    {
    	if (s->n == 0)
	    return NULL;
	else
	    s->r= 0;
    }

    /* if column is out of range */
    if (col < 0 || col >= s->c) return NULL;

    if (PQgetisnull(s->h, s->r, col)) return NULL;

    return PQgetvalue(s->h, s->r, col);
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
        if (*in == '\'')
	{
            *(out++)= '\\';
            *(out++)= '\'';
	}
	else if (*in == '\\')
	{
            *(out++)= '\\';
            *(out++)= '\\';
	}
	else
	    *(out++)= *in;
        in++;
    }
    *(out++)= '\'';
    *(out++)= '\0';
}


#endif /* USE_SQL && SQL_PGSQL */
