/* (c) 1996-2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <ctype.h>

#include "format.h"
#include "dict.h"
#include "sysdict.h"
#include "stack.h"
#include "exec.h"
#include "str.h"
#include "vstr.h"
#include "tags.h"
#include "tagdefs.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* text formatting operators */

static char *pTable= NULL;

/* Character types */
#define CT_CTL 1
#define CT_BL 2
#define CT_NL 3
#define CT_LT 4
#define CT_GT 5
#define CT_AMP 6
#define CT_COLON 7
#define CT_CR 8
#define CT_TAB 9
#define CT_QUOTE 10
#define CT_APOSTROPHE 11

static void InitTable()
{
    pTable= (char *)malloc(256);
    if (!pTable) die("out of memory (256)\n");
    memset(pTable,CT_CTL,32);		/* control characters */
    memset(pTable+32,0,256-32);		/* ordinary characters */
    pTable[' ']=  CT_BL;
    pTable['\n']= CT_NL;
    pTable['<']=  CT_LT;
    pTable['>']=  CT_GT;
    pTable['&']=  CT_AMP;
    pTable[':']=  CT_COLON;
    pTable['\r']=  CT_CR;
    pTable[9]=  CT_TAB;
    pTable['"']=  CT_QUOTE;
    pTable['\'']=  CT_APOSTROPHE;
}

/* table of matchable URL protocols */

static char * protocols[] = {
    /* Standard URL protocols */
    "http",
    "https",
    "ftp",
    "nntp",
    "telnet",
    "gopher",
    /* gopher should be last protocol that needs // after the : */
    "mailto",
    "news",
    /* conf should be first Backtalk URL protocol */
    "conf",
    "item",
    "resp",
    NULL};

#define PCOL_HTTP 0
#define PCOL_FTP 2
#define PCOL_GOPHER 5
#define PCOL MAILTO 6
#define PCOL_NEWS 7
#define PCOL_CONF 8
#define PCOL_ITEM 9
#define PCOL_RESP 10

/* prot_expand - clickify one of the above protocols.  ppin is a pointer to
 * a pointer into the input text.  It should point at the first character after
 * a ':' when called.  When returned it will point to the first character after
 * the reference.  pout is a pointer to the output string structure.   If 'wrap'
 * is non-zero, do word wrapping at that column.  Pass in a pointer to the
 * current column index and wrap delta as pcol and pwdelta if you are wrapping.
 * If 'newline' is not NULL, it points to something to output instead of
 * newlines.  If nolink is true, it expands out conf:xxx things, but doesn't
 * enclose them in <A> tags.
 */

