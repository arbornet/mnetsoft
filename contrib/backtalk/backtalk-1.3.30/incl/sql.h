/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* This file has defines a generic interface to various different SQL
 * database systems, allowing us to use the same set of calls to submit
 * queries and return results for all servers.
 */

#ifndef _BT_SQL_H
#define _BT_SQL_H

/* Query Handle Definitions - When doing a SELECT, you get back an SQLhandle
 * which is used to access the results.  It's defined different ways for
 * different servers.
 */

/* Server description */

char *showopt_sql_module;

#ifdef USE_SQL

#ifdef SQL_PGSQL
#include <pgsql/libpq-fe.h>

typedef struct sqh {
    PGresult *h;	/* Postgres statement handle */
    int r;		/* Current row number */
    int n;		/* Total number of rows */
    int c;		/* Total number of columns */
    } *SQLhandle;

#endif /* SQL_PGSQL */

#ifdef SQL_MSQL
#include "msql.h"

typedef struct sqh {
    m_result *h;	/* mSQL stored result */
    m_row    *r;	/* current row */
    int       c;	/* Total number of columns */
    } *SQLhandle;

#endif /* SQL_MSQL */

#ifdef SQL_MYSQL
#ifdef HAVE_MYSQL_H
#include <mysql.h>
#endif
#ifdef HAVE_MYSQL_MYSQL_H
#include <mysql/mysql.h>
#endif

typedef struct sqh {
    MYSQL_RES   *h;	/* MySQL stored result */
    MYSQL_ROW	r;	/* current row */
    int		c;	/* Total number of columns */
    } *SQLhandle;

#endif /* SQL_MYSQL */


/* Function definitions - These are the same for all SQL servers */

int sql_connected(void);
void sql_connect(char *host, char *port, char *login, char *passwd,
	char *dbname);
void sql_disconnect(void);
int sql_do(char *cmd, int ignore_error);
SQLhandle sql_get(char *cmd);
int sql_nrows(SQLhandle s);
int sql_nextrow(SQLhandle s);
char *sql_fetchstr(SQLhandle s, int col);
void sql_got(SQLhandle s);
void sql_quote(char *in, char *out);

#endif /* USE_SQL */

#endif /* _BT_SQL_H */
