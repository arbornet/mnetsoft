/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Routines to read the conflist
 *
 * Mar 15, 1993 - Jan Wolter:  Original bbsread version
 * Dec  2, 1995 - Jan Wolter:  Ansification
 * Dec  4, 1995 - Jan Wolter:  Expansion of % in confname to confdir
 * Jan  3, 1996 - Jan Wolter:  Backtalk version
 * Jul 23, 2001 - Jan Wolter:  Caching version
 */

#include "backtalk.h"

#include "sysdict.h"
#include "dict.h"
#include "str.h"
#include "ps_conflist.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *expand_path(char *path, int dynamic);
void subchar(char *buf, char *string);

/* Linked list structure for cached conflist file.  There is an array of
 * 26 lists, one for each possible first letter of the conference name.
 * each list is a linked list in the same order as the conflist file.
 */

#define NCLCACHE 26
struct clist {
    char *pat;		/* conference name pattern with possible _'s */
    char *dir;		/* conference directory, possibly unexpanded */
    struct clist *next;	/* pointer to next entry in list */
    } **clcache= NULL;
char *dflt_path= NULL;


/* OPEN_CONFLIST - Open the conflist, check it's headers, and skip the default
 * conference, and return the file pointer.
 */

FILE *open_conflist()
{
    FILE *fp;
    char buf[BFSZ];

    if ((fp= fopen(csdict(VAR_CONFLIST),"r")) == NULL)
	die("could not open %s",csdict(VAR_CONFLIST));

    /* Read and check magic code number */
    if (fgets(buf,BFSZ,fp) == NULL || strcmp(buf,"!<hl01>\n"))
	die("conference list %s has bad magic number",
	    csdict(VAR_CONFLIST));

    /* Read and save default conference path - skipping comments */
    for (;;)
    {
	if (fgets(buf,BFSZ,fp) == NULL)
	    die("conference list %s too short", csdict(VAR_CONFLIST));
	if (buf[0] != '#')
	{
	    if (dflt_path == NULL)
	    {
		*firstin(buf,"\n")= '\0';
	    	dflt_path= strdup(buf);
	    }
	    break;
	}
    }

    return fp;
}


/* CACHE_CONFLIST -- Load the conflist file into memory.  If this is done,
 * future cfpath calls will use that instead of reading the file, improving
 * performance when we are going to do a large number of lookups.  If 'load'
 * is true, we load it, otherwise we unload it.
 */

void cache_conflist(int load)
{
    int i;

    if (load)
    {
	/* Load the conflist into cache */

	struct clist *curr[NCLCACHE], *new;
	char buf[BFSZ], *dir;
	FILE *fp= open_conflist();

	/* Initialize the cache */
	clcache= (struct clist **) calloc(NCLCACHE,sizeof(struct clist *));
	for (i= 0; i < NCLCACHE; i++)
	    curr[i]= NULL;

	/* Read list of conferences */
	while (fgets(buf,BFSZ,fp) != NULL)
	{
	    if (buf[0] == '#' || buf[0] == '\n') continue;

	    /* Parse out pattern and dir */
	    if ((dir= strchr(buf,':')) == NULL)
		die("bad line in conference list %s", csdict(VAR_CONFLIST));
	    *dir= '\0';
	    dir++;
	    *firstin(dir,"\n \t")= '\0';

	    /* Make the structure */
	    new= (struct clist *)malloc(sizeof(struct clist));
	    new->pat= strdup(buf);
	    new->dir= strdup(dir);
	    new->next= NULL;

	    /* Append it to the list */
	    i= buf[0] % NCLCACHE;		/* Which list this goes in */
	    if (curr[i] == NULL)
		clcache[i]= new;
	    else
		curr[i]->next= new;
	    curr[i]= new;
	}
	fclose(fp);
    }
    else
    {
	/* Deallocate the cached conflist */

	struct clist *p, *q;

	if (clcache == NULL) return;

    	for (i= 0; i < NCLCACHE; i++)
	    for (p= clcache[i]; p != NULL; p= q)
	    {
	    	q= p->next;
		free(p->pat);
		free(p->dir);
		free(p);
	    }
	free(clcache);
	clcache= NULL;
    }
}


/* CFPATH -- Given a conference name, this returns the directory name.  It
 * returns null if the conference does not exist.  If the conference name is
 * '?' then it returns the default conference path.  It exits with an error
 * message if some other weird error occurs.  The returned value is in static
 * storage.
 */

char *cfpath(char *name)
{
    if (clcache == NULL)
    {
	static char buf[BFSZ];
	char *a;

	/* Open file and skip headers */
	FILE *fp= open_conflist();

	if (name[0] == '?' && name[1] == '\0')
	    return (dflt_path ? expand_path(dflt_path,TRUE) : NULL);

	/* Search list of conferences */
	while (fgets(buf,BFSZ,fp) != NULL)
	{
	    if (buf[0] == '#' || buf[0] == '\n') continue;

	    if (match(name,buf,1))
	    {
		if ((a= strchr(buf,':')) == NULL)
		    die("bad line in conference list %s", csdict(VAR_CONFLIST));
		a++;
		*firstin(a,"\n \t")= '\0';
		fclose(fp);
		return expand_path(a,FALSE);
	    }
	}
	fclose(fp);
    }
    else
    {
	struct clist *p;
    	int i;

	if (name[0] == '?' && name[1] == '\0')
	    return (dflt_path ? expand_path(dflt_path,TRUE) : NULL);

    	i= name[0] % NCLCACHE;	/* Which list this goes in */
	for (p= clcache[i]; p != NULL; p= p->next)
	{
	    if (match(name,p->pat,1))
	    {
	    	p->dir= expand_path(p->dir,TRUE);
	    	return p->dir;
	    }
	}
    }
    return NULL;
}


/* SUBCHAR - Replace the first character of "buf" with the given string.
 * No checking for buffer overflow is done.
 */

void subchar(char *buf, char *string)
{
    char *p= strchr(buf,'\0');
    int len= strlen(string)-1;

    for (p= strchr(buf,'\0'); p > buf; p--)
	p[len]= *p;
    
    for (p= buf; *string != '\0'; p++, string++)
	*p= *string;
}


/* EXPAND_PATH - Expand out % sign in path.  If dynamic is true, path points
 * to a block of malloced memory and this reallocs it larger.  If not, the
 * buffer must be large enough to allow for the expansion.
 */

char *expand_path(char *path, int dynamic)
{
    char *p;

    if (path[0] == '%')
    {
	char *bbsdir= csdict(VAR_BBSDIR);
	if (dynamic)
	    path= (char *)realloc(path, strlen(path)+strlen(bbsdir)+2);
	subchar(path,bbsdir);
    }
    return path;
}


#ifdef CLEANUP
/* FREE_CLIST - deallocate conference list stuff.  Used for memory
 * debugging only.
 */

void free_clist()
{
    cache_conflist(FALSE);
    if (dflt_path != NULL) free(dflt_path);
}
#endif