void prot_expand(char **ppin, VSTRING *pout,
	int wrap, int *pcol, int *pwdelta,
	char *newline, int nolink)
{
    char **pp= protocols;
    char *pc;
    char *pcin;
    int protocol_len;

    /* If we are not clickifying, ignore all standard protocols */
    if (nolink) pp+= PCOL_CONF;

    while (*pp)
    {
	pc= (*pp) + (protocol_len=strlen(*pp));
	/* prepare to scan protocol backwards from colon */
	pcin= *ppin-2;
	while (--pc >= *pp &&
	    ((isascii(*pcin) && isupper(*pcin)) ?  tolower(*pcin) : *pcin)
		== *pc)
	    --pcin;
	if (pc < *pp) break;	/* protocol matched */
	++pp;		/* protocol did not match, try another */
    }
    /* the first few protocols must be followed by "//" */
    /* all other protocols must be followed an alphanumeric */
    if (*pp && (((*ppin)[0]=='/' && (*ppin)[1]=='/') ||
		(pp-protocols > PCOL_GOPHER)) &&
	((isascii((*ppin)[0]) && isalnum((*ppin)[0])) ||
	 (pp-protocols <= PCOL_GOPHER)))
    {
	char *pint= *ppin;
        char  c1;
	Token *fmtok;

	if (pp-protocols >= PCOL_CONF) /* Backtalk-style protocol */
	{
	    char macro_name[16];
	    /* Backtalk protocols require a format macro to function */
	    /* first check to see if the format_xxxx macro exists */
	    strcpy (macro_name,"format_");
	    strcat (macro_name,*pp);
	    fmtok= get_dict(macro_name);
	    if (!fmtok || type(*fmtok) != TK_PROCEDURE) goto nonclick;
	}
	++pcin;		/* pointing to beginning of protocol name */

	/* if it is a "Backtalk" protocol, we convert it differently */
	/* Backtalk URLs are conf:, item:, and resp: */
	if (pp-protocols >= PCOL_CONF)
	{
	    /* temporary item and response numbers to pass to the filter */
	    int ni, nr;

	    char *pin1= *ppin;

	    pout->p-= strlen(*pp);

	    /* check to see if a conference name was entered */
	    if (isascii(**ppin) && isalpha(**ppin))
	    {
		++(*ppin);
		while (isascii(**ppin) && isalnum(**ppin))
		    ++(*ppin);
		pint= (char *)malloc(*ppin-pin1+1);
		memmove(pint, pin1, *ppin-pin1);
		pint[*ppin-pin1]= '\0';
		push_string(pint,FALSE);
		if (**ppin == ',' || **ppin == '.' || **ppin == ':')
		    ++(*ppin);	
	    }
	    else
	    {
		push_string(sdict(VAR_CONF), TRUE);
	    }

	    /* check if item number (ni), and response number (nr) are there */
	    nr= ni= 0;
	    while(isascii(**ppin) && isdigit(**ppin))
		ni*=10, ni+= **ppin-'0', ++(*ppin);
	    if ((**ppin == ',' || **ppin == '.' || **ppin == ':') &&
		    isascii((*ppin)[1]) && isdigit((*ppin)[1]))
	    {
		++(*ppin);
		while(isascii(**ppin) && isdigit(**ppin))
		    nr*=10, nr+= **ppin-'0', ++(*ppin);
	    }
	    else if (pp-protocols == PCOL_RESP) nr= ni, ni= 0;

	    if (ni == 0) ni= idict(VAR_ITEM);
	    push_integer(ni);
	    push_integer(nr);
	    uniquify_token(fmtok);
	    rerun(aval(*fmtok));	/* interpret the filter macro */
	    /* <conference> <item> <response> format_xxxx <<URL> */
	    pint= pop_string();
	    if (!nolink) vs_cat(pout,"<a href=\"");
	    vs_cat(pout,pint);
	    if (!nolink) vs_cat(pout,"\">");
	    free(pint);							
	}
	else /* an ordinary internet protocol, expand to clickable */
	{
	    /* find end of the URL */
	    while (isascii(**ppin) && isgraph(**ppin) &&
		    **ppin!='"'&& **ppin!='<' && **ppin!='>')
		++(*ppin);

	    while (c1= (*ppin)[-1], (isascii(c1) && ispunct(c1) && c1!='/'))
		--(*ppin);

	    if (*ppin < pint+3) {*ppin= pint; goto nonclick; }
	    pout->p-= strlen(*pp);
	    /* expand to clickable */
	    if (bdict(VAR_LINKTONEW))
	        vs_cat_n(pout,"<a target=\"_blank\" href=\"",25);
	    else
	        vs_cat_n(pout,"<a target=\"_top\" href=\"",23);
	    vs_cat_n(pout,pcin,*ppin-pcin);
	    *vs_inc(pout)= '"';
	    *vs_inc(pout)= '>';
	}

	if (!nolink)
	{
	    /* wrap overlong URLs */
	    while (wrap && *pcol+*pwdelta+(*ppin-pcin) > wrap)
	    {
		int ii= wrap-*pcol-*pwdelta;
		vs_cat_n(pout,pcin,ii);
		(*pcol)+= ii;
		pcin+= ii;
		if (newline == NULL)
		    *vs_inc(pout)= '\n';
		else
		{
		    int l= strlen(newline);
		    vs_cat_n(pout,newline,l);
		}
		(*pwdelta)= -(*pcol);
	    }
	    vs_cat_n(pout,pcin,*ppin-pcin);
	    vs_cat_n(pout,"</a>",4);
	    if (pcol != NULL) (*pcol)+= *ppin-pcin;
	}
    }
    else	/* unrecognized protocol, treat colon as ordinary */
    {
nonclick:
	*vs_inc(pout)= ':';
	if (pcol != NULL) (*pcol)++;
    }
}


/* FORMAT - does various cleanup on hunks of plain text.  if quote is TRUE, it
 * converts special HTML characters < > & to their quoted forms.  In addition
 * if expand is TRUE it will expand URLs to be clickable, and, if wrap is >0
 * it will wrap text at that many columns.
 *
 * Prefix and suffix, if not NULL, are strings put at the beginning and ending
 * of the response respectively.  Each newline character is replaced with the
 * newline string, if newline is not NULL.
 *
 * Note: always frees its "input" argument when it is done.
 *       caller is responsible for freeing the result string.
 */

