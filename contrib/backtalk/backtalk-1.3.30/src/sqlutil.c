/* (c) 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#ifdef USE_SQL

#include "sysconfig.h"
#include "sql.h"
#include "sqlutil.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* MAKE_SQL_CONNECTION - Open an SQL connection to the Backtalk database,
 * if we don't already have one.  Calls die() on error.
 */

void make_sql_connection()
{
    if (sql_connected()) return;

    read_config_file();

    sql_connect(
    	((cf_sql_hostname != NULL) && strcmp(cf_sql_hostname,"localhost")) ?
	    cf_sql_hostname : NULL,
	cf_sql_port,
	cf_sql_login,
	cf_sql_password,
	cf_sql_dbname);
}

#endif /* USE_SQL */
