/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <errno.h>

#include "comp.h"
#include "comp_tok.h"
#include "str.h"
#include "byteorder.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

CToken head;			/* Head of compiled program */
CToken *curr;			/* End (so far) of compiled program */
int startproc;			/* Did we just start a new subprocess? */
int last_file;			/* file index of last token written */
int write_error;		/* Non-zero error code if any writes failed */


/* INIT_COMP - Initialize the compilation data structure by putting in a
 * procedure token for the whole program.
 */

void init_comp(int sfid)
{
    head.t.flag= TK_PROCEDURE;
    head.t.sfid= sfid;
    head.t.line= 0;
    head.t.val= (void *)NULL;
    head.parent= NULL;
    head.prev= NULL;
    head.next= NULL;

    curr= &head;
    startproc= 1;
    write_error= 0;
}


/* ADD_TOK - Add the given token to the end of the current program.  The
 * startproc flag, if true, indicates that we should add the token as the
 * first child of the current TK_PROCEDURE token.  Otherwise we add it
 * after the current token.
 */

CToken *add_tok(word flag, byte file, word line, void *val)
{
    CToken *new= (CToken *) malloc(sizeof(CToken));

    new->t.flag= flag;
    new->t.val= val;
    new->t.line= line;
    new->t.sfid= file;

    add_ctoken(new);
    return curr;
}


/* ADD_CTOKEN - Insert an existing ctoken (see add_tok comments).
 */

void add_ctoken(CToken *new)
{
    if (startproc)
    {
	curr->t.val= (void *)new;
	new->parent= curr;
	new->prev= NULL;
	startproc= 0;
    }
    else
    {
	curr->next= (void *)new;
	new->prev= curr;
	new->parent= curr->parent;
    }
    new->next= NULL;
    curr= new;
}


/* START_PROC - Append a TK_PROC token, and set a switch so that the next
 * added token will be added inside it.
 */

void start_proc(byte file, word line)
{
    add_tok(TK_PROCEDURE, file, line, (void *)NULL);
    startproc= 1;
}


/* END_PROC - Indicate an end to the current procedure, so that the next
 * token will be added after the parent TK_PROC token instead of after
 * the current token.  Returns TRUE if we have just ended the root
 * procedure.
 */

int end_proc()
{
    if (curr == &head) return 1;

    if (startproc)
	startproc= 0;
    else
	curr= curr->parent;

    return(curr == &head);
}


/* DO_INLINE -- Include another file in-line inside this file.  This is either
 * the result of an "include", "stopped" or "chain" command.  We first load
 * the whole file in as a single PROCEDURE token, then append an integer
 * constant which is the index into the filename list of the file this was
 * loaded from, and then we put on the bound literal given by the <cmd>
 * parameter.  This will normally be either "@include", "@stopped" or "@chain".
 * <line> is the line number of the include command in the parent file.
 */

int insp= 0;

void do_inline(int cmd, char *filename, byte file, word line)
{
    int i, new_file, old_file;
    char *source;
    extern int curr_file;
    FILE *fp;
    static stack[BFSZ];

    if (sb_name(filename, &source, NULL))
	die("included file name \"%s\" too long",filename);

    if ((fp= fopen(source,"r")) == NULL)
	die("included file \"%s\" does not exist",source);

    /* Store the filename in the list that will go into the header */
    old_file= curr_file;
    new_file= curr_file= add_file_name(source);

    /* Check inline stack to make sure we aren't in an include loop */
    for (i= 0; i < insp; i++)
	if (stack[i] == curr_file)
	    die("file %s included in itself",filename);

    /* Add this to the include file stack */
    if (insp >= BFSZ) die("include stack overflow");
    stack[insp++]= curr_file;

    /* Start a child procedure to put the included file into */
    start_proc(file, line);

    /* Load the included file */
    compile(fp);

    /* Point back to parent file (for error messages) */
    curr_file= old_file;
    insp--;

    if (end_proc()) /* { */
	die("unmatched } in included file %s",filename);

    /* Append command */
    add_tok(TK_BOUND_SYMBOL, file, line, (void *)(long)cmd);
}


/* TOKEN_SIZE -- return the number of bytes the given token will be when
 * it is written out.
 */

