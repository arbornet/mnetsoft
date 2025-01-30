/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* ROUTINES TO READ/EDIT THE TEXT VERSION OF THE BACKTALK GROUP DATABASE
 *
 *  Our group database file format is similar to the Unix one:
 *     group_name::GID:user_list
 *  Where user_list is a comma separated list of users who are in that group,
 *  (not necessarily including those who have it as their primary group id).
 *  Long lines may be broken up with \ at end of each broken line.
 */

#include "backtalk.h"
#include "group.h"

#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *showopt_group_module= "grp_text (file="GROUP_FILE")";

/* ========================= READ ONLY FUNCTIONS ======================= */

/* GROUPNAME - Given a gid number, return the matching group name.  Return
 * NULL if not defined.  Returned name should be treated as being in static
 * memory - future calls may overwrite it.
 *
 * File version.
 */

char *groupname(gid_t gid)
{
    FILE *fp;
    static char bf[BFSZ+1];
    char *p, *q;
    int continued= 0;
    int len;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
    {
	/* No file - use defaults */
#if defined(CFADM_GID) && defined(CFADM_GROUP)
	if (gid == CFADM_GID) return CFADM_GROUP;
#endif
#if defined(USER_GID) && defined(USER_GROUP)
	if (gid == USER_GID) return USER_GROUP;
#endif
#if defined(GRADM_GID) && defined(GRADM_GROUP)
	if (gid == GRADM_GID) return GRADM_GROUP;
#endif
    	return NULL;
    }

    while (fgets(bf, BFSZ, fp) != NULL)
    {
	len= strlen(bf);
    	if (!continued && bf[0] != '#' && (p= strchr(bf,':')) != NULL &&
	    (q= strchr(p+1,':')) != NULL && isascii(q[1]) && isdigit(q[1]) &&
	    atoi(q+1) == gid)
	{
	    *p= '\0';
	    fclose(fp);
	    return bf;
	}
	continued= ((bf[len-1] != '\n') || (bf[len-2] == '\\'));
    }
    fclose(fp);
    return NULL;
}


/* GROUPID - Given a group name, return the matching group id.  Return
 * -1 if not defined.
 *
 * File version.
 */

int groupid(char *gname)
{
    FILE *fp;
    static char bf[BFSZ+1];
    char *p, *q;
    int continued= 0;
    int len;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
    {
	/* No file - use defaults */
#if defined(CFADM_GID) && defined(CFADM_GROUP)
	if (!strcmp(gname,CFADM_GROUP)) return CFADM_GID;
#endif
#if defined(USER_GID) && defined(USER_GROUP)
	if (!strcmp(gname,USER_GROUP)) return USER_GID;
#endif
#if defined(GRADM_GID) && defined(GRADM_GROUP)
	if (!strcmp(gname,GRADM_GROUP)) return GRADM_GID;
#endif
    	return -1;
    }

    while (fgets(bf, BFSZ, fp) != NULL)
    {
	len= strlen(bf);
    	if (!continued && bf[0] != '#' && (p= strchr(bf,':')) != NULL)
	{
	    *p= '\0';
	    if (!strcmp(bf,gname))
	    {
		fclose(fp);
		if ((q= strchr(p+1,':')) == NULL) return -1;
		return atoi(q+1);
	    }
	}
	continued= ((bf[len-1] != '\n') || (bf[len-2] == '\\'));
    }
    fclose(fp);
    return -1;
}


/* INGROUPNO - Given a group number return true if the given user with the
 * given group id is in that group.
 *
 * File version - do character-at-a-time reading to handle really long lines
 * without buffer overflow.
 */

