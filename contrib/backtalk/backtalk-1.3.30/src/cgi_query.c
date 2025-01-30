/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* NOTE: The portion of this file below the line is derived from NCSA source,
 * and is not covered by the Backtalk copyright or license.
 */

#include "backtalk.h"
#include "str.h"
#include "vstr.h"
#include "cgi_query.h"
#include "dict.h"
#include "debug.h"
#include "baaf.h"

#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char x2c(char *what);
void unescape_url(char *url);
void plustospace(char *str);
char *get_multipart_query(int cl, char *ct, FILE *repfp);

/* LOW-LEVEL CGI-INTERFACE
 *
 * This module contains code to decode cgi data for either GET or POST
 * methods.  It started as a combination of code Jan Wolter derived from
 * the stuff in the ncsa-defaults.tar.Z distribution for web-newuser and
 * Webspan 0.2 code by Steve Weiss.  It has been extended to handle multipart
 * posts in a slightly strange way.
 */


/* GET_QUERY - fetch the raw query string and return it.  It is taken either
 * from the QUERY_STRING or from stdin, depending on weather the
 * REQUEST_METHOD is POST or GET.  If it was POST the returned query string is
 * in malloc'ed memory, if it was GET it isn't.  Rather a defect, if you ask me.
 *
 * If the saverep flag is true, then we save  a copy of the query and the
 * environment to the environment.
 */

char *get_query(int saverep)
{
    char *query;
    char *method= getenv("REQUEST_METHOD");
    FILE *repfp= NULL;

    if (saverep)
    {
	if ((repfp= fopen(REPEAT_FILE,"w")) == NULL)
	{
	    fprintf(stderr,"Could not write repeat file "REPEAT_FILE"\n");
	    return NULL;
	}
	dump_env(repfp, FALSE);
    }

    if (method != NULL && !strcmp(method,"POST"))
    {
	char *cl= getenv("CONTENT_LENGTH");
	int len= (cl == NULL) ? 0 : atol(cl);
	char *ct= getenv("CONTENT_TYPE");
	if (cl > 0 && ct != NULL && !strncasecmp(ct,"multipart/form-data",19))
	{
	    /* A Multipart POST Query */
	    query= get_multipart_query(len,ct,repfp);
	    if (repfp != NULL) fclose(repfp);
	    return query;
	}
	else
	{
	    /* A Standard POST Query */
	    query= (char *)malloc(len+1);
	    len= fread(query,1,len,stdin);
	    query[len]= '\0';
	}

	/* Save the query (POST only) */
	if (query != NULL && repfp != NULL)
	{
	    fputs(query,repfp);
	    fputc('\n',repfp);
	}
    }
    else
	query= getenv("QUERY_STRING");

    if (repfp != NULL) fclose(repfp);

    return query;
}


/* RTB_VSTR - This is a function to pass to read_to_boundary() that appends
 * each character to a VSTRING.  It does cgi quoting on the value as it goes.
 */

void rtb_vstr(int ch, void *vs, void *junk)
{
    const char *hex= "0123456789ABCDEF";
    if (ch == ' ')
	ch= '+';
    else if (!isascii(ch) || !isprint(ch) || strchr("+&=%",ch) != NULL)
    {
	*vs_inc((VSTRING *)vs)= '%';
	*vs_inc((VSTRING *)vs)= hex[(ch >> 4) & 0x0F];
	ch= hex[ch & 0x0F];
    }
    *vs_inc((VSTRING *)vs)= ch;
}


/* RTB_PUTC - This is a function to pass to read_to_boundary() that outputs
 * each character to a file desciptor.
 */

#if ATTACHMENTS
void rtb_putc(int ch, void *fp, void *isplain)
{
    if (!isascii(ch) || (iscntrl(ch) && !isspace(ch) && ch != '\b'))
	*((int *)isplain)= 0;
    putc(ch,(FILE *)fp);
}
#endif /* ATTACHMENTS */


/* READ_TO_BOUNDARY - This routine reads from standard input until it has
 * read the given boundary, reading no more than the given number of
 * characters.  If repfp is not NULL, all characters read a written to that.
 * If chfunc is not NULL, it points to a function that is called for each
 * character of data read, up to, but not including, the boundary.  It is
 * always passed the character read as the first argument, and arg as the
 * second.  We should have just read a newline, or be at the beginning
 * of the input stream when this is called.
 *
 * We don't buffer any of our input in memory, which is good for handling
 * large hunks of data (and why else would you be using multipart/form-data?)
 * but makes the code rather complex.  Either a CR-NL or a NL alone is
 * considered a legal line ending.
 *
 * Returns true if the boundary we found had an extra -- at the end.  This
 * Marks end of document.
 */

