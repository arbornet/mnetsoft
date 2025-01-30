/* Copyright 2004, Jan D. Wolter, All Rights Reserved. */

#include "common.h"
#include "tag.h"
#include "readfile.h"
#include "lock.h"
#include "str.h"

#include <ctype.h>
#include <errno.h>
#include <pwd.h>
#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif


/* OPEN_TAG - given the file name of a tag file, prepare for a scan.
 * This actually loads the whole file into memory.
 */

rtag *open_tag(char *fname, int lock)
{
    rtag *rt= (rtag *) malloc(sizeof(rtag));

    if ((rt->buf= read_file(fname, lock)) == NULL)
    {
	free(rt);
	return NULL;
    }
    rt->p= rt->buf;
    return rt;
}


/* CLOSE_TAG - close a tagfile structure */

void close_tag(rtag *rt)
{
    if (rt == NULL) return;
    if (rt->buf != NULL) free(rt->buf);
    free(rt);
}

/* GET_TAG - Get the next tag from the tag file.  Returns:
 *     -1  on Error
 *      0  on EOF
 *      1  on string value
 *      2  on integer value (returned as string)
 *      3  on protected string
 *      4  on protected integer
 * Note that string values are unescaped before being returned.
 */

int get_tag(rtag *rt, char **var, char **val)
{
    char *end, closep;
    int is_string, protect;

    if (*rt->p == '\0') return 0;

    *var= rt->p;

    /* Parse file line */
    if (protect= (**var == '*')) (*var)++;
    if ((*val= strchr(*var,'=')) == NULL)
	return -1;

    /* These days strings are enclosed in ()'s, but they used to be enclosed
     * in {} in some tag files.  We accept either while reading.
     */
    if ((*val)[1] == '(')
	is_string= ')';
    else if ((*val)[1] == '{')
	is_string= '}';
    else
	is_string= 0;

    if (is_string) {
	int esc= 0;
	for (end=(*val)+1; *end != '\0' && (esc || *end != is_string); end++)
	    esc= (*end == '\\' && !esc);
	if (end[0] == '\0' || end[1] != '\n' )
	    return -1;
	end++;
    }
    else
    {
	end= strchr(*var,'\n');
	if (end == NULL || *val > end)
	    return -1;
    }

    /* Terminate variable name */
    **val= '\0';

    /* Update current position pointer */
    rt->p= end+1;

    if (is_string)
    {
	(*val)+= 2;
	*(end-1)= '\0';
	unesctag(*val,*val,0);
	return protect ? 3 : 1;
    }
    else
    {
	(*val)++;
	*end= '\0';
	return protect ? 4 : 2;
    }
}


/* WRTSTRTAG - Write a string as a tag line */

void wrtstrtag(FILE *fp, char *name, char *value, int protect)
{
    fprintf(fp,"%s%s=(", protect?"*":"", name);
    while (*value != '\0')
    {
	switch (*value)
	{
	case '\r':                   break;
	case '\n': fputs("\\n",fp);  break;
	case '\\': fputs("\\\\",fp); break;
	case '(':  fputs("\\(",fp);  break;
	case ')':  fputs("\\)",fp);  break;
	default:   fputc(*value,fp); break;
	}
	value++;
    }
    fputs(")\n",fp);
}


/* WRTINTTAG - Write an integer as a tag line */

void wrtinttag(FILE *fp, char *name, int value, int protect)
{
    fprintf(fp,"%s%s=%d\n", protect?"*":"", name, value);
}


/* WRTTAGLINE - write a tag line corresponding to a tag list entry.
 */

void wrttagline(FILE *fp, taglist *tag, int protect)
{
    if (tag->type == 0)
	wrtstrtag(fp, tag->name, tag->val.s, protect);
    else
	wrtinttag(fp, tag->name, tag->val.i, protect);
}