char *format(char *input, int quote, int expand, int wrap,
	char *prefix, char *suffix, char *newline)
{
    VSTRING out;
    int inputlen;
    int col,		/* input column number */
	wdelta,		/* wrap delta - col+wdelta = output column number */
	ct,		/* character type */
	c,		/* character itself */
	indentCol,	/* what is the column of the first graphic */
	newlinelen,	/* length of newline argument */
	prevIndentCol,	/* previous line's indentCol */
	sawGraphic,	/* seen a graphic (non-blank) on this line yet? */
	wrapping;	/* 0=not wrapping, 1 or 2=wrapping (2=hunting for graphic at NL) */
    char *pin, *lastpin; 
    
    if (!pTable) InitTable();
    if (!vs_new(&out,2*(inputlen= strlen(input))))
	die("not enough memory to hold string of length 2*%d.\n",inputlen);
    pin= input;
    wrapping= sawGraphic= indentCol= wdelta= col= 0; lastpin= NULL;

    if (prefix != NULL)
    {
	int l= strlen(prefix);
	vs_cat_n(&out,prefix,l);
    }
    if (newline != NULL)
    {
	newlinelen= strlen(newline);
	/* White space may be significant, so delete trailing white space */
	while (isascii(input[inputlen-1]) && isspace(input[inputlen-1]))
	    input[--inputlen]= '\0';
    }
    
    while (*pin)
    {
	ct= pTable[c= (*pin++)&0xFF];
	if (wrapping == 2 && "xx  xxxx  "[ct]=='x')
	{
	    /* we have a graphic character while wrap logic is scanning past
	     * suppressed nl */
	    if (indentCol == prevIndentCol)
	    {
		/* continue wrapping */
		*vs_inc(&out)= ' '; /* emit blank to replace nl */
		wdelta++;
		wrapping= 1;
	    }
	    else
	    {
		/* stop wrapping because indentation changed */
		if (newline == NULL)
		    *vs_inc(&out)= '\n';
		else
		    vs_cat_n(&out,newline,newlinelen);
		for (wdelta= 0; wdelta < col; wdelta++) *vs_inc(&out)= ' ';
		wrapping= wdelta= 0;
	    }
	}
	if (ct)
	    switch (ct)
	    {
	    case CT_LT: 
		if (quote) vs_cat(&out,"&lt;");	/* escape the < */
		else *vs_inc(&out)= c;
		col++;
		sawGraphic= 1;
		break;
	    case CT_GT: 
		if (quote) vs_cat(&out,"&gt;");	/* escape the > */
		else *vs_inc(&out)= c;
		col++;
		sawGraphic= 1;
		break;
	    case CT_AMP: 
		if (quote) vs_cat(&out,"&amp;");	/* escape the & */
		else *vs_inc(&out)= c;
		col++;
		sawGraphic= 1;
		break;
	    case CT_QUOTE: 
		if (quote) vs_cat(&out,"&quot;");	/* escape the " */
		else *vs_inc(&out)= c;
		col++;
		sawGraphic= 1;
		break;
	    case CT_APOSTROPHE: 
		/* Can't use &apos; here because IE doesn't understand it */
		if (quote) vs_cat(&out,"&#39;");	/* escape the ' */
		else *vs_inc(&out)= c;
		col++;
		sawGraphic= 1;
		break;
	    case CT_CR:			/* expand the \r char --> nothing*/
		break;
	    case CT_TAB:			/* expand the \i char */
		if (wrapping == 2) do {++col; --wdelta;} while (col%8);
		else do {*vs_inc(&out)= ' '; ++col;} while (col%8);
		if (!sawGraphic) indentCol= col;
		break;
	    case CT_CTL:			/* expand the ctl char */
		*vs_inc(&out)= '^';
		*vs_inc(&out)= (c & 0x1f) + 'A'-1;
		col+= 2;
		sawGraphic= 1;
		break;
	    case CT_NL:				/* rewind the column count */
		prevIndentCol= indentCol;
		lastpin= pin;
		out.last= out.p;
		if (wrapping)
		{
		    if (wrapping == 2)
		    {
			/* stop wrapping at paragraph boundary */
			if (newline == NULL)
			    *vs_inc(&out)= '\n';
			else
			    vs_cat_n(&out,newline,newlinelen);
		       	wdelta= wrapping= 0;
		    }
		    else
			wrapping= 2; /* suppress NL */
		}
		if (wrapping)
		    wdelta+= col; /* don't output NL */
		else if (newline == NULL)
		    *vs_inc(&out)= '\n';
		else
		    vs_cat_n(&out,newline,newlinelen);
		sawGraphic= indentCol= col= 0;
		break;
	    case CT_BL:		/* remember this spot as possible wrap point */
		lastpin= pin;
		out.last= out.p;
		if (wrapping == 2)
		    --wdelta;
		else
		    *vs_inc(&out)= c;
		col++;
		if (!sawGraphic) indentCol= col;
		break;
	    case CT_COLON:		/* look for URL to make clickable */
		sawGraphic= 1;
		if (expand)
		    prot_expand(&pin, &out, wrap, &col, &wdelta, newline,
			FALSE);
		else		/* not expanding, treat colon as ordinary */
		{
		    *vs_inc(&out)= c;
		    col++;
		};
	    }
	else /* ordinary character case */
	{
	    *vs_inc(&out) = c;
	    col++;
	    sawGraphic= 1;
	}
	
	/* now if it is beyond the limit, go back to the last wrap point
	 * and insert a nl */
	if (wrap && wdelta+col>wrap)
	{
	    if (lastpin)
	    {
		pin= lastpin;
		out.p= out.last;
	    }
	    if (newline == NULL)
		*vs_inc(&out)= '\n';
	    else
		vs_cat_n(&out,newline,newlinelen);
	    for (wdelta= 0; wdelta < indentCol && wdelta < wrap/2; wdelta++)
		*vs_inc(&out)= ' ';
	    wdelta= indentCol-col;
	    wrapping= 1;
	    lastpin= out.last= NULL;
	}
    }
    if (suffix)
    {
	int l= strlen(suffix);
	vs_cat_n(&out,suffix,l);
    }
    free(input);
    return vs_return(&out);
}


static int DepthTable[N_TAGS];	/* How many levels of each tag we are in */
static int NoBrkLnDepth;	/* Levels of !brkln tags we are in */
static int IllegalDepth;	/* Levels of !legal tags we are in */

/* lookup_tag
 *
 * call with a address of pointer to a string to be interpreted as a TAG,
 * which may be delimited by > or whitespace. The pointer is
 * incremented to point to the delimiter.
 *
 * It returns the index. -1 if not found.
 * It is used during initialization, and also by match_tag
 */

