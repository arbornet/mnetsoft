#include <stdio.h>

char *getenv(), *index(), *strdup();
char x2c(char *what);
void unescape_url(char *url);
void plustospace(char *str);

/* LOW-LEVEL CGI-INTERFACE
 *
 * This module contains code to decode cgi data for either GET or POST
 * methods.  It is based very loosely on the stuff in the ncsa-defaults.tar.Z
 * distribution, but should be more secure, if slightly slower.  Code has
 * also been borrowed from Steve Weiss.
 *
 * All programs should first call:
 *   load_query(get_query());         - Read in the query string.
 *
 * Then they can make calls to:
 *   val= getvar(var);     - Return value of a variable (NULL if undefined).
 *   firstvar(&var,&val);  - Return first argument (return 0 if none).
 *   nextvar(&var,&val);   - Return next argument (return 0 if end-of-list).
 *
 * And after all desired scanning of arguments is complete, and no further
 * references to the returned variable names or values will be needed:
 *   free_query();         - Deallocate temporary storage.
 */

/* VERSION HISTORY:
 * 03/29/96 - Last version before I started recording version history. [jdw]
 * 06/24/96 - Added version history.
 */

/* This the structure in which load_query() temporarily stores the parsed
 * query - a linked list of pointers to name and value strings.
 */

typedef struct entstr {
	char *name;		/* variable name */
	char *value;		/* variable value */
	struct entstr *next;
	} entry;

entry *head;		/* Head of entry list */
char *buffer;		/* Memory area used to store strings in entry list */

/* GET_QUERY - fetch the raw query string and return it.  It is taken either
 * from the QUERY_STRING or from stdin, depending on weather the
 * REQUEST_METHOD is POST or GET.
 */

char *get_query()
{
char *query;
char *method= getenv("REQUEST_METHOD");

	if (method != NULL && !strcmp(method,"POST"))
	{
		char *cl= getenv("CONTENT_LENGTH");
		int len= (cl == NULL) ? 0 : atol(cl);
		query= (char *)malloc(len+1);
		len= fread(query,1,len,stdin);
		query[len]= '\0';
	}
	else
		query= getenv("QUERY_STRING");

	return (query);
}


/* LOAD_QUERY - given a raw query string, decode it into a linked list of
 * entry fields.  After this has been called, the getvar(), firstvar(), and
 * nextvar() routine may be called to pick out particular elements of the
 * query string.  This allocates all it's own memory, so when you are done
 * with the *var() calls, free_query() should be called.
 */

void load_query(char *query)
{
char *p, *end;
entry *new;

	buffer= NULL;
	head= NULL;

	if  (query == NULL || query[0] == '\0')
		return;
	
	/* Make local copy of the string - don't want to hack up environment */
	buffer= strdup(query);

	p= buffer;
	do {
		new= (entry *)malloc(sizeof(entry));
		new->next= head;
		head= new;

		new->name= p;

		if ((end= index(p,'&')) != NULL) *end= '\0';

		plustospace(p);
		unescape_url(p);

		if ((p= index(p,'=')) == NULL)
			new->value= "";
		else
		{
			*p= '\0';
			new->value= p + 1;
		}
		p= end + 1;
	} while (end != NULL);

	return;
}


/* FREE_QUERY - OK, so the name makes no sense, but this deallocates all the
 * memory load_query() allocated.  Don't do this until you are done with the
 * entries list.
 */

void free_query()
{
entry *p,*n;

	if (buffer != NULL) free(buffer);

	for (p= head; p != NULL; p= n)
	{
		n= p->next;
		free(p);
	}
}


/* GETVAR - Return the value associated with a variable.  Returns NULL if the
 * variable is undefined, and a pointer to an empty string if the variable is
 * defined but has no value.  This must be called after load_query() but
 * before free_query().
 */

char *getvar(char *name)
{
entry *v;

	for (v= head; v != NULL; v= v->next)
		if (!strcmp(name,v->name))
			return(v->value);
	return(NULL);
}


/* FIRSTVAR - Return the first name/value pair in the query string.  Well,
 * actually it's the last, because we reverse it when we decode it.  This
 * is meant to initialize a scan through all entries, using the nextvar()
 * call.  Returns 0 if list empty, 1 otherwise.
 */

static entry *curr;	/* last query returned */

int firstvar(char **name, char **value)
{
	curr= head;
	if (curr == NULL) return(0);
	*name= curr->name;
	*value= curr->value;
	return(1);
}


/* NEXTVAR - After calling firstvar() once, you can call this to get the
 * next values.  It returns 0 when you hit the end of the list, 1 otherwise.
 */

int nextvar(char **name, char **value)
{
	if (curr == NULL) return(0);
	curr= curr->next;
	if (curr == NULL) return(0);
	*name= curr->name;
	*value= curr->value;
	return(1);
}



/* STRDUP Many older Unixes don't have strdup.  But it is easy to define. 
 */

#if defined(NeXT)
char *strdup(char *old)
{
char *new;

	new= (char *)malloc(strlen(old)+2);
	strcpy(new,old);
}
#endif


/* ------- The rest of this is recycled as is from the NCSA stuff ------- */

char x2c(char *what) {
    register char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
    return(digit);
}

void unescape_url(char *url) {
    register int x,y;

    for(x=0,y=0;url[y];++x,++y) {
        if((url[x] = url[y]) == '%') {
            url[x] = x2c(&url[y+1]);
            y+=2;
        }
    }
    url[x] = '\0';
}

void plustospace(char *str) {
    register int x;

    for(x=0;str[x];x++) if(str[x] == '+') str[x] = ' ';
}