int ingroupno(gid_t gid, gid_t my_gid, char *my_login)
{
    FILE *fp;
    char bf[BFSZ+1];
    int state, ch, i, esc;

    if (my_login == NULL || my_login[0] == '\0') return 0;
    if (gid < 0) return 0;
    if (gid == my_gid) return 1;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
	return 0;

    state= 0;
    esc= 0;
    while ((ch= fgetc(fp)) != EOF)
    {
    	switch (state)
	{
	case -1: /* skip to start of line */
	    if (ch == '\n' && !esc) state= 0;
	    break;

	case 0: /* ready to read group name */
	    if (ch == '#') {state= -1; break;}
	    state= 1;
	    i= 0;	/* count of colons */
	    /* drop through */

	case 1: /* reading group name */
	    if (ch == ':')
	    {
		/* on second colon - start reading gid */
		if (++i == 2) state= 2;
	    }
	    else if (ch == '\n' && !esc)
		state= 0;
	    break;

	case 2: /* ready to read gid */
	    state= 3;
	    i= 0;
	    /* drop through */

	case 3: /* reading gid number */
	    if (ch == ':' || (ch == '\n' && !esc))
	    {
		bf[i]= '\0';
		if (i <= 0 || !isascii(bf[0]) || !isdigit(bf[0]) ||
		         atoi(bf) != gid)
		    state= -1;	/* gid doesn't match - skip to next line */
		else if (ch != '\n')
		    state= 4;	/* gid matches - check names */
		else
		{
		    /* found line, but gid doesn't match and no names */
		    fclose(fp);
		    return 0;
		}
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ) state= -1;
	    }
	    break;

	case 4: /* ready to read a name */
	    state= 5;
	    i= 0;

	case 5: /* reading a name */
	    if (ch == ',' || (ch == '\n' && !esc))
	    {
	    	bf[i]= '\0';
		if (!strcmp(my_login, bf))
		{
		    fclose(fp);
		    return 2;
		}

		if (ch == '\n')
		{
		    /* no more names */
		    fclose(fp);
		    return 0;
		}
		else
		    state= 4;	/* read next name */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ)
		{
		    fclose(fp);
		    return 0;
		}
	    }
	    break;
	}
	esc= (ch == '\\');
    }

    /* group not found */
    fclose(fp);
    return 0;
}


/* INGROUPNAME - Given a group name return true if the given user with the
 * given group id is in that group.
 *
 * File version - do character-at-a-time reading to handle really long lines
 * without buffer overflow.
 */

int ingroupname(char *groupname, gid_t my_gid, char *my_login)
{
    FILE *fp;
    char bf[BFSZ+1];
    int state, ch, i, esc;

    if (groupname == NULL || groupname[0] == '\0') return 0;
    if (my_login == NULL || my_login[0] == '\0') return 0;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
    {
	/* No file - use defaults */
	return
#if defined(CFADM_GID) && defined(CFADM_GROUP)
	    (my_gid == CFADM_GID && !strcmp(groupname,CFADM_GROUP)) ||
#endif
#if defined(USER_GID) && defined(USER_GROUP)
	    (my_gid == USER_GID && !strcmp(groupname,USER_GROUP)) ||
#endif
#if defined(GRADM_GID) && defined(GRADM_GROUP)
	    (my_gid == GRADM_GID && !strcmp(groupname,GRADM_GROUP)) ||
#endif
	    0;
    }

    state= 0;
    esc= 0;
    while ((ch= fgetc(fp)) != EOF)
    {
    	switch (state)
	{
	case -1: /* skip to start of line */
	    if (ch == '\n' && !esc) state= 0;
	    break;

	case 0: /* ready to read group name */
	    if (ch == '#') {state= -1; break;}
	    state= 1;
	    i= 0;
	    /* drop through */

	case 1: /* reading group name */
	    if (ch == ':')
	    {
		bf[i]= '\0';
		if (strcmp(bf, groupname))
		    state= -1;	/* group name doesn't match - goto next line */
		else
		    state= 10;	/* group name matches - read gid */
	    }
	    else if (ch == '\n')
	    {
		if (!esc)
		    state= 0;
		else if (i > 0)
		    i--;
	    }
	    else
	    {
		bf[i++]= ch;
		if (i >= BFSZ) state= -1;
	    }
	    break;

	case 10: /* skip to next colon (OK, the state number is out of seq) */
	    if (ch == ':') state= 2;
	    break;

	case 2: /* ready to read gid */
	    state= 3;
	    i= 0;
	    /* drop through */

	case 3: /* reading gid number */
	    if (ch == ':' || (ch == '\n' && !esc))
	    {
		bf[i]= '\0';
		/* check gid */
		if (my_gid == atoi(bf))
		{
		    fclose(fp);
		    return 1;
		}
		state= 4;	/* check names */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ) state= -1;
	    }
	    break;

	case 4: /* ready to read a name */
	    state= 5;
	    i= 0;

	case 5: /* reading a name */
	    if (ch == ',' || (ch == '\n' && !esc))
	    {
	    	bf[i]= '\0';
		if (!strcmp(my_login, bf))
		{
		    fclose(fp);
		    return 2;
		}

		if (ch == '\n')
		{
		    /* no more names */
		    fclose(fp);
		    return 0;
		}
		else
		    state= 4;	/* read next name */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ)
		{
		    fclose(fp);
		    return 0;
		}
	    }
	    break;
	}
	esc= (ch == '\\');
    }

    /* group not found */
    fclose(fp);
    return 0;
}


