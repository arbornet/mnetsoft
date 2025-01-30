/* Copyright 2003, Jan D. Wolter, All Rights Reserved. */

#include "backtalk.h"
#include "sysdict.h"
#include "ps_ulist.h"
#include "str.h"
#include "group.h"
#include <ctype.h>

#define ACL "/acl"

/* Typically the same keywords, like +ulist, appear in more than one line of
 * the acl file.  We only want to test them once, so after the first test,
 * we store the result in this linked list.  It's all in dynamic memory,
 * except for the first entry.
 */

static struct aclc {
    char *key;
    int value;
    struct aclc *next;
} acl_cache = {"all", 1, NULL};

static int r_list;     /* ulist, glist or passwd restricts read access? */
static int w_list;     /* ulist, glist or passwd restricts respond access? */
static int acl_secret; /* secret password exists? */
static int acl_ulist;  /* ulist exists? */
static int acl_glist;  /* glist exists? */


/* ACL_WORD:  Given a keyword from a acl line, decide if it is true or
 * false for the current user.
 */

int acl_word(char *word, int linetype, char *fn, int line)
{
    int negate= 0;
    int value;
    struct aclc *ac;

    /* Parse out + or - prefix */
    if (word[0] == '-') { negate= 1; word++; }
    else if (word[0] == '+') word++;

    /* Check the cache */
    for (ac= &acl_cache; ac != NULL; ac= ac->next)
	if (!strcmp(ac->key,word))
	    return (ac->value ^ negate);

    /* Some words that are so easy they aren't worth caching */

    if (!strcmp(word,"registered"))
	return ((sdict(VAR_ID) != NULL && sdict(VAR_ID)[0] != '\0') ^ negate);

    if (!strcmp(word,"fwlist"))
	return (bdict(VAR_AMFW) ^ negate);

    if (!strcmp(word,"sysop"))
	return (bdict(VAR_AMADM) ^ negate);

    if (!strcmp(word,"originlist"))	/* Not supported - yet */
	return !negate;

    /* The rest we cache */

    if (word[0] == 'f' && word[1] == ':')
    {
	if (!strcmp(word+2,"ulist"))
	{
	    acl_ulist= 1;
	    if (linetype == 'r') r_list= 1;
	    else if (linetype == 'w') w_list= 1;
	}
	value= in_ulist(sdict(VAR_ID),word+2);
    }

    else if (word[0] == 'u' && word[1] == '=')
    {
	char *p, *e;

	value= 0;
	for (p= word+2; *p != '\0'; p= e+1)
	{
	    e= firstin(p,",");
	    if (sdict(VAR_ID)[e-p] == '\0' && !strncmp(sdict(VAR_ID),p,e-p))
	    {
		value= 1;
		break;
	    }
	}
    }

    else if (word[0] == 'g' && word[1] == ':')
    {
	if (!strcmp(word+2,"glist"))
	{
	    acl_glist= 1;
	    if (linetype == 'r') r_list= 1;
	    else if (linetype == 'w') w_list= 1;
	}
	value= in_glist(sdict(VAR_ID),idict(VAR_GID), word+2);
    }

    else if (word[0] == 'g' && word[1] == '=')
    {
	/* Load list of group names into array */
	char *glb= strdup(word+2);
	int n= strcnt(word+2,',')+1;
	char **gl= (char **)malloc((n+1)*sizeof(char *));
	int i;
	char *p, *e;

	for (i= 0, p= glb; i < n; i++, p= e+1)
	{
	    gl[i]= p;
	    if ((e= strchr(p,',')) != NULL) *e= '\0';
	}
	gl[n]= NULL;

	/* Check if we are in any of those groups */
	value= ingrouplist(gl, idict(VAR_GID), sdict(VAR_ID));

	free(glb);
	free(gl);
    }

    else if (!strcmp(word,"password"))
    {
	acl_secret= 1;
	if (linetype == 'r') r_list= 1;
	else if (linetype == 'w') w_list= 1;
	value= cpass_ok(sdict(VAR_CPASS));
    }

    else
	die("Unknown acl keyword %s in %s line %d",word,fn,line);

    /* Do the caching */
    ac= (struct aclc *)malloc(sizeof(struct aclc));
    ac->key= strdup(word);
    ac->value= value;
    ac->next= acl_cache.next;
    acl_cache.next= ac;

    return (value ^ negate);
}


/* FREE_ACL_CACHE:  Deallocate that acl_cache
 */

void free_acl_cache()
{
    struct aclc *ac, *nac;

    for (ac= acl_cache.next; ac != NULL; ac= nac)
    {
	nac= ac->next;
	free(ac->key);
	free(ac);
    }
    acl_cache.next= NULL;
}


/* READ_ACL:  Check if an acl file exists for the conference.  If not,
 * return false.  Otherwise, return true, setting mayread, maypost, and
 * mayresp to true or false, depending on whether the current user has
 * those capabilities in the conference.
 */

int read_acl(char *dir, int *mayread, int *mayresp, int *maypost)
{
    char fn[BFSZ];
    char word[BFSZ];
    int ch, line= 1;
    int linetype= 0;
    int inword= 0, wordlen= 0;
    int new;
    int anon= (sdict(VAR_ID) == NULL || sdict(VAR_ID)[0] == '\0');
    FILE *fp;

    /* Open the acl file */
    strcpy(fn,dir);
    strcat(fn,ACL);
    if ((fp= fopen(fn,"r")) == NULL)
	return 0;

    /* Till we know better, assume he can do anything */
    if (mayread != NULL) *mayread= 1;
    if (mayresp != NULL) *mayresp= !anon;
    if (maypost != NULL) *maypost= !anon;
    acl_ulist= acl_glist= acl_secret= r_list= w_list= 0;

    /* Parse the file */
    while ((ch= getc(fp))!= EOF)
    {
	if (linetype == 0)
	{
	    /* Haven't read first character of line yet */
	    if (isspace(ch)) continue;
	    if (strchr("rwca#",ch))
		linetype= ch;
	    else if (strchr("RWCA",ch))
		linetype= tolower(ch);
	    else
		die("Syntax error in line %d of %s\n",line,fn);
	}
	else if (linetype == '#' || linetype == 'a')
	{
	    /* Ignore comments and 'a' lines (at least for now) */
	}
	else if (inword)
	{
	    if (isspace(ch))
	    {
		/* Finished reading in a keyword */
		word[wordlen]= '\0';
		new= acl_word(word, linetype, fn, line);
		if (!new)
		{
		    /* If we've falsified the user's access, set flag to zero
		     * and ignore rest of line
		     */
		    if (linetype == 'r' && mayread != NULL) *mayread= 0;
		    if (linetype == 'w' && mayresp != NULL) *mayresp= 0;
		    if (linetype == 'c' && maypost != NULL) *maypost= 0;
		    linetype= '#';
		}
		inword= wordlen= 0;
	    }
	    else if (wordlen >= BFSZ-1)
		die("Keyword too long in line %d of %s\n",line,fn);
	    else
		word[wordlen++]= ch;
	}
	else if (!isspace(ch))
	{
	    inword= 1;
	    word[0]= ch;
	    wordlen= 1;
	}

	if (ch == '\n') linetype= 0;
    }

    /* Reasonable approximations of settings for various old-style flags */
    seti(VAR_USERLIST, acl_ulist);
    seti(VAR_GROUPLIST, acl_glist);
    seti(VAR_SECRET, acl_secret);
    seti(VAR_FISHBOWL, !r_list && w_list);

    free_acl_cache();
    return 1;
}