#ifdef UNUSED
int token_size(CToken *p)
{
    /* Procedures:  four byte header, four bytes size, plus recursive */
    if (type(p->t) == TKC_PROCEDURE)
    {
	CToken *q;
	int size= 8;

	for (q= (CToken *)p->t.val; q != NULL; q= q->next)
	    size+= token_size(q);

	return size;
    }

    /* Integers and bound symbols:  4 byte header, 4 byte value/offset */
    if (class(p->t) == TKC_INTEGER)
	return 8;

    /* Strings and unbound symbols: 4 byte header, 4 byte size,
       plus string length */
    if (class(p->t) == TKC_STRING)
	return 8 + strlen((char *)p->t.val);

    die("bug: cannot size token");
}
#endif


/* PROC_LEN -- return the number of tokens in the top level of a procedure.
 */

int proc_len(CToken *p)
{
    CToken *q;
    int len= 0;

    for (q= (CToken *)p->t.val; q != NULL; q= q->next)
	len++;
    return len;
}


/* WRITE_CHECK -- Do a write, but set the global variable "write_error" if
 * we get an error.
 */

int write_check(int fd, char *buf, size_t count) 
{
    int rc;

    rc= write(fd,buf,count);
    if (rc != count)
	write_error= errno;
    return rc;
}



/* WRITE_VAL -- Write out an unsigned integer of size 1, 2, 3, or 4 bytes
 * with big-endian byte order (most significant byte first).  We have a
 * version for big-endian machines (eg, sparcs) and for little-endian machines
 * (eg, intels).  This won't work for pdp machines, but does anyone care?
 */

int write_val(int fd, int size, int val)
{
    if (big_endian)
	/* BIG-ENDIAN - write it as it */
	return write_check(fd, ((char *)(&val))+sizeof(val)-size, size);
    else
    {
	/* LITTLE-ENDIAN - shuffle into big endian */
	unsigned char bf[4];
	int i;

	for (i= 0; i < size; i++)
	    bf[i]= ((char *)(&val))[size-i-1];
	return write_check(fd, (char *)bf, size);
    }
}


/* WRITE_TOKEN -- Write out a token from the data structure.
 */

void write_token(int fd, CToken *p)
{
    int len;
    char *r;
    CToken *q;

    if (last_file != p->t.sfid)
    {
	/* Write out a file change token */
	write_val(fd, 2, TK_UNDEF);
	write_val(fd, 1, p->t.sfid);
	last_file= p->t.sfid;
    }

    write_val(fd, 2, p->t.flag);
    write_val(fd, 2, p->t.line);

    if (type(p->t) == TK_PROCEDURE)
    {
	/* Procedures */
	write_val(fd, 4, proc_len(p));
	for (q= (CToken *)p->t.val; q != NULL; q= q->next)
	    write_token(fd,q);
    }
    else if (class(p->t) == TKC_INTEGER)
    {
	/* Integers and bound symbols */
	write_val(fd, 4, (int)ival(p->t));
    }
    else if (class(p->t) == TKC_STRING)
    {
	/* Strings and unbound symbols */
	len= strlen(sval(p->t));
	write_val(fd, 4, len);
	write_check(fd,sval(p->t),len);
    }
    else if (class(p->t) == TKC_REGEX)
    {
	/* Regular Expresssions */
	r= regtoflat(&(rval(p->t)->re), &len);
	if (r == NULL)
	    die("cannot write bad regex");
	write_val(fd, 4, len);
	write_check(fd, r, len);
    }
    else
	die("bug: cannot write token type (%d)",p->t);
}


/* WRITE_PROG -- Write out the compiled program to the given file descriptor.
 * Return non-zero errno code if any writes failed (usually because of a full
 * disk).
 */

int write_prog(int fd)
{
    /* Write Header */
    write_check(fd, MAGIC, 4);
    write_val(fd, 1, VERSION_A);
    write_val(fd, 1, VERSION_B);
    write_val(fd, 1, VERSION_C);
    write_val(fd, 1, VERSION_D);

    /* Write List of File Names */
    write_file_list(fd);

    /* Write out the program */
    last_file= -1;
    write_token(fd, &head);

    return write_error;
}


