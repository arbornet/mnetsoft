/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Program to create SQL databases.  Argument specifies which database:
 *   all  (defaut)
 *   auth
 *   ident
 *   nextuid or uid
 *   group or grp
 *   session or sess
 */

#include "common.h"
#include "adm.h"
#include "sqlutil.h"

/* Load in only qry_*_create SQL query routines */
#define SQL_CREATE
#define AUTHIDENT_H
#include "sqlqry.c"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


int main(int argc, char **argv)
{
    char *type= NULL;
    int didit= 0;

#ifdef USE_SQL
    if (argc > 2)
       die("usage: %s [<dbname>]\n", argv[0]);
    if (argc > 1 && strcmp(argv[1],"all"))
    	type= argv[1];
    
    make_sql_connection();

#ifdef CREATE_AUTH
    if (type == NULL || !strcmp(type,"auth"))
    {
	qry_auth_create();
	printf("creating auth database\n");
	didit= 1;
    }
#endif

#ifdef CREATE_IDENT
    if (type == NULL || !strcmp(type,"ident"))
    {
	qry_ident_create();
	printf("creating ident database\n");
	didit= 1;
    }
#endif

#ifdef CREATE_UID
    if (type == NULL || !strcmp(type,"uid") || !strcmp(type,"nextuid"))
    {
	qry_uid_create();
	printf("creating nextuid database\n");
	didit= 1;
    }
#endif

#ifdef CREATE_GROUP
    if (type == NULL || !strcmp(type,"grp") || !strcmp(type,"group"))
    {
	qry_group_create();
	printf("creating group database\n");
	didit= 1;
    }
#endif

#ifdef CREATE_SESS
    if (type == NULL || !strcmp(type,"sess") || !strcmp(type,"session"))
    {
	qry_sess_create();
	printf("creating session database\n");
	didit= 1;
    }
#endif

#ifdef CREATE_BAAI
    if (type == NULL || !strcmp(type,"baai"))
    {
	qry_baai_create();
	printf("creating attachment index database\n");
	didit= 1;
    }
#endif

#endif /* USE_SQL */

    if (!didit)
    {
	if (type)
	    printf("No SQL %s database defined in this installation.\n", type);
	else
	    printf("No SQL databases defined in this installation.\n");
    }


    exit(0);
}