int lookup_tag(char **pp)
{
    int min= 0;
    int max= N_TAGS-1;
    int mid;
    char *p;

    /* binary search the tag table to see if we have a match */
    while (max >= min)
    {
	char *t;

	p= *pp;
	mid= (max + min)/2;
	t= tagtype[mid].name;
	/* match the tag char by char */
	while (*p != '\0' && (*p == *t || *p == tolower(*t)))
	    ++p, ++t;
	if (!*t && (*p == '>' || (isascii(*p) && isspace(*p)))) break;
	if ((isascii(*p) && islower(*p) ? toupper(*p) : *p) < *t)
	    max= mid-1;
	else
	    min=mid+1;
    }

    /* we either found the tag in our table or we didn't */
    if (max < min) mid= -1;	/* we didn't */
    *pp= p;
    return mid;
}


/* match_tag
 *
 * search tag table to see if the tag is legit. Count it (up or down),
 * and scan to the end-of-tag marker, honoring quotes.
 *
 * it returns TRUE if a recognized tag is found and properly terminated.
 * if pTag!=NULL, it returns the number of the tag found in *pTag 
 *
 * if no tag is found (eg, no closing >) it returns FALSE and pTag is set
 * to -2.
 *
 * if a tag is found, but it is not a recognized tag, it returns false and
 * pTag is set to -1.
 *
 * *pp is advanced past the end of the tag, even if it was not recognized.
 *
 * Comments are treated as unrecognized tags, but are parsed so that *pp can
 * be advanced to the proper place.  */

int match_tag(char **pp, int *pTag)
{
    char *p= *pp;

    if (*p == '!')	/* html or sgml comment */
    {
	int sgml= p[1]=='-' && p[2]=='-';
	char *firstbrac= NULL;
	do
	{
	    while (*p && *p != '>') ++p;
	    if (*p) ++p; else break;
	    if (!firstbrac) firstbrac= p;
	} 
	while (sgml && (p[-2] != '-' || p[-3] != '-'));

	/* If we never found an "-->" to match the "<!--", stop at the
	 * first ">" we saw.  This is what mozilla does.
	 */
	if (sgml && (p[-2] != '-' || p[-3] != '-') && firstbrac)
	    p= firstbrac;

	*pp= p;
	if (pTag) *pTag= -1;
	return 0; /* treat all comments as failures to match */
    }
    else	/* a real tag - see if we recognize it */
    {
	int close= 0;
	int quote= 0;
	int tagnum;

	p= *pp;
	if (close= *p=='/') ++p;

	tagnum= lookup_tag(&p);

	/* now scan for the end of the tag */
	for (;;)
	{
	    if (*p == '\0')
	    {
		if (pTag) *pTag= -2;
		return 0;
	    }
	    else if (quote)
	    {
		if (*p == quote) quote= 0;
	    }
	    else if (*p == '\'' || *p == '"')
		quote= *p;
	    else if (*p == '>')
		break;
	    p++;
	}
	if (*p) ++p;	/* bump past the '>' */

	/* now update the counts, if it is a valid tag that needs to be
	 * counted */
	if (tagnum >= 0)
	{
	    /* Check various special cases for disallowed tags */
	    if (
		/* disallowed <IMG> */
		(tagnum == Tag_IMG && !bdict(VAR_ALLOWGRAPHICS)) ||
		/* disallowed <TR> outside TABLE, <TD> outside <TR>, etc */
		(tagtype[tagnum].inside >= 0 &&
		   !DepthTable[tagtype[tagnum].inside])
		)
	    {
		tagnum= -1;
	    }
	    else if (tagtype[tagnum].close)
	    {
		if (!close)
		{
		    /* opening tag - count it */
		    ++DepthTable[tagnum];
		    if (!tagtype[tagnum].brkln)
			++NoBrkLnDepth;
		    if (!tagtype[tagnum].legal)
		    {
			++IllegalDepth;
			tagnum= -1;
		    }
		}
		else if (DepthTable[tagnum] > 0)
		{
		    /* closing tag that was opened - decrement counts */
		    --DepthTable[tagnum];
		    if (!tagtype[tagnum].brkln)
			--NoBrkLnDepth;
		    if (!tagtype[tagnum].legal)
		    {
			--IllegalDepth;
			tagnum= -1;
		    }
		}
		else
		    /* closing a tag that never was opened - discard it */
		    tagnum= -1;
	    }
	    else if (close)
	    {
		/* discard non-required closing tag, like </p> and </hr> */
		/* dunno that this is a good idea */
		tagnum= -1;
	    }
	}

	*pp= p;

	if (pTag) *pTag= tagnum;
	return tagnum >= 0;
    }
}

/* These are attributes that can appear in legal tags whose values are
 * typically URLs, so we might want to do protocol expansion in them.
 * We expand these in any tags they appear in, the list of tags in which
 * they are used is just for reference.
 */

char *AttribURL[]= {
    "CITE",	    /* BLOCKQUOTE, DEL, INS, Q */
    "DYNSRC",	    /* IMG */
    "HREF",	    /* A, AREA */
    "LONGDESC",	    /* IMG */
    "LOWSRC",	    /* IMG */
    "SRC",	    /* IMG */
    "USEMAP",	    /* IMG */
    NULL};

/* scrub_tag:  remove banned attributes from legal tags and expand urls.
 *
 * This is passed pointers to the beginning, and end (the latter normally
 * having been found by match_tag()).  If the tag is OK, it returns NULL.
 * If the tag needs some fixing, it returns a point to a fixed copy in
 * malloced memory.
 *
 * TARGET attributes are deleted from all tags.  If addtarget is true,
 * a TARGET=_blank attributte is added.
 *
 * This routine makes many assumptions about the sanity of the tag - we know
 * it is terminated by a > and that it contains an even number of matching
 * quotes and so forth.  match_tag() should have been called first to
 * guarantee all this.
 */