/* GET_TOKEN -- return a pointer to one of the previous tokens.  An index
 * of 1 returns the current token, 2 returns the previous one, and so forth.
 * Returns NULL if there aren't that many previous tokens in the current
 * procedure.
 */

#ifdef UNUSED
Token *get_token(int offset)
{
    CToken *p= curr;

    while (--offset > 0)
    {
	if (p == NULL) return NULL;
	p= p->prev;
    }
    return &(p->t);
}
#endif /* UNUSED */


/* FREE_CTOKEN - Deallocate the memory associated with a CToken.  
 */

void free_ctoken(CToken *p)
{
    CToken *c, *n;

    /* Procedures */
    if (type(p->t) == TK_PROCEDURE)
    {
	for (c= (CToken *)p->t.val; c != NULL; c= n)
	{
	    n= c->next;
	    free_ctoken(c);
	}
    }
    else if (class(p->t) == TKC_STRING)
    {
	free(sval(p->t));
    }

    free(p);
}


/* DROP_PROGRAM - Discard the entire program compiled to date.  Will need to
 * do another init_comp() call before you can start again.
 */

void drop_program(void)
{
    CToken *c, *n;

    for (c= head.t.val; c != NULL; c= n)
    {
	n= c->next;
	free_ctoken(c);
    }
}


/* DROP_TOKEN - Discard the last n tokens in the current procedure.  If free
 * is true, deallocate the dropped tokens, otherwise, don't.  Bombs if there
 * aren't n tokens in the current procedure.  Returns the file index and
 * line number of the first token dropped.
 */

void drop_token(int n, int free, byte *file, word *line)
{
    CToken *old;

    if (file != NULL) *file= curr->t.sfid;
    if (line != NULL) *line= curr->t.line;

    while (n-- > 0)
    {
	curr= (old= curr)->prev;
	if (curr == NULL)
	{
	    /* Deleting first token of procedure */
	    if (n > 0) die("bug: dropped too many tokens");
	    curr= old->parent;
	    curr->t.val= (void *)NULL;
	    startproc= 1;
	}
	else
	{
	    curr->next= NULL;
	}
	if (free) free_ctoken(old);
    }

    return;
}


/* ADD_PROC -- insert the TK_PROCEDURE ctoken pointed to by p into the
 * current procedure as a chunk of in-line code.  If free is true, the
 * original ctoken is deallocated.
 */

void add_proc(CToken *p, int free_it)
{
    CToken *parent;

    /* Empty procedures are legal, but not exciting */
    if (p->t.val == NULL)
    {
	if (free_it) free(p);
	return;
    }

    /* Append the procedure to the current procedure */
    if (startproc)
    {
	parent= curr;
	curr= (CToken *)(curr->t.val= p->t.val);
	startproc= 0;
    }
    else
    {
	parent= curr->parent;
	((CToken *)p->t.val)->prev= curr;
	curr= curr->next= (CToken *)p->t.val;
    }

    /* Walk through the old procedure, fixing parent pointers */
    for (;;)
    {
	curr->parent= parent;
	if (curr->next == NULL) break;
	curr= curr->next;
    }

    /* Deallocate old ctoken (but not the procedure! - don't free_token) */
    if (free_it) free(p);
}


/* TEMPLATE - First argument is a length n string of type codes, which must
 * match previous elements of program (in reverse order):
 *   i   matches TK_INTEGER only
 *   s   matches TK_STRING only
 *   b   matches TK_INTEGER or TK_STRING
 *   l   matches TK_UNBOUND_LITERAL or TK_BOUND_LITERAL
 *   p   matches TK_PROCEDURE ONLY
 *   [   matches TK_MARK with value of 0.
 *   `   matches TK_MARK with value of 1.
 *   c   matches any constant token
 *   a   matches anything
 *   -   matches anything (don't return value).
 * if match is unsuccessful, return true.  If match is successsful, return
 * false and assign next n (CToken **) argument to point to tokens in
 * same order as string, omitting ones matched to -, ` or [.
 */

#if __STDC__
int template(char *tmpl, ...)
#else
int template(tmpl, va_alist)
    char *tmpl;
    va_dcl
