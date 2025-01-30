/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"
#include "sysconfig.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* Default settings of configuration variables */

char *cf_email_fromaddr= NULL;
char *cf_email_fromname= NULL;
char *cf_email_organization= NULL;
char *cf_sql_hostname= NULL;
char *cf_sql_port= NULL;
char *cf_sql_login= NULL;
char *cf_sql_password= NULL;
char *cf_sql_dbname= "backtalk";

/* Flag tells if the config file has been loaded */
int cf_loaded= 0;

/* Table of config variables */

struct cfvar {
    char *name;		/* Name of option */
    char **var;		/* Pointer to variable */
    } config_var[] = {
    {"email_fromaddr",		&cf_email_fromaddr},
    {"email_fromname",		&cf_email_fromname},
    {"sql_hostname",		&cf_sql_hostname},
    {"sql_port",		&cf_sql_port},
    {"sql_login",		&cf_sql_login},
    {"sql_password",		&cf_sql_password},
    {"sql_dbname",		&cf_sql_dbname},

    {NULL,		NULL}};

/* READ_CONFIG_FILE - Read the config file, saving settings in the global
 * variables defined above if we haven't previously called this.  Dies on
 * failure.  A non-existant config file is not an error.  
 */

void read_config_file()
{
    FILE *fp;
    char bf[BFSZ+1];
    char *cmd, *arg, *p;
    int i, line= 0;

    if (cf_loaded) return;
    cf_loaded= 1;

    if ((fp= fopen(CONFIG_FILE,"r")) == NULL)
	return;

    while (fgets(bf, BFSZ, fp) != NULL)
    {
	/* Make sure we got a whole, newline terminated line */
	p= firstin(bf,"\n\r");
	if (*p == '\0') die(CONFIG_FILE" line %d too long\n",line);
	*p= '\0';
    	line++;

	/* skip comments */
    	if (bf[0] == '#') continue;

	/* parse out command and argument string */
	cmd= firstout(bf," \t\n\r");
	if (*cmd == '\0') continue;
	p= firstin(bf," \t\n\r");
	*p= '\0';
	arg= firstout(p+1," \t\n\r");
	*firstin(arg+1,"\n\r")= '\0';

	/* look up command */
	for (i= 0; config_var[i].name != NULL; i++)
	    if (!strcmp(cmd,config_var[i].name))
	    	break;

	if (config_var[i].name == NULL)
	    die("Unknown config option %s on line %d of "CONFIG_FILE,cmd,line);
	    
	/* Save value - note that if we set the same option more than once
	 * the memory used in the first option will never be freed.  But
	 * so what?
	 */

	if (*arg == '\0')
	     *config_var[i].var= NULL;
	else
	    *config_var[i].var= strdup(arg);
    }
    fclose(fp);
}