char *scrub_tag(char *b, char *e, int expand, int addtarget)
{
    char *p, *q, *an, *av;
    int anl, avl, i, quoted;
    VSTRING out;
    out.begin= NULL;

    /* Non-tags, comments and closing tags are always OK */
    if (b[0] != '<' || b[1] == '/' || b[2] == '!') return NULL;

    /* Find start of first attribute */
    p= firstin(b," \n\t\r>");
    p= firstout(p," \n\t\r");

    while (*p != '>' && *p != '\0')
    {
	/* Find an= beginning of an attribute name, anl= length */
	an= p;
	p= firstin(p," \n\t\r=>");
	anl= p - an;
	/* find '=' sign, if there is one, or start of next attribute */
	p= firstout(p," \n\t\r");
	if (*p == '=')
	{
	    /* Find av= start of attribute value, avl= length */
	    av= firstout(p+1," \n\t\r");
	    quoted= (*av == '"' || *av == '\'');
	    if (quoted)
	    {
		p= strchr(av+1,*av) + 1;
		avl= p - av;
	    }
	    else
	    {
		p= firstin(av," \n\t\r>");
		avl= p - av;
	    }
	    /* Find start of next attribute */
	    p= firstout(p," \n\t\r");
	}
	else
	    av= NULL;


	/* Ban all attributes whose names start with "on"
	 * and ban "target" attribute */
	if (!strncasecmp(an,"ON",2) ||
	    (anl == 6 && !strncasecmp(an,"TARGET",6)))
	{
	    if (out.begin == NULL) vs_start(&out, e-b+50, b, an - b);
	    continue;
	}


	if (av != NULL)
	{
	    /* Ban Javascript entities - any value like "&{whatever}" */
	    if (av[quoted] == '&' && av[quoted+1] == '{' &&
		    av[avl - 1 - quoted] == '}')
	    {
		if (out.begin == NULL) vs_start(&out, e-b+50, b, an - b);
		continue;
	    }

	    /* Expand URLs in args */

	    /* Check for : in arg */
	    q= NULL;
	    for (i= 1; i < avl-1; i++)
		if (av[i] == ':') { q= av+i+1; break; }

	    if (q != NULL)
	    {
		for (i= 0; AttribURL[i] != NULL; i++)
		{
		    if (!strncasecmp(an,AttribURL[i],anl) &&
			AttribURL[i][anl] == '\0')
		    {
			/* suppress "javascript:" protocol URLs */
			if (q-11 >= av && !strncmp(q-11,"javascript",10))
			{
			    if (out.begin == NULL)
				vs_start(&out, e-b+50, b, an - b);
			    goto nextattrib;
			}
			if (!expand) break;

			if (out.begin == NULL)
			    vs_start(&out, e-b+50, b, an - b);

			/* expand other URLs */
			vs_cat_n(&out,an,q-1-an);
			prot_expand(&q, &out, FALSE, NULL, NULL, NULL, TRUE);
			vs_cat_n(&out,q,p-q);
			goto nextattrib;
		    }
		}
	    }
	}

	/* Attribute was OK - append it to out if we are into that */
	if (out.begin != NULL)
	    vs_cat_n(&out,an,p-an);
nextattrib:;
    }

    /* Add target tag, if requested */
    if (addtarget && *p == '>')
    {
	if (out.begin == NULL) vs_start(&out, e-b+50, b, p - b);
	if (bdict(VAR_LINKTONEW))
	    vs_cat_n(&out," target=\"_blank\"",16);
	else
	    vs_cat_n(&out," target=\"_top\"",14);
    }

    /* Terminate the tag, if we are copying it */
    if (out.begin != NULL)
    {
	if (*p == '>') *vs_inc(&out)= '>';
	*vs_inc(&out)= '\0';
    }

    return out.begin;
}


/* subroutine to close a tag - called below when needed */

void CloseTag (VSTRING* pOut, int tag)
{
    char *pTag= tagtype[tag].name;

    /* ensure closing TD before TR, TR before TABLE, etc */
    if (tag == Tag_TABLE || tag == Tag_TR || tag == Tag_MAP)
    {
	int i;
	for (i= 0; i < N_TAGS; i++)
	    if (tagtype[i].inside == tag)
		while (DepthTable[i]) CloseTag(pOut,i);
    }

    /* close tag */
    *vs_inc(pOut)= '<';
    *vs_inc(pOut)= '/';
    while (*pTag && *pTag!='/') *vs_inc(pOut)= *pTag++;
    *vs_inc(pOut)= '>';
    --DepthTable[tag];
}

/* format_html - a subroutine, not a stack operator. Call it with a string that 
 * should contain text/html. It will sanity-check the HTML and sanitize it,
 * returning the "clean, sane" HTML in a freshly allocated string.
 * Calls free on its input, so make a copy first if you want to use input again.
 * Caller must free the output.  If 'newline' is non-null, it is a string to
 * substitute for any newline not in a tag.  If inpre is false, newline
 * substitutions are not done in environments where newlines are significant,
 * like, <PRE>.  If 'expand' is true, we clickify URLs outside of tags.
 * Returns NULL if an internal error occurred.
 */