/* INGROUPLIST - Given a NULL-terminated array of pointers to group names and
 * an ID, return true if that id is in any of those groups.
 *
 * File version.
 */

int ingrouplist(char **groups, gid_t my_gid, char *my_login)
{
    FILE *fp;
    char bf[BFSZ+1];
    int state, ch, i, esc, j;

    if (groups == NULL || groups[0] == NULL) return 0;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
    {
	/* No file - use defaults */
	for (i= 0; groups[i] != NULL; i++)
	{
#if defined(CFADM_GID) && defined(CFADM_GROUP)
	    if (my_gid == CFADM_GID && !strcmp(groups[i],CFADM_GROUP))
	    	return 1;
#endif
#if defined(USER_GID) && defined(USER_GROUP)
	    if (my_gid == USER_GID && !strcmp(groups[i],USER_GROUP))
	    	return 1;
#endif
#if defined(GRADM_GID) && defined(GRADM_GROUP)
	    if (my_gid == GRADM_GID && !strcmp(groups[i],GRADM_GROUP))
	    	return 1;
#endif
	}
	return 0;
    }

    state= 0;
    esc= 0;
    while ((ch= fgetc(fp)) != EOF)
    {
    	switch (state)
	{
	case -1: /* skip to start of line */
	    if (ch == '\n' && !esc) state= 0;
	    break;

	case 0: /* ready to read group name */
	    if (ch == '#') {state= -1; break;}
	    state= 1;
	    i= 0;
	    /* drop through */

	case 1: /* reading group name */
	    if (ch == ':')
	    {
		bf[i]= '\0';
		state= -1;   /* if group name doesn't match - goto next line */
		for (j= 0; groups[j] != NULL; j++)
		{
		    if (!strcmp(bf, groups[j]))
		    {
			state= 10;	/* group name matches - read gid */
			break;
		    }
		}
	    }
	    else if (ch == '\n')
	    {
		if (!esc)
		    state= 0;
		else if (i > 0)
		    i--;
	    }
	    else
	    {
		bf[i++]= ch;
		if (i >= BFSZ) state= -1;
	    }
	    break;

	case 10: /* skip to next colon (OK, the state number is out of seq) */
	    if (ch == ':') state= 2;
	    break;

	case 2: /* ready to read gid */
	    state= 3;
	    i= 0;
	    /* drop through */

	case 3: /* reading gid number */
	    if (ch == ':' || (ch == '\n' && !esc))
	    {
		bf[i]= '\0';
		/* check gid */
		if (my_gid == atoi(bf))
		{
		    fclose(fp);
		    return 1;
		}
		state= 4;	/* check names */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ) state= -1;
	    }
	    break;

	case 4: /* ready to read a name */
	    state= 5;
	    i= 0;

	case 5: /* reading a name */
	    if (ch == ',' || (ch == '\n' && !esc))
	    {
	    	bf[i]= '\0';
		if (!strcmp(my_login, bf))
		{
		    fclose(fp);
		    return 1;
		}

		if (ch == '\n')
		{
		    state= 0;
		}
		else
		    state= 4;	/* read next name */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ)
		{
		    fclose(fp);
		    return 0;
		}
	    }
	    break;
	}
	esc= (ch == '\\');
    }

    /* group not found */
    fclose(fp);
    return 0;
}