int read_to_boundary(char *boundary, int *cl,
    FILE *repfp, void (*chfunc)(int, void *, void *), void *arg1, void *arg2)
{
    int ch;		    /* Current character */
    int nl= FALSE;	    /* Have we read any /n yet? */
    int cr= 0;	    	    /* No. of immediately previous carriage returns */
    int crnl= FALSE;	    /* Was last /n was preceded by a /r? */
    int inbound= TRUE;	    /* Does line appear to be boundary so far? */
    int col= 0;		    /* Current column number in boundary */
    int dash= 0;	    /* Number of extra end-of-boundary dashes read */
    int i;

    while (*cl > 0 && (ch= getchar()) != EOF)
    {
	(*cl)--;

	/* Output to repeat file, if we are doing that */
	if (repfp) putc(ch,repfp);

	if (inbound)
	{
	    if (ch == '\r')
	    {
		/* Saw a CR while reading boundary - wait for nl */
		cr++;
		continue;
	    }
	    else if (ch == '\n' && cr < 2)
	    {
		/* Check if we found end of boundary */
		if (boundary[col] == '\0' && dash != 1)
		    return (dash == 2);
	    }
	    else if (cr == 0 && ch == '-' && boundary[col] == '\0' && dash < 2)
	    {
		dash++;
		continue;
	    }
	    else if (cr == 0 && ch == boundary[col++])
		/* Found another character that matches boundary */
		continue;

	    /* If we get here, then what started out looking like a boundary
	     * turned out not to be one after all, so it must all have been
	     * data after all.  Output it all.
	     */
	    if (chfunc)
	    {
		if (crnl) (*chfunc)('\r', arg1, arg2);
		crnl= FALSE;
		if (nl) (*chfunc)('\n', arg1, arg2);
		for (i= 0; i < col - 1; i++)
		    (*chfunc)(boundary[i], arg1, arg2);
		for (i= 0; i < dash; i++)
		    (*chfunc)('-', arg1, arg2);
		dash= 0;
	    }
	    if (ch != '\n')
	    {
		/* Output character that proved it not a boundary ... */
		if (chfunc)
		{
		    while (cr-- > 0) (*chfunc)('\r', arg1, arg2);
		    cr= 0;
		    (*chfunc)(ch, arg1, arg2);
		}
		inbound= FALSE;
	    }
	    else
	    {
		/* ... Unless it is a newline, in which case it might be part
		 * of a boundary starting on next line */
		col= 0;
		if (cr > 0) crnl= TRUE;
		while (cr-- > 1) (*chfunc)('\r', arg1, arg2);
		cr= 0;
		nl= TRUE;
	    }
	}
	else if (ch == '\r')
	    /* Found a CR while in a line that is not a boundary */
	    /* Just remember it */
	    cr++;
	else if (ch == '\n')
	{
	    /* Found a NL while in a line that is not a boundary */
	    /* Start a new line, which might be a boundary */
	    col= 0; inbound= TRUE;

	    /* Update bookkeeping about newlines */
	    if (cr) crnl= TRUE;
	    while (cr-- > 1) (*chfunc)('\r', arg1, arg2);
	    cr= 0;
	    nl= TRUE;
	}
	else if (chfunc)
	{
	    /* Read a random character while not in a boundary */
	    /* Just output it, along with any previous CRs */
	    while (cr-- > 0) (*chfunc)('\r', arg1, arg2);
	    cr= 0;
	    (*chfunc)(ch, arg1, arg2);
	}
    }
}


/* GET_CONTENT_TYPE - We have just read the colon of a content-type header.
 * Read the rest, and return the content type.
 */

char *get_content_type(int *cl, FILE *repfp)
{
    int state= 0;     /* 0=white space; 1= value; 2= subsequent stuff */
    int wasnl= FALSE; /* last char was newline? */
    int ch;
    VSTRING ctype;

    vs_new(&ctype, 50);

    while (*cl > 0 && (ch= getchar()) != EOF)
    {
	if (ch == ' ' || ch == '\t')
	    wasnl= FALSE;
	else if (wasnl)
	{
	    /* newline followed by non-space ends the header.  Push the
	     * non-space back into input stream, and return. */
	    ungetc(ch,stdin);
	    return vs_return(&ctype);
	}
	else if (ch == '\n')
	    wasnl= TRUE;
	else
	{
	    if (ch == '\r' || ch == ';' || ch == ',')
	    {
		if (state == 1) state++;
	    }
	    else
	    {
		if (state == 0) state++;
		if (state == 1) *vs_inc(&ctype)= ch;
	    }
	}

	/* Output to repeat file, if we are doing that */
	if (repfp) putc(ch,repfp);
	(*cl)--;
    }
}