char *format_html(char *input, char *newline, int inpre, int expand)
{
    char *pin= input;
    int newlinelen;
    int tag;
    char c;
    VSTRING out;

    if (newline != NULL) newlinelen= strlen(newline);

    memset(DepthTable,0,sizeof(DepthTable));
    IllegalDepth= 0;
    NoBrkLnDepth= 0;

    if (!vs_new(&out,strlen(input)+256))
	 die("format_html: not enough memory for string of length %d + 256.\n",
	     strlen(input));

    while (*pin)
    {
	/* scan for tags, and check each one to see if it is permitted,
	 * and also count it */
	c= *pin++;
	if (c == '<')
	{
	    if (*pin=='/' || *pin=='!' || (isascii(*pin) && isalpha(*pin)))
	    {
		/* now we have a tag */
		char *pBeginTag= pin-1;
		int tag= -1;
		/* recognized tag */
		if (match_tag(&pin,&tag))
		{
		    char *fixedtag;
		    int addtarget= (tag == Tag_A);
		    if ((fixedtag= scrub_tag(pBeginTag,pin,expand,addtarget))
			    == NULL)
		    {
			size_t TagSize= pin-pBeginTag;
			vs_cat_n(&out,pBeginTag,TagSize);
		    }
		    else
		    {
			vs_cat(&out,fixedtag);
			free(fixedtag);
		    }
		}
		else if (tag == -2)
		{
		    /* Unclosed tag - escape the < */
		    pin= pBeginTag + 1;
		    vs_cat_n(&out,"&lt;",4);
		}
	    }
	    else
		/* '<' not a tag, but don't trust browser, escape it */
		vs_cat_n(&out,"&lt;",4);
	}

	else if (expand && c == ':' && !DepthTable[Tag_A])
	    /* Expand a protocol, if not between <A> and </A> */
	    prot_expand(&pin, &out, 0, NULL, NULL,
		    (inpre || NoBrkLnDepth == 0) ? newline : NULL, FALSE);

	else if (newline != NULL && c == '\n' &&
		(inpre || NoBrkLnDepth == 0) && IllegalDepth == 0)
	    /* Convert terminal newline, if not in a nobrkln tag */
	    vs_cat_n(&out,newline,newlinelen);

	else if (IllegalDepth == 0)
	    /* Copy text, if not in an !legal tag */
	    *vs_inc(&out)= c;
    }

    free (input);

    /* we are done with sanity-checks - now clean up any unclosed tags */
    for (tag= 0; tag < N_TAGS; tag++)
	while (tagtype[tag].legal && DepthTable[tag] > 0)
	    CloseTag(&out,tag);

    return vs_return(&out);
}

/* flush_token - a utility used by format_html2text, below */
static void flush_token(VSTRING *pToken, VSTRING *pOut,
	int LeftMargin, int RightMargin, int *pOutColumn, int *pNeedSep)
{
    char *p= pToken->begin;
    int count;

    while (count= pToken->p - p)
    {
	if (*pOutColumn > LeftMargin &&
		*pOutColumn + count + *pNeedSep > RightMargin)
	{
	    *vs_inc(pOut)= '\n';
	    *pOutColumn= *pNeedSep= 0;
	}

	if (*pOutColumn < LeftMargin)
	{
	    memset(vs_inc_n(pOut,LeftMargin - *pOutColumn),' ',LeftMargin - *pOutColumn);
	    *pOutColumn= LeftMargin;
	    *pNeedSep= 0;
	}

	/* emit separater if needed */
	if (*pNeedSep) *vs_inc(pOut)= ' ';
	++(*pOutColumn);

	/* emit the current token, but still wrap it at 80 if it is
	 * long enough to need it */
	if (80 - *pOutColumn < count) count= 80 - *pOutColumn;
	vs_cat_n(pOut,p,count);
	p+= count;
	(*pOutColumn)+= count;
	if (p >= pToken->p) break;	/* the token fit */

	*vs_inc(pOut)= '\n';	/* the token needed folding */
	*pOutColumn= 0;
    }
    pToken->p= pToken->begin;	/* empty the token VSTRING*/
    *pNeedSep= 1;
}

/* table of escape sequences for HTML characters */

static char* EscapeTable[]=
   {"LT;<",
    "GT;>",
    "AMP;&",
    "NBSP; ",
    "COPY;(C)",
    "QUOT;\"",
    NULL};


/* format_html2text - a subroutine, not a stack operator. Call it with a
 * string that should contain text/html. It will produce formatted text
 * from it.
 *
 * Thus this routine functions much like a browser. It is a micro-mini-browser,
 * with the ability to understand and process only a small handful of tags.
 *
 * Like the others, it frees its input when its done reading it, and produces
 * an output string which it is the caller's responsibility to free.
 */