#endif
{
    va_list ap;
    CToken *p= curr;
    int rc= 0;

    VA_START(ap,tmpl);

    while (*tmpl != '\0')
    {
	if (p == NULL)
	    {rc= 1; goto done;}

	switch (*tmpl)
	{
	case '-':
	case 'a':
	    break;
	case 'c':
	    if (!(p->t.flag & TKT_CONSTANT))
		{rc= 1; goto done;}
	    break;
	case 'i':
	    if (type(p->t) != TK_INTEGER)
		{rc= 1; goto done;}
	    break;
	case 's':
	    if (type(p->t) != TK_STRING)
		{rc= 1; goto done;}
	    break;
	case 'b':
	    if (type(p->t) != TK_INTEGER && type(p->t) != TK_STRING)
		{rc= 1; goto done;}
	    break;
	case 'l':
	    if (type(p->t) != TK_UNBOUND_LITERAL &&
		    type(p->t) != TK_BOUND_LITERAL)
		{rc= 1; goto done;}
	    break;
	case 'p':
	    if (type(p->t) != TK_PROCEDURE)
		{rc= 1; goto done;}
	    break;
	/*
	case 'k':
	    if (!(p->t.flag & TKT_CONSTANT) &&
		     type(p->t) != TK_PROCEDURE)
		{rc= 1; goto done;}
	    break;
	*/
	case '[':
	    if (type(p->t) != TK_MARK || ival(p->t) != 0)
		{rc= 1; goto done;}
	    break;
	case '`':
	    if (type(p->t) != TK_MARK || ival(p->t) != 1)
		{rc= 1; goto done;}
	    break;
	default:
	    die("bug: weird character '%c' in template",*tmpl);
	}

	/* save token pointer */
	if (*tmpl != '-' && *tmpl != '`' && *tmpl != '[')
	    *va_arg(ap, CToken **)= p;

	/* advance to next */
	p= p->prev;
	tmpl++;
    }
done:	va_end(ap);
    return rc;
}


/* DUMP_PROG - Print out the current program data structure.  Debugging use
 * only.
 */

int dpl_sfid;

void dpl(int level, CToken *p)
{
    int i;
    char *c;

    for ( ; p != NULL; p= p->next)
    {
	if (p->t.sfid != dpl_sfid)
	    printf("%s:\n",find_file_name(dpl_sfid= p->t.sfid));

	printf("%3d: ",p->t.line);

	for (i= 0; i < level; i++)
	    fputs("  ",stdout);

	switch (type(p->t))
	{
	case TK_PROCEDURE:
	    fputs("{\n",stdout);
	    dpl(level+1,(CToken *)p->t.val);
	    for (i= 0; i < level; i++)
		fputs("  ",stdout);
	    fputs("     }\n",stdout);
	    break;
	case TK_INTEGER:
	    printf("%ld\n",ival(p->t));
	    break;
	case TK_BOUND_SYMBOL:
	    printf("%-32s  <bound>\n",sysdict[ival(p->t)].key);
	    break;
	case TK_BOUND_LITERAL:
	    printf("/%-32s <bound>\n",sysdict[ival(p->t)].key);
	    break;
	case TK_MARK:
	    printf("mark <%d>\n",ival(p->t));
	    break;
	case TK_STRING:
	    putchar('(');
	    for (c= sval(p->t); *c != '\0'; c++)
	    {
		if (*c == '\n')
		    fputs("\\n",stdout);
		else
		    putchar(*c);
	    }
	    fputs(")\n",stdout);
	    break;
	case TK_UNBOUND_SYMBOL:
	    printf("%-32s  <unbound>\n",sval(p->t));
	    break;
	case TK_UNBOUND_LITERAL:
	    printf("/%-32s <unbound>\n",sval(p->t));
	    break;
	case TK_ARRAY:
	    printf("[...]\n");
	    break;
	case TK_FUNCTION:
	    printf("<func>\n");
	    break;
	case TK_DYNAMIC:
	    printf("<dyna:%s>\n",sval(p->t));
	    break;
	}
    }
}

void dump_prog()
{
    dpl_sfid= -1;
    dpl(0,&head);
}