/* UNESCTAG - unexpand all \\, \(, \) and \n escapes in the given string,
 * while copying it from f to t.  f and t can be the same place.  If proc is
 * true, we do \{ and \} instead of \( and \).
 */

void unesctag(char *f, char *t, int proc)
{
    char left=  "{("[!proc];
    char right= "})"[!proc];

    for (; *f != '\0'; t++, f++)
    {
	if (*f == '\\')
	{
	    f++;
	    if (*f == 'n')
		*t= '\n';
	    else if (*f == left)
		*t= left;
	    else if (*f == right)
		*t= right;
	    else if (*f == '\\')
		*t= '\\';
	    else
	    {
		*(t++)= '\\';
		*t= *f;
	    }
	}
	else
	    *t= *f;
    }
    *t= '\0';
}

/* ADD_STR_TAG
 * ADD_INT_TAG
 * Add a tag value to the given list.  All memory management of values is
 * the caller's problem.  This routines assume all the values passed in 
 * will exist for the life of the list.
 */

void add_str_tag(taglist **tagl, char *name, char *val)
{
    taglist *new, *p;

    for (p= *tagl; p != NULL; p= p->next)
    {
	if (!strcmp(p->name, name))
	{
	    p->type= 0;
	    p->val.s= val;
	}
    }

    new= (taglist *)malloc(sizeof(taglist));
    new->name= name;
    new->type= 0;
    new->val.s= val;

    new->next= *tagl;
    *tagl= new;
}


void add_int_tag(taglist **tagl, char *name, int val)
{
    taglist *new, *p;

    for (p= *tagl; p != NULL; p= p->next)
    {
	if (!strcmp(p->name, name))
	{
	    p->type= 1;
	    p->val.i= val;
	}
    }

    new= (taglist *)malloc(sizeof(taglist));
    new->name= name;
    new->type= 1;
    new->val.i= val;

    new->next= *tagl;
    *tagl= new;
}



/* UPDATE_TAG - Update the given tag file with the given list of new
 * values.  New values replace old values of the same name or are appended.
 * Protected values can only be overwritten if amadm is true.  The taglist
 * will be deallocated as a side effect of this function.
 */

void update_tag(char *fname, taglist **tagl, int amadm)
{
    rtag *rt;
    char *var, *val;
    FILE *fp;
    int rc, protect, type;
    taglist *c, *p, *n;

    /* If no new values, nothing needs to be done */
    if (tagl == NULL || *tagl == NULL) return;

    /* Load existing file (if any) into memory */
    rt= open_tag(fname, TRUE);

    /* Truncate and prepare to rewrite */
    if ((fp= fopen(fname,"w+")) == NULL)
	die("Cannot open file %s to write",fname);
    lock_exclusive(fileno(fp),fname);

    /* If the file exists, write it back, with updates */
    if (rt != NULL)
    {
	while ((rc= get_tag(rt, &var, &val)) > 0)
	{
	    protect= (rc > 2);
	    type= ((rc+1) & 1);  /* 0=string, 1= integer */

	    /* Check if tag's value has been changed */
	    for (c= *tagl, p= NULL;
		 c != NULL && strcmp(c->name,var);
		 p= c, c= c->next) { }

	    if (c != NULL && (amadm || !protect))
	    {
		/* Found it in taglist - Write out the new value */
		wrttagline(fp, c, protect);

		/* Delete item from list */
		if (p == NULL)
		    *tagl= c->next;
		else
		    p->next= c->next;
		free(c);
	    }
	    else if (type == 0)
	    {
		/* Write old string value back out */
		wrtstrtag(fp, var, val, protect);
	    }
	    else
	    {
		/* Write old intger value back out */
		wrtinttag(fp, var, atoi(val), protect);
	    }
	}
    }
    close_tag(rt);

    /* Write out new values */
    for (c= *tagl; c != NULL; c= n)
    {
	n= c->next;
	wrttagline(fp, c, 0);
	free(c);
    }

    fclose(fp);
    unlock(fname);
}