char *format_html2text(char *input)
{
    char *pin= input;
    VSTRING out;
    VSTRING token;
    int outColumn= 0;
    int NeedSep= 0;
    int LeftMargin= 0;
    int RightMargin= 80;

    memset(DepthTable,0,sizeof(DepthTable));
    IllegalDepth= 0;
    NoBrkLnDepth= 0;

    if (!vs_new(&out,strlen(input)))
     die("format_html2text: not enough memory for string of length %d.\n",
	 strlen(input));
    if (!vs_new(&token,80))
	 die("format_html2text: not enough memory for string of length 80.\n");

    while (*pin)
    {
	/* ignore whitespace */
	while (*pin && isascii(*pin) && isspace(*pin)) ++pin;

	/* various meta-characters - most of which we remove, some of which
	 * we fix up */
	if ((unsigned char)*pin > 127)
	{
	    switch ((unsigned char)*pin)
	    {
	    case 145:	/* Smart single left quote */
		*vs_inc(&token)= '`';
		break;

	    case 146:	/* Smart single right quote */
		*vs_inc(&token)= '\'';
		break;

	    case 147:	/* Smart double left quote */
	    case 148:	/* Smart double right quote */
		*vs_inc(&token)= '"';
		break;

	    case 169:	/* copyright */
		*vs_inc(&token)= '(';
		*vs_inc(&token)= 'c';
		*vs_inc(&token)= ')';
		break;
	    }
	    pin++;
	}

	else if (*pin=='&') /* an escape character is probable */
	{
	    char ** esc;
	    char *pe;
	    char *pi;

	    /* See if we recognize the escape sequence */
	    for (esc= EscapeTable; *esc; esc++)
	    {
		pe= *esc;
		pi= pin+1;
		while (*pe != '\0' && *pe != ';' &&
			(*pe == *pi || tolower(*pe) == *pi))
		    ++pi, ++pe;
		if (*pi == ';' && *pe == ';')
		    /* we got an exact escape sequence */
		    break;
	    }
	    if (*esc)	/* we got one, emit its production */
	    {
		pin= pi+1;
		for (++pe; *pe; ++pe) *vs_inc(&token)= *pe;
	    }
	    else	/* failed to match - just send out the & and move on */
	    {
		pin++;
		*vs_inc(&token)= '&';
	    }
	}
	/* see if we have a tag here */
	else if (*pin=='<')	/* a tag is probable */
	{
	    if (pin[1]=='/' || pin[1]=='!' ||
		    (isascii(pin[1]) && isalpha(pin[1])))
	    {
		/* now we have a tag */
		int tag;
		char *pBeginTag= pin++;

		if(match_tag(&pin,&tag)) 
		{
		    /* recognized tag - some we handle specially right now */
		    if (tagtype[tag].brkln)
		    {
			/* These tags must finish any earlier lines first -
			 * first flush any buffered token */
			if (token.p > token.begin)
			     flush_token(&token, &out, LeftMargin, RightMargin,
				 &outColumn, &NeedSep);


			/* end the current line if there is stuff on it */
			if (outColumn)
			{
			    *vs_inc(&out)= '\n';
			    outColumn= NeedSep= 0;
			}

			if (tag==Tag_P) *vs_inc(&out)= '\n';
/*			else if (tag==Tag_HR) 
			{
			    if (LeftMargin) memset(vs_inc_n(&out,LeftMargin),' ',LeftMargin);
			    memset(vs_inc_n(&out,RightMargin-LeftMargin),'-',RightMargin-LeftMargin);
			    *vs_inc(&out)= '\n';
			}
*/			else if (tag==Tag_LI)
			{
			    if (LeftMargin>2)
				memset(vs_inc_n(&out,LeftMargin-2),' ',
					LeftMargin-2);
			    *vs_inc(&out)= "**o+-><="[
				(DepthTable[Tag_UL]+DepthTable[Tag_OL]+
				 DepthTable[Tag_MENU]+DepthTable[Tag_DIR])%8]; 
			    *vs_inc(&out)= ' ';
			    outColumn= LeftMargin;
			}
		    }
		    else if (tag == Tag_IMG)
		    {
			char *pi;
			int quote= 0;
			int alt= 0;

			if (token.p > token.begin)
			     flush_token(&token, &out, LeftMargin, RightMargin, &outColumn, &NeedSep);

			/* find the alt attribute if it is given */

			for (pi= pBeginTag; pi < pin; pi++)
			{
			    if (*pi == '"')
			    {
				if (alt && quote) break;
				quote= !quote;
			    }
			    else if (alt) *vs_inc(&token)= *pi;
			    else if (!quote && isascii(*pi) && isspace(*pi) &&
				     (pi[1] == 'A' || pi[1] == 'a') &&
				     (pi[2] == 'L' || pi[2] == 'l') &&
				     (pi[3] == 'T' || pi[3] == 't') &&
				      pi[4] == '=')
			    {
				alt= 1;
				pi+= 4;
			    }
			}
			/* emit default if no alt attribute provided */
			if (!alt) vs_cat(&token,"[IMAGE]");

			if (token.p > token.begin)
			     flush_token(&token, &out, LeftMargin,
				 RightMargin, &outColumn, &NeedSep);
		    }

		    /* recompute margins after processing a tag */
		    LeftMargin= DepthTable[Tag_BLOCKQUOTE]*7 +
			(DepthTable[Tag_UL]+DepthTable[Tag_OL])*4;
		    if (DepthTable[Tag_UL]+DepthTable[Tag_OL] > 0)
			LeftMargin+= 2;
		    RightMargin= 80 - DepthTable[Tag_BLOCKQUOTE]*7;
		    if (RightMargin < LeftMargin+15)
		    {
			RightMargin= LeftMargin+ 15;
			if (RightMargin > 80) RightMargin= 80;
		    }

		}
		else if (tag == -2)
		{
		    /* Unclosed tag - just append the < */
		    *vs_inc(&token)= '<';
		    pin= pBeginTag+1;
		}
		/* else unrecognized tag, just ignore it */
	    }
	    else
		*vs_inc(&token)= *pin++; /* '<' not a tag. append it to token */
	}

	/* otherwise just move a token into the token VSTRING */
	else
	    while (*pin != '\0' && *pin != '&' && *pin != '<' &&
		    isascii(*pin) && !isspace(*pin))
	    {
		char c= *pin++;
		*vs_inc(&token)=
		    (DepthTable[Tag_H1] && isascii(c) && islower(c)) ?
		    toupper(c): c;
	    }

	/* suppress text appearing between title directives */
	if (IllegalDepth > 0) token.p= token.begin;

	/* now see if we have completed building a token, or if it needs to be
	 * forced out. */
	else if (token.p > token.begin &&
		(*pin == '\0' || (isascii(*pin) && isspace(*pin))))
	     flush_token(&token, &out, LeftMargin, RightMargin,
		 &outColumn, &NeedSep);
    }

    vs_destroy(&token);
    return vs_return(&out);
}