/* GET_CONTENT_DISPOSITION - We have just read the colon of a
 * content-disposition header.  Read the rest, and return the 'name' and
 * 'filename' values.
 */

void get_content_disposition(int *cl, FILE *repfp, char **pname, char **pfile)
{
    int state= 0;       /* 0 = white space before 'form-data';
		         * 1 = 'form-data';
		         * 2 = white space after ;
		         * 3 = name of a parameter
		         * 4 = value of 'name' parameter
		         * 5 = value of 'file' parameter
		         * 6 = value of some other parameter */
    int wasnl= FALSE;   /* last char was newline? */
    int inquote= FALSE; /* Are we in " marks? */
    int ch;
    VSTRING par, name, file;
    VSTRING *cval;

    vs_new(&par, 50);
    vs_new(&name, 50);
    vs_new(&file, 50);

    while (*cl > 0 && (ch= getchar()) != EOF)
    {
	if (ch == '\n')
	{
	    if (wasnl)
	    {
		/* newline followed by non-space ends the header.  Push the
		 * non-space back into input stream, and return. */
		ungetc(ch,stdin);
		break;
	    }
	    if (state > 0) state= 2;
	    wasnl= TRUE;
	    inquote= FALSE;
	}
	else if (inquote)
	{
	    if (ch == '"')
	    {
		inquote= FALSE;
		state= 2;
	    }
	    else
		*vs_inc(cval)= ch;
        }
	else if (ch == ' ' || ch == '\t')
	    wasnl= FALSE;
	else
	{
	    if (wasnl)
	    {
		/* newline followed by non-space ends the header.  Push the
		 * non-space back into input stream, and return. */
		ungetc(ch,stdin);
		break;
	    }

	    if (ch == '\r' || ch == ';' || ch == ',')
	    {
		if (state > 0) state= 2;
	    }
	    else if (ch == '=' && state == 3)
	    {
		*vs_inc(&par)= '\0';
		if (!strcasecmp(par.begin, "name"))
		{
		    state= 4;
		    cval= &name;
		}
		else if (!strcasecmp(par.begin, "filename"))
		{
		    state= 5;
		    cval= &file;
		}
		else
		{
		    state= 6;
		}
	    }
	    else if (ch == '"' && state > 3)
	    {
		inquote= TRUE;
	    }
	    else
	    {
		switch (state)
		{
		case 0:
		    state= 1; break;
		case 1: case 6:
		    break;
		case 2:
		    state= 3;
		    vs_empty(&par);
		    cval= &par;
		    break;
		}
		if (state >= 3 && state <= 5)
		    *vs_inc(cval)= ch;
	    }
	}

	/* Output to repeat file, if we are doing that */
	if (repfp) putc(ch,repfp);
	(*cl)--;

    }

    /* Clean up and return our data */
    vs_destroy(&par);
    *pname= vs_return(&name);
    *pfile= vs_return(&file);
}


/* GET_MULTIPART_QUERY - Handle a multipart query.  For the most part, we
 * convert it into a standard query string and return that.  However, if a
 * field name starts with 'baa_' and we have an attachment archive, then we
 * save the data to the archive, setting the variable to the unique ID of the
 * attachment instead.  CONTENT_LENGTH and CONTENT_TYPE are passed in.  If
 * repfp is non-null, log a copy of the query to that open file.
 *
 * This whole hunk of code adequately demonstrates what is wrong with ad-hoc
 * parsing.  I should probably be using yacc or something.
 *
 * Handling the query by converting it into a standard query and then
 * parsing that query apart to set our variables is rather roundabout and
 * inefficient, but this is really rarely used and we want to make it behave
 * as much like a normal query as possible.
 */