/* FOR_GROUPS - Call a given function once with each currently defined group
 * name.
 *
 * File version.
 */

void for_groups(void (*func)(char *))
{
    FILE *fp;
    static char bf[BFSZ+1];
    char *p, *q;
    int continued= 0;
    int len;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
    {
	/* No file - use defaults */
#ifdef CFADM_GROUP
	(*func)(CFADM_GROUP);
#endif
#if defined(USER_GID) && defined(USER_GROUP)
	(*func)(USER_GROUP);
#endif
#if defined(GRADM_GID) && defined(GRADM_GROUP)
	(*func)(GRADM_GROUP);
#endif
    	return;
    }

    while (fgets(bf, BFSZ, fp) != NULL)
    {
	len= strlen(bf);
    	if (!continued && bf[0] != '#' && (p= strchr(bf,':')) != NULL)
	{
	    *p= '\0';
	    (*func)(bf);
	}
	continued= ((bf[len-1] != '\n') || (bf[len-2] == '\\'));
    }
    fclose(fp);
}

#if !NOPWEDIT

/* ==================== LOW-LEVEL GROUP EDIT FUNCTIONS ==================== */


/* A structure to use when loading groups into memory */
struct grp {
        char *name;
        int gid;
        char **members;         /* NULL terminated array of pointers */
        struct grp *next;
        };


/* CHECK_GROUP_NAME - Do syntax checks on a group name.  No upper case letters,
 * only a few symbols, first character must be a letter.  There is no length
 * limit for Backtalk group names.  There would be lots more restrictions on
 * unix group names, but we don't administer those.
 */

static int check_group_name(char *name)
{
    static char *oksym= "_-.@";

    if (name == NULL || name[0] =='\0' || !isascii(name[0]) ||!islower(name[0]))
	return 1;

    for (name++; *name != '\0'; name++)
    {
	if (!isascii(*name) || (!islower(*name) && !isdigit(*name) &&
		!strchr(oksym,*name)))
	    return 1;
    }
    return 0;
}


/* FREE_GRP - Deallocate a single group structure.
 */

static void free_grp(struct grp *g)
{
    char **m;

    if (g->name != NULL) free(g->name);
    if (g->members != NULL)
    {
	for (m= g->members; *m != NULL; m++)
	    free(*m);
    	free(g->members);
    }
    free(g);
}


/* FREE_GRPS - deallocate a whole list of groups.
 */

void free_grps(struct grp *head)
{
    struct grp *g, *n;
    char **m;

    for (g= head; g != NULL; g= n)
    {
	n= g->next;
	free_grp(g);
    }
}


/* APPEND_GRP - Append a group to the list whose head and tail pointers
 * are given.  The members array and name should already be in malloc'ed
 * memory.
 */

static void append_grp(struct grp **head, struct grp **tail,
    char *name, gid_t gid, char **mem)
{
    struct grp *new= (struct grp *)malloc(sizeof(struct grp));
    new->name= name;
    new->gid= gid;
    new->members= mem;
    new->next= NULL;

    if (*head == NULL)
    	*head= new;
    else
        (*tail)->next= new;

    *tail= new;
}


/* LOAD_GRPS - Load the group file into memory.  Return a pointer to the
 * head of the linked list of group structures.  Can handle arbitrarily long
 * lines, but group and user names are limited to something like 1024
 * characters.
 */

struct grp *load_grps()
{
    FILE *fp;
    struct grp *tail, *head= NULL;
    char bf[BFSZ+1];
    char *name= NULL, **mem= NULL;
    int memi,memmax;
    gid_t gid;
    int state, ch, i, esc, j;

    if ((fp= fopen(GROUP_FILE,"r")) == NULL)
    {
    	/* Pretend we loaded the defaults */
#if defined(CFADM_GID) && defined(CFADM_GROUP)
	append_grp(&head,&tail,strdup(CFADM_GROUP),CFADM_GID,NULL);
#endif
#if defined(USER_GID) && defined(USER_GROUP)
	append_grp(&head,&tail,strdup(USER_GROUP),USER_GID,NULL);
#endif
#if defined(GRADM_GID) && defined(GRADM_GROUP)
	append_grp(&head,&tail,strdup(GRADM_GROUP),GRADM_GID,NULL);
#endif
	return head;
    }

