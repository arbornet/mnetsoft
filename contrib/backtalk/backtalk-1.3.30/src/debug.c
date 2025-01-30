#include "backtalk.h"
#include "vstr.h"
#include "str.h"
#include "format.h"
#include "debug.h"

char **old_env= NULL;
extern char **environ;


/* DUMP_ENV - print the environment to the given file descriptor.  If quoting
 * is true, we do HTML quoting (expanding "<" to "&lt;" and such).  If quoting
 * is false, we just turn newlines into "\n" and backslashes into "\\".
 */

void dump_env(FILE *fp, int quote)
{
    char **ep, *q;

    for (ep= environ; *ep != NULL; ep++)
	if (quote)
	{
	    /* Do HTML quoting */
	    fprintf(fp,"%s\n",q= sysformat(*ep));
	    free(q);
	}
	else
	{
	    /* Expand newlines and backslashes only */
	    for (q= *ep; *q != '\0'; q++)
	    	switch (*q)
	    	{
	    	case '\n':
	    	    fputs("\\n",fp);
	    	    break;
	    	case '\\':
	    	    fputs("\\\\",fp);
	    	    break;
		default:
		    fputc(*q,fp);
		}
	    fputc('\n',fp);
	}
    fputc('\n',fp);
}



/* MGETS - Like fgets(), but handles arbitrarily long lines.  Returned line
 * is in malloced memory and must be freed by the caller.  Returns NULL on
 * EOF.
 */

char *mgets(FILE *fp)
{
    VSTRING vs;
    int ch;

    vs_new(&vs, BFSZ);

    while ((ch= getc(fp)) != EOF)
    {
	*vs_inc(&vs)= ch;

	if (ch == '\n')
	    return vs_return(&vs);
    }

    if (vs.begin == vs.p)
	return NULL;
    else
	return vs_return(&vs);
}


/* LOAD_REPEAT - Load a previously written repeat file.  The environment in
 * the file is stored as our environment.  If the new environment indicates
 * that this was a POST query, then attach file to stdin, so the query can
 * be read.
 */

void load_repeat(char *fname)
{
#define MALSZ 40
    int esize= MALSZ, en=0;
    char **newenv= malloc(esize*sizeof(char *));
    char *bf;

    if (freopen(fname,"r",stdin) == NULL)
	die("Cannot open repeat file %s to read",fname);
    
    while ((bf= mgets(stdin)) != NULL)
    {
	if (en == esize)
	    newenv= realloc(newenv,(esize+= MALSZ)*sizeof(char *));

	if (bf[0] == '\n')
	{
	    /* Install new environment */
	    free(bf);
	    newenv[en]= NULL;
	    old_env= environ;
	    environ= newenv;
	    return;
	}

	/* Add a line to the environment */
	*firstin(bf,"\n")= '\0';
	unslash(bf);
	newenv[en]= (char *)malloc(strlen(bf)+1);
	strcpy(newenv[en], bf);
	free(bf);
	en++;
    }
    die("Premature EOF in %s",fname);
}


/* FREE_REPEAT - Restore the original environment, and free up all the memory
 * used in the fake environment that we created in load_repeat().  This
 * function is normally only used when testing for memory leaks.
 */

#ifdef CLEANUP
void free_repeat()
{
    int i;

    for (i= 0; environ[i] != NULL; i++)
	free(environ[i]);
    free(environ);
    environ= old_env;
}
#endif /* CLEANUP */