char *get_multipart_query(int cl, char *ct, FILE *repfp)
{
    char *p, *e, *boundary, *qfname;
    char *ftype, *fname, *ffile;
    int col, ch, inbound, rc;
    VSTRING head, query;

    /* Find the boundary */
    if ((p= strstr(ct+19, "boundary=")) == NULL)
	return NULL;
    p+= 9;
    if (*p == '"' || *p == '\'' )
    {
	/* Boundary is in quotes.  Find closing quote */
	if ((e= strchr(p+1,*p)) == NULL)
	    return NULL;
	*p++;
    }
    else
	/* Boundary is not in quotes.  Goes up to ; , or end of line */
	e= firstin(p,";,");
    /* Copy into malloced memory, adding preliminary -- */
    boundary= (char *)malloc(e-p+3);
    boundary[0]= '-';
    boundary[1]= '-';
    strncpy(boundary+2,p,e-p);
    boundary[e-p+2]= '\0';

    /* Find first Boundary, discarding anything we skip over to find it */
    if (read_to_boundary(boundary, &cl, repfp, NULL, NULL, NULL))
	return NULL;	/* No data */

    vs_new(&head, 100);
    vs_new(&query, BFSZ);

    for (;;)
    {
	/* Read some headers */
	fname= ffile= ftype= NULL;
	for (;;)
	{
	    /* Read header name */
	    vs_empty(&head);
	    while (cl > 0 && (ch= getchar()) != EOF)
	    {
		cl--;

		/* Output to repeat file, if we are doing that */
		if (repfp) putc(ch,repfp);

		/* Just ignore carriage returns */
		if (ch == '\r') continue;

		if (ch == '\n')
		{
		    /* Newline at start of line means end of headers */
		    if (head.p == head.begin)
			goto data;
		    /* Other lines without colons are bad data */
		    *vs_inc(&head)= '\0';
		    die("Bad multipart/form-data header: %s", head.begin);
		}

		/* Colon marks end of header name */
		if (ch == ':') break;

		/* Anything else is part of header name */
		*vs_inc(&head)= ch;
	    }
	    *vs_inc(&head)= '\0';

	    /* Read header values */
	    if (!strcasecmp(head.begin,"Content-Type"))
		ftype= get_content_type(&cl, repfp);
	    else if (!strcasecmp(head.begin,"Content-Disposition"))
		get_content_disposition(&cl, repfp, &fname, &ffile);
	    else
		die("Unknown multipart/form-data header: %s",head.begin);
	}
data:;

        /* Append '&fieldname=' to query */
        if (query.begin != query.p) *vs_inc(&query)= '&';
	qfname= cgiquote(fname);
	vs_cat(&query,qfname);
	free(qfname);
	*vs_inc(&query)= '=';

	/* Get field value and append to query */
#if ATTACHMENTS
	if (!strncmp(fname,"baa_",4) && ffile != NULL && ffile[0] != '\0')
	{
	    /* Open temp attachment file */
	    char *tmpfile, *handle, *qhandle;
	    int isplain= 1;
	    FILE *fp= open_temp_attach(&tmpfile);

	    /* Save data to file */
	    rc= read_to_boundary(boundary, &cl, repfp, rtb_putc, (void *)fp,
		    (void *)&isplain);
	    fclose(fp);

	    /* Fix content types */
	    if (isplain && ftype != NULL &&
		    !strcmp(ftype,"application/octet-stream"))
	    {
		free(ftype);
		ftype= strdup("text/plain");
	    }

	    /* Set variable to temp attachment value */
	    handle= temp_attach_handle(tmpfile, ffile, ftype);
	    free(tmpfile);
	    qhandle= cgiquote(handle);
	    free(handle);
	    vs_cat(&query,qhandle);
	    free(qhandle);
	}
	else
#endif
	{
	    /* Save data through next boundary */
	    rc= read_to_boundary(boundary, &cl, repfp, rtb_vstr,
		(void *)&query, NULL);
	}

	if (ffile != NULL) free(ffile);
	if (fname != NULL) free(fname);
	if (ftype != NULL) free(ftype);

	if (rc) break;
    }
    vs_destroy(&head);
    free(boundary);

    return vs_return(&query);
}


/* LOAD_QUERY - given a raw query string, decode it and store each pair in
 * the symbol table.  No longer mangles the query.
 */

void load_query(char *query)
{
    char *p, *end, *label;

    if  (query == NULL || query[0] == '\0')
	return;
    
    p= query;
    for (;;)
    {
	label= p;

	if ((end= strchr(p,'&')) != NULL) *end= '\0';

	plustospace(p);
	unescape_url(p);

	if ((p= strchr(p,'=')) == NULL)
	    set_sys_dict(label, "", SM_KEEP);
	else
	{
	    *p= '\0';
	    set_sys_dict(label, p+1, SM_COPY);
	    *p= '=';
	}
	if (end == NULL) break;
	*end= '&';
	p= end + 1;
    }

    return;
}


/* ------- The rest of this is recycled as is from the NCSA stuff ------- */

char x2c(char *what) {
    register char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
    return digit;
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