#ifdef CLEANUP
void free_format()
{
    if (pTable != NULL) free(pTable);
    pTable= NULL;
}
#endif


#ifndef TESTMAIN
/* these entry points are suppressed if testing */

/* FUNC_QUOTE:  <ascii-string> quote <html-string>
 * replaces reserved HTML characters with quoted versions, so that the
 * resulting text will not contain any HTML tags
 */

void func_quote()  
{ 
    push_string(format(pop_string(), TRUE, FALSE, 0, NULL, NULL, NULL), FALSE); 
}

/* FUNC_EXPAND:  <ascii-string> expand <html-string>
 * same as quote, but makes URLs clickable and wraps text at 79 cols
 */

void func_expand() 
{ 
    push_string(format(pop_string(), TRUE, TRUE , 79, NULL, NULL, NULL), FALSE); 
}

/* FUNC_WRAP:  <string> <numcols> wrap <wrapped-string>
 * Does nothing but wrap to the specified number of columns
 */

void func_wrap()
{
    int cols= pop_integer();
    push_string(format(pop_string(), FALSE, FALSE, cols, 
		NULL, NULL, NULL), FALSE);
}

/* FUNC_CLEANHTML: <html-string> clean_html <html-string>
 * Sanity check and sanitize and HTML string.
 */

void func_cleanhtml()
{
    char *clean= format_html(pop_string(),NULL,TRUE,TRUE);
    if (clean == NULL) die("HTML cleanup failed");
    push_string(clean,FALSE);
}

/* FUNC_UNHTML: <html-string> clean_html <ascii-string>
 * Convert an HTML string to something plain.
 */

void func_unhtml()
{
    char *text= format_html2text(pop_string());
    if (text == NULL) die("HTML conversion failed");
    push_string(text,FALSE);
}


/* sysformat: called internally to quote a string for dumping into HTML. 
* see func_quote, above. strdups input, so caller is responsible for freeing.
* Will not call "die" or otherwise fail. Caller must still free output too. */

char *sysformat(char *input)
{
    char *s;
    s= format(strdup(input), TRUE, FALSE, 0, NULL, NULL, NULL);
    return s;
}
#endif

/* the rest of this program is only used for testing */
#ifdef TESTMAIN
main()
{
    char *inbuf, *outbuf, line[256];
    int inlen;
    
    for (;;)
    {
	inbuf=(char *)malloc(1);
	*inbuf= '\0';
	inlen= 0;
	
	for (;;)
	{
	    if (fgets(line,256,stdin))
	    {
		int n;
		char *newinbuf;
		
		if (!strcmp(line,".\n")) break;
		n= strlen(line);
		newinbuf= (char *)malloc(inlen+n+1);
		memmove(newinbuf,inbuf,inlen);
		memmove(newinbuf+inlen,line,n);
		newinbuf[inlen+n]= '\0';
		free(inbuf);
		inbuf= newinbuf;
		inlen+= n;
	    }
	    else break;
	}
	if (!*inbuf) break;
/*	outbuf= format(inbuf,TRUE,TRUE,40,NULL,NULL,NULL); /**/

	outbuf= format_html(inbuf,NULL,TRUE,TRUE); /**/
/*	outbuf= format_html2text(inbuf); /**/
	printf("\n\n\n===============\n\n\n");
	fputs(outbuf,stdout);
	free(outbuf);
    }
    return 0;
}


#if __STDC__
void die(const char *fmt,...)
#else
void die(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
va_list ap;

    VA_START(ap,fmt);
    fputs("DIED:  ",stdout);
    vprintf(fmt, ap);
    fputc('\n',stdout);
    va_end(ap);
    exit(1);
}

void push_exec(int flag, char *src, ...) {}
void push_integer(int n) {}
void push_string(char *s, int copy) {}
void uniquify_token(Token *t) {};

void rerun(Array *src) {}
HashEntry sysdict[1];
char *pop_string() {return strdup("[filter results]");}
Token *get_dict(char *label)
{
    static Token t;
    t.flag= TK_PROCEDURE;
    return(&t);
}


#endif