    state= 0;
    esc= 0;
    while ((ch= fgetc(fp)) != EOF)
    {
    	switch (state)
	{
	case -1: /* skip to start of line */
	    if (ch == '\n' && !esc) state= 0;
	    break;

	case 0: /* ready to read group name */
	    if (ch == '#') {state= -1; break;}
	    state= 1;
	    i= 0;
	    /* drop through */

	case 1: /* reading group name */
	    if (ch == ':')
	    {
		bf[i]= '\0';
		if (name != NULL) free(name);
		name= strdup(bf);
		state= 10;	/* get gid next */
	    }
	    else if (ch == '\n')
	    {
		if (!esc)
		    state= 0;
		else if (i > 0)
		    i--;
	    }
	    else
	    {
		bf[i++]= ch;
		if (i >= BFSZ) state= -1;
	    }
	    break;

	case 10: /* skip to next colon (OK, the state number is out of seq) */
	    if (ch == ':') state= 2;
	    break;

	case 2: /* ready to read gid */
	    state= 3;
	    i= 0;
	    /* drop through */

	case 3: /* reading gid number */
	    if (ch == ':' || (ch == '\n' && !esc))
	    {
		bf[i]= '\0';
		/* save gid */
		gid= atoi(bf);
		state= 4;	/* get names */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ) state= -1;
	    }
	    break;

	case 4: /* ready to read a name */
	    state= 5;
	    i= 0;

	case 5: /* reading a name */
	    if (ch == ',' || (ch == '\n' && !esc))
	    {
		if (i > 0)
		{
		    bf[i]= '\0';
		    if (mem == NULL)
		    {
			memmax= 10;
			memi= 0;
			mem= (char **)malloc(memmax*sizeof(char *));
		    }
		    else if (memi > memmax - 1)
		    {
			memmax*= 2;
			mem= (char **)realloc(mem,memmax*sizeof(char *));
		    }
		    mem[memi++]= strdup(bf);
		}

		if (ch == '\n')
		{
		    /* Line done - append it */
		    state= 0;
		    if (mem != NULL) mem[memi]= NULL;
		    append_grp(&head,&tail,name,gid,mem);
		    name= NULL;
		    mem= NULL;
		}
		else
		    state= 4;	/* read next name */
	    }
	    else if (ch == '\n')	/* line ends with \ */
	    {
		if (i > 0) i--;
	    }
	    else
	    {
	    	bf[i++]= ch;
	    	if (i >= BFSZ) state= -1;
	    }
	    break;
	}
	esc= (ch == '\\');
    }

    /* all done */
    fclose(fp);
    return head;
}


/* WRITE_GRPS - write a group structure back to the file.  If 'freemem' is true,
 * deallocate the memory for the structure.
 */

void write_grps(struct grp *head, int freemem)
{
    struct grp *g, *n;
    char **m;
    FILE *fp;

    if ((fp= fopen(GROUP_FILE,"w")) == NULL)
    	die("cannot write to "GROUP_FILE);

    for (g= head; g != NULL; g= n)
    {
    	fprintf(fp,"%s::%d:",g->name,g->gid);
	if (g->members != NULL)
	{
	    for (m= g->members; *m != NULL; m++)
	    {
		if (m != g->members) fputc(',',fp);
		fputs(*m,fp);
		if (freemem) free(*m);
	    }
	    if (freemem) free(g->members);
	}
	fputc('\n',fp);
	n= g->next;
	if (freemem) {free(g->name); free(g);}
    }
    fclose(fp);
}


/* ADD_GRP - Add a new group (with no members) to the list.  GID is
 * automatically generated.  Returns 1 if the group name already exists.
 * Names need not be in malloc'ed memory.
 */

int add_grp(struct grp **head, char *name)
{
    struct grp *g, *tail= NULL;
    gid_t gid= 100;

    /* Scan through existing groups, checking that name is used, and finding
     * a free gid number */
    for (g= *head; g != NULL; tail= g, g= g->next)
    {
    	if (g->gid > gid) gid= g->gid;
    	if (!strcmp(name,g->name))
	    return 1;
    }
    gid++;
    append_grp(head,&tail,strdup(name),gid,NULL);
    return 0;
}


/* DEL_GRP - Delete the named group.  Returns 1 if it doesn't exit.
 */

int del_grp(struct grp **head, char *name)
{
    struct grp *g, *p= NULL;

    for (g= *head; g != NULL; p= g, g= g->next)
    {
    	if (!strcmp(name,g->name))
	{
	    if (p == NULL)
	    	*head= NULL;
	    else
	    	p->next= g->next;
	    free_grp(g);
	    return 0;
	}
    }
    return 1;
}


/* ADD_MEMB - Add a member to an existing group on the list.
 * Returns 1 if the group name doesn't exist.
 * Names need not be in malloc'ed memory.
 */

int add_memb(struct grp *head, char *gname, char *mname)
{
    struct grp *g;
    char **m;
    int n;

    for (g= head; g != NULL; g= g->next)
    {
    	if (!strcmp(gname,g->name))
	{
	    if (g->members == NULL)
	    {
		n= 0;
	    	g->members= (char **)malloc(2*sizeof(char *));
	    }
	    else
	    {
		/* Count current members */
		for (m= g->members, n= 0; *m != NULL; m++, n++)
		    ;
		/* Enlarge array */
		g->members= (char **)realloc(g->members,(n+2)*sizeof(char *));
	    }
	    g->members[n]= strdup(mname);
	    g->members[n+1]= NULL;
	    return 0;
	}
    }
    return 1;
}


/* DEL_MEMB - Delete a member to an existing group on the list.
 * Returns 1 if the group name doesn't exist or the member isn't in it.
 */

int del_memb(struct grp *head, char *gname, char *mname)
{
    struct grp *g;
    char **m;

    for (g= head; g != NULL; g= g->next)
    {
    	if (!strcmp(gname,g->name))
	{
	    if (g->members == NULL) return 1;
	    /* Count current members */
	    for (m= g->members; *m != NULL; m++)
	    {
		if (!strcmp(mname,*m))
		{
		    free(*m);
		    for (; *m != NULL; m++)
		    	m[0]= m[1];
		    m[-1]= NULL;
		    return 0;
		}
	    }
	    return 1;
	}
    }
    return 1;
}

/* ==================== HIGH-LEVEL GROUP EDIT FUNCTIONS ==================== */

/* NEWGROUP(gname,members)
 *   Creates a new group with the given name and list of members.  Returns
 *   0 on success, 1 if the group already exists.
 */

int newgroup(char *gname)
{
    struct grp *groups;
    int i;

    groups= load_grps();
    if (add_grp(&groups,gname))
    {
        free_grps(groups);
    	return 1;
    }
    else
    {
        write_grps(groups,1);
	return 0;
    }
}


/* NEWGROUPMEM(gname,member)
 *   Add a user to an existing group.  Returns 0 on success, 1 if the group
 *   does not exists.
 */

int newgroupmem(char *gname, char *mem)
{
    struct grp *groups;

    groups= load_grps();
    if (add_memb(groups,gname,mem))
    {
        free_grps(groups);
    	return 1;
    }
    else
    {
        write_grps(groups,1);
    	return 0;
    }
}


/* DELGROUP(gname)
 *   Delete a group and all its members.  Returns 0 on success.
 */

int delgroup(char *gname)
{
    struct grp *groups;

    groups= load_grps();
    if (del_grp(&groups,gname))
    {
        free_grps(groups);
    	return 1;
    }
    else
    {
        write_grps(groups,1);
    	return 0;
    }
}


/* DELGROUPMEM(gname, member)
 *   Remove a user from a group.  Returns 0 on success, 1 if the group
 *   does not exists or the user was not a member of it.
 */

int delgroupmem(char *gname, char *member)
{
    struct grp *groups;

    groups= load_grps();
    if (del_memb(groups,gname,member))
    {
        free_grps(groups);
    	return 1;
    }
    else
    {
        write_grps(groups,1);
    	return 0;
    }
}

#endif /* !NOPWEDIT */
