/* Copyright 2003, Jan D. Wolter, All Rights Reserved. */

#include <ctype.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "common.h"
#include "email.h"
#include "version.h"
#include "date.h"
#include "vstr.h"
#include "str.h"
#include "sysconfig.h"

#if EMAIL

#define MIME_MESSAGE "This is a multi-part message in MIME format.\n"


/* NEW_EMAIL - Create an email message object.  All arguments may be NULL.
 * If fromaddr is NULL, the name and address will default to those set in
 * backtalk.conf.
 */

Email *new_email(char *fromaddr, char *fromname, char *subj, char *replyaddr,
	char *replyname)
{
    int i;

    Email *em= (Email *)malloc(sizeof(Email));
    em->subj= strdup(subj ? subj : "");
    em->from.addr= fromaddr ? strdup(fromaddr) : NULL;
    em->from.name= fromname ? strdup(fromname) : NULL;
    em->replyto.addr= replyaddr ? strdup(replyaddr) : NULL;
    em->replyto.name= replyname ? strdup(replyname) : NULL;
    for (i= 0; i < N_REC; i++)
	em->to[i]= NULL;
    em->cont= NULL;
    em->contl= NULL;
    em->encoded= FALSE;
    em->boundary= NULL;
    return em;
}


/* EMAIL_FREE_RECIPIENT - Free a list of recipients.
 */

void email_free_recipient(struct emaddr *addr)
{
    struct emaddr *naddr;

    for (; addr != NULL; addr= naddr)
    {
	naddr= addr->next;
	if (addr->name != NULL) free(addr->name);
	if (addr->addr != NULL) free(addr->addr);
	free(addr);
    }
}


/* EMAIL_RECIPIENT - Add a recipient to an email message.
 *
 * Type is one of REC_TO, REC_CC, or REC_BCC.  Name may be NULL, but addr is
 * required.
 *
 * If append is true, we add the recipient to the existing list.  If not
 * the new person replaces any previous recipients.
 */

void email_recipient(Email *email, int type, char *addr, char *name, int append)
{
    struct emaddr *ema= (struct emaddr *)malloc(sizeof(struct emaddr));
    ema->addr= strdup(addr);
    ema->name= name ? strdup(name) : NULL;
    ema->next= NULL;

    if (!append)
	email_free_recipient(email->to[type]);

    if (email->to[type] != NULL && append)
	email->tol[type]->next= ema;
    else
	email->to[type]= ema;
    email->tol[type]= ema;
}


/* EMAIL_BODY - Supply a body to an email message.  If contype is NULL, this
 * is added on to the end of the previous block of content.  Otherwise,
 * a new block of content is begun with the given content type.  If alt
 * is 1, then this is for the alternate version of the text.  If the length
 * is non-negative, then the text can contain internal nuls.  Otherwise the
 * text is assumed to be a nul-terminated string, and the length will
 * be computed.  If enctype is ENC_DUNNO, then code will choose an appropriate
 * encoding to use, otherwise, the encoding give will be used, appropriate
 * or not.
 */

void email_body(Email *email, int alt, char *contype, int enctype,
	char *text, int len, char *desc)
{
    int oldlen;
    struct emcont *emc;

    if (email->encoded)
	die("Cannot add content to a email message that has "
	    "already been encoded");

    if (len < 0) len= strlen(text);

    if (contype == NULL)
    {
	/* Append to existing content block */
	if (email->contl == NULL || email->contl->type[alt] == NULL)
	    die("appending to nonexistant email text block");
	emc= email->contl;
	oldlen= emc->tlen[alt];
	emc->text[alt]= (char *)realloc(emc->text[alt], oldlen+len+1);
	memmove(emc->text[alt]+oldlen, text, len);
	emc->tlen[alt]= oldlen + len;
	if (emc->etype[alt] == ENC_DUNNO)
	    emc->etype[alt]= enctype;
	if (emc->desc[alt] == NULL && desc != NULL)
	    emc->desc[alt]= strdup(desc);
	return;
    }

    if (email->cont == NULL || email->contl->type[alt] != NULL)
    {
	/* Create a new content node */
	emc= (struct emcont *)malloc(sizeof(struct emcont));
	emc->type[1-alt]= NULL;
	emc->text[1-alt]= NULL;
	emc->desc[1-alt]= NULL;
	emc->next= NULL;
	if (email->contl == NULL)
	    email->cont= emc;
	else
	    email->contl->next= emc;
	email->contl= emc;
    }
    else
	emc= email->contl;

    /* Save new content text */
    emc->type[alt]= strdup(contype);
    email->contl->etype[alt]= enctype;
    emc->text[alt]= (char *)malloc(len + 1);
    memmove(emc->text[alt], text, len);
    emc->tlen[alt]= len;
    emc->desc[alt]= (desc == NULL) ? NULL : strdup(desc);
}


/* EMAIL_FREE - Deallocate an email object.
 */

void email_free(Email *email)
{
    struct emaddr *addr, *naddr;
    struct emcont *cont, *ncont;
    int i;

    /* Free lists of recipients */
    for (i= 0; i < N_REC; i++)
	email_free_recipient(email->to[i]);

    /* Free lists of content */
    for (cont= email->cont; cont != NULL; cont= ncont)
    {
	ncont= cont->next;
	if (cont->type[0] != NULL) free(cont->type[0]);
	if (cont->type[1] != NULL) free(cont->type[1]);
	if (cont->text[0] != NULL) free(cont->text[0]);
	if (cont->text[1] != NULL) free(cont->text[1]);
	if (cont->desc[0] != NULL) free(cont->desc[0]);
	if (cont->desc[1] != NULL) free(cont->desc[1]);
	free(cont);
    }

    /* Free misc fields */
    if (email->subj != NULL) free(email->subj);
    if (email->from.name != NULL) free(email->from.name);
    if (email->from.addr != NULL) free(email->from.addr);
    if (email->replyto.name != NULL) free(email->replyto.name);
    if (email->replyto.addr != NULL) free(email->replyto.addr);
    if (email->boundary != NULL) free(email->boundary);

    /* Free the structure */
    free(email);
}

/* EMAIL_WRITE_ADDR - Write and email address/name to the given file
 * descriptor.
 */

void email_write_addr(struct emaddr *addr, FILE *fp)
{
    char *p, *del;
    int quote;

    /* Print user name, if defined - we always put this in quotes */
    if (addr->name != NULL)
    {
	putc('"',fp);
	for (p= addr->name; *p != '\0'; p++)
	{
	    if (strchr("\"\\",*p) != NULL) putc('\\', fp);
	    putc(*p,fp);
	}
	putc('"',fp);
	putc(' ',fp);
    }

    /* Print email address - we output one component at a time, components
     * being delimited by '@' and then '.' .  Each component is quoted if
     * it contains certain awful characters.
     */
    putc('<',fp);
    /* First component ends with the @ sign */
    del= firstin(p= addr->addr,"@");
    for (;;)
    {
	/* check for awful characters that might force quoting */
	quote= (firstin(p," ;:,<>()\t\"\\") < del);
	if (quote) putc('"',fp);
	/* Print the component */
	for (; p < del; p++)
	{
	    if (strchr("\"\\",*p) != NULL) putc('\\', fp);
	    putc(*p,fp);
	}
	if (quote) putc('"',fp);

	if (*p == '\0') break;

	/* Print delimiter */
	putc(*(p++),fp);
	
	/* Find end of next compenent */
	del= firstin(p,".");
    }
    putc('>',fp);
}


/* EMAIL_QP_ENCODE - Generate a Quoted-Printable encoding of a text.
 * Length of the text is passed in.  Text can contain nil characters,
 * but ordinarily wouldn't.  Returns a pointer to the encoding of the
 * original text, in dynamic memory.
 */

char *email_qp_encode(char *text, int len, int crnl)
{
    VSTRING out;
    int col= 0;
    char bf[4], *end= text + len;

    vs_new(&out,strlen(text)*2);

    for (; text < end; text++)
    {
	/* Printable characters, spaces not at end of line -
	 *  no encoding needed */
	if ((*text >= '!' && *text <= '<') ||
	    (*text >= '>' && *text <= '~') ||
	    ((*text == ' ' || *text == '\t') &&
	     (text+1 == end ||  text[1] != '\n') ) )
	{
	    if (++col >= 76)
	    {
		*vs_inc(&out)= '=';
		if (crnl) *vs_inc(&out)= '\r';
		*vs_inc(&out)= '\n';
		col= 1;
	    }
	    *vs_inc(&out)= *text;
	}
	/* Hard line breaks - become CRLF */
	else if (*text == '\n')
	{
	    if (crnl) *vs_inc(&out)= '\r';
	    *vs_inc(&out)= '\n';
	    col= 0;
	}
	/* Encode other characters */
	else
	{
	    if ((col+= 3) >= 76)
	    {
		*vs_inc(&out)= '=';
		if (crnl) *vs_inc(&out)= '\r';
		*vs_inc(&out)= '\n';
		col= 3;
	    }
	    sprintf(bf,"=%02X",*text);
	    vs_cat_n(&out,bf,3);
	}
    }
    return vs_return(&out);
}


/* EMAIL_B64_ENCODE - Generate a base64 encoding of a text.
 * Returns a pointer to the encoding of the original text, in dynamic
 * memory.
 */

char *email_b64_encode(char *text, int len, int crnl)
{
    VSTRING out;
    unsigned char *p, *e;
    int n;
    static const char b64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    vs_new(&out,len*2);

    n= 0;
    for (p= (unsigned char *)text, e= p+len; p < e - 2; p+= 3)
    {
	*vs_inc(&out)= b64[p[0] >> 2];
	*vs_inc(&out)= b64[((p[0] & 0x03) << 4) | (p[1] >> 4)];
	*vs_inc(&out)= b64[((p[1] & 0x0f) << 2) | (p[2] >> 6)];
	*vs_inc(&out)= b64[p[2] & 0x3f];
	if (++n == 19)
	{
	    if (crnl) *vs_inc(&out)= '\r';
	    *vs_inc(&out)= '\n';
	    n= 0;
	}
    }

    if (e-p > 0)
    {
	*vs_inc(&out)= b64[p[0] >> 2];
	if (e-p == 1)
	{
	    *vs_inc(&out)= b64[((p[0] & 0x03) << 4)];
	    *vs_inc(&out)= '=';
	}
	else
	{
	    *vs_inc(&out)= b64[((p[0] & 0x03) << 4) | (p[1] >> 4)];
	    *vs_inc(&out)= b64[((p[1] & 0x0f) << 2)];
	}
	*vs_inc(&out)= '=';
	n++;
    }

    if (n != 0)
    {
	if (crnl) *vs_inc(&out)= '\r';
	*vs_inc(&out)= '\n';
    }

    return vs_return(&out);
}


/* EMAIL_ENCODE_BLOCK - Apply an appropriate encoding to a hunk of content.
 * Returns encoding type and encoded content.  Encoded type in is static
 * memory, encoded text is in dynamic.  Encoded text is NULL if the original
 * is to be used.  The input text can contain nil's, but the output text
 * never does and is null terminated.
 */

void email_encode_block(char *text, int len, char *type,
	char *enctype, char **enctext, int crnl)
{
    char *p;
    int linlen, maxlen, istext;
    int n8bit, nnull;

    /* If no encoding was given, choose one */
    if (*enctype == ENC_DUNNO)
    {
	/* We always encode these base-64 */
	if (!strncasecmp(type,"image/",6) || !strncasecmp(type,"audio/",6) ||
	     !strncasecmp(type,"video/",6))
	    *enctype= ENC_B64;
	else
	{
	    /* Collect some statistics */
	    nnull= n8bit= linlen= maxlen= 0;
	    for (p= text; p < text+len ; p++)
	    {
		if (*p == '\n')
		{
		    if (linlen > maxlen) maxlen= linlen;
		    linlen= 0;
		}
		else if (*p == '\0')
		    nnull++;
		else
		{
		    linlen++;
		    if (!isascii(*p))
			n8bit++;
		}
	    }

	    istext= !strncasecmp(type,"text/",5);

	    /* Use base64 if no terminal newline or more than 10% (for text
	     * types) or 5% (for others) is 8-bit */
	    if (p[-1] != '\n' || n8bit > len*(istext ? 0.10 : 0.05) )
		*enctype= ENC_B64;
	    /* Use quoted-printable if there are 8-bits or long lines */
	    else if (n8bit > 0 || nnull > 0 ||
		    maxlen > (!strcasecmp(type,"text/plain") ? 100 : 20) )
		*enctype= ENC_QP;
	    else
	    /* Otherwise, leave it unencoded */
		*enctype= ENC_NONE;
	}
    }
    
    /* Do the encoding */
    if (*enctype == ENC_QP)
	*enctext= email_qp_encode(text, len, crnl);
    else if (*enctype == ENC_B64)
    {
	/* if istext, should first convert text to CRNL line breaks */
	*enctext= email_b64_encode(text, len, crnl);
    }
    else
	*enctext= NULL;
}


/* EMAIL_ENCODE - Appropriatedly encode all text in an email object.
 */

void email_encode(Email *email, int crnl)
{
    struct emcont *c;
    char *ntext;
    int i;

    if (email->encoded) return;

    for (c= email->cont; c != NULL; c= c->next)
    {
	for (i= 0; i < 2; i++)
	{
	    if (c->text[i] != NULL)
	    {
		email_encode_block(c->text[i], c->tlen[i], c->type[i],
		    &c->etype[i], &ntext, crnl);
		if (ntext != NULL)
		{
		    free(c->text[i]);
		    c->text[i]= ntext;
		    c->tlen[i]= -1;
		}
	    }
	}
    }
    email->encoded= TRUE;
}


/* EMAIL_WRITE_TO - Write an Email to-list to the given file desciptor.
 * We do line breaking here, but we are pretty sloppy about exactly counting
 * the columns.  After all, the actual maximum line length is 1000 characters,
 * I think, so if we got a bit over 72, it shouldn't be an problem.
 */

void email_write_to(char *head, struct emaddr *to, int crnl, FILE *fp)
{
    int col= strlen(head)-1;
    int ncol= 0, n= 0;

    fputs(head, fp);

    for ( ; to != NULL; to= to->next)
    {
	/* Estimate length of address component */
	col+= strlen(to->addr)+4;
	if (to->name != NULL) col+= strlen(to->name)+3;
	if (n++ > 0) putc(',', fp);
	if (ncol++ > 0 && col > 72)
	{
	    if (crnl) putc('\r',fp);
	    fputs("\n  ",fp);
	    ncol= 0;
	    col= 2;
	}
	email_write_addr(to, fp);
    }
    
    if (crnl) putc('\r',fp);
    putc('\n',fp);
}


/* EMAIL_BOUNDARY - Given an encoded mail message, return a boundary string
 * that doesn't appear in any message.  For no good reason, we make some
 * effort to make a good-looking boundary.
 */

char *email_boundary(struct emcont *c)
{
    int i, col, blen, match;
    char *p, *e;
    char *bound;

    bound= strdup("===--==--==_-_==--==--==");
    blen= strlen(bound);

    for (; c != NULL; c= c->next)
    {
	for (i= 0; i < 2; i++)
	{
	    /* Don't need to check encoded text - boundary can't be there */
	    if (c->text[i] != NULL && c->etype[i] == ENC_NONE)
	    {
		/* Check hunk of text to make sure boundary is not in it */
		match= TRUE;
		col= 0;
		e= c->text[i] + c->tlen[i];
		for (p= c->text[i]; p < e; p++)
		{
		    if (match)
		    {
			if (col-2 == blen)
			{
			    /* Grow boundary */
			    bound= (char *)realloc(bound,blen+5);
			    strcpy(bound+blen,"--==");
			    blen+= 4;
			}
			if ((col < 2 && *p == '-') ||
			    ( col >= 2 && *p == bound[col-2]))
			    col++;
			else
			{
			    match= FALSE;
			    col= 0;
			}
		    }
		    if (*p == '\n') match= TRUE;
		}
	    }
	}
    }
    return bound;
}


/* EMAIL_WRITE_TEXT - Write the given string, possibly converting NL to CRNL.
 * If length is -1, text must be null-terminated, otherwise length gives
 * length of text.  Check for dots at start of line and handle appropriately.
 */

void email_write_text(char *s, int len, int crnl, int dotmode, FILE *fp)
{
    int newline= TRUE;

    for (; len != 0 && (len > 0 || *s != '\0'); s++, len--)
    {
	if (dotmode != 0 && newline && *s == '.')
	{
	    if (dotmode == 1 &&
		    (s[1] == '\n' || (s[1] == '\r' && s[2] == '\n')))
	    {
		fputs(". ",fp);
		newline= FALSE;
		continue;
	    }
	    else if (dotmode == 2)
		putc('.',fp);
	}
	newline= (*s == '\n');
	if (crnl && newline) putc('\r', fp);
	putc(*s, fp);
    }
}


/* EMAIL_WRITE_HUNK - Write a content hunk.  It should already have been
 * encoded.  It may be a single block, or it maybe be a multipart/alternative
 * MIME structure.
 */

void email_write_hunk(struct emcont *c, int n, char **boundary, int crnl,
	int dotmode, FILE *fp)
{
    int multipart= FALSE;
    int i, sdesc= FALSE;
    char suffix[30];
    char *cr= crnl ? "\r" : "";

    if (c->text[0] == NULL)
    {
	if (c->text[1] == NULL)
	    return;
    }
    else if (c->text[1] != NULL)
    {
	multipart= TRUE;
	if (*boundary == NULL)
	{
	    *boundary= email_boundary(c);
	    suffix[0]= '\0';
	}
	else
	    sprintf(suffix,"--alt%d",n);

	fprintf(fp,"Content-Type: multipart/alternative;%s\n"
		   "  boundary=\"%s%s\"%s\n",cr,*boundary,suffix,cr);
	if (c->desc[0] != NULL && c->desc[1] != NULL &&
		!strcmp(c->desc[0],c->desc[1]))
	{
	    sdesc= TRUE;
	    fprintf(fp,"Content-Description: %s%s\n",
		    c->desc[0],cr);
	}
	if (crnl) putc('\r',fp); putc('\n',fp);

    }

    for (i= 0; i < 2; i++)
    {
	if (c->text[i])
	{
	    if (multipart) fprintf(fp,"%s\n--%s%s%s\n",cr,*boundary,suffix,cr);
	    fprintf(fp,"Content-Type: %s%s\n", c->type[i],cr);
	    if (c->etype[i] != ENC_NONE)
		fprintf(fp,"Content-Transfer-Encoding: %s%s\n",
		    c->etype[i] == ENC_QP ? "quoted-printable" : "base64",cr);
	    if ((c->desc[i] != NULL) && !sdesc)
		fprintf(fp,"Content-Description: %s%s\n",c->desc[i],cr);

	    if (crnl) putc('\r',fp); putc('\n',fp);

	    email_write_text(c->text[i], c->tlen[i],
		crnl && (c->etype[i] == ENC_NONE), dotmode, fp);
	}
    }
    if (multipart) fprintf(fp,"%s\n--%s%s--%s\n",cr,*boundary,suffix,cr);
}


/* EMAIL_WRITE - Write an email message to the given file descriptor.
 * Output includes standard headers, multipart boundaries, etc.  The
 * Email object is not deallocated.  Flags is a boolean OR of EFG_*
 * settings.
 */

void email_write(Email *email, int flags, FILE *fp)
{
    struct tm *tm;
    time_t now= time(0L);
    char date[32];
    char hostname[BFSZ];
    char *boundary;
    struct emcont *c;
    int n, multipart;
    int crnl= (flags & EFG_CRNL);
    int dotmode= (flags & EFG_DOTSPACE) ? 1 : ((flags & EFG_DOTTWICE) ? 2 : 0);
    char *cr= crnl ? "\r" : "";
    static int nmsg= 0;

    /* Get host name for message ID and maybe for From address */
    gethostname(hostname,BFSZ);

    /* Read config file for "Organization" setting and default From settings */
    read_config_file();
    if (email->from.addr == NULL)
    {
	if (cf_email_fromaddr != NULL)
	    email->from.addr= strdup(cf_email_fromaddr);
	else
	{
	    email->from.addr= (char *) malloc(strlen(hostname)+10);
	    strcpy(email->from.addr,"backtalk@");
	    strcpy(email->from.addr+9,hostname);
	}
	if (cf_email_fromname != NULL)
	    email->from.name= strdup(cf_email_fromname);
    }

    /* Print Date: line */
    mime_time(now, date);
    fprintf(fp,"Date: %s%s\n",date,cr);

    /* Print From: line */
    fputs("From: ",fp);
    email_write_addr(&email->from, fp);
    if (crnl) putc('\r',fp); putc('\n',fp);

    /* Print Reply-To: line */
    if (email->replyto.addr != NULL)
    {
	fputs("Reply-To: ",fp);
	email_write_addr(&email->replyto, fp);
	if (crnl) putc('\r',fp); putc('\n',fp);
    }

    /* Print Subject: line */
    if (email->subj != NULL)
	fprintf(fp,"Subject: %s%s\n",email->subj,cr);

    /* Print Message-ID: line */
    fprintf(fp,"Message-ID: <%d.%d.%d@%s>%s\n",getpid(),now,nmsg++,hostname,cr);

    /* Print To: line */
    email_write_to("To: ",email->to[REC_TO],crnl,fp);

    /* Print Cc: line */
    if (email->to[REC_CC] != NULL)
	email_write_to("Cc: ",email->to[REC_CC],crnl,fp);

    /* Print Bcc: line */
    if ((flags & EFG_BCC) && email->to[REC_BCC] != NULL)
	email_write_to("Bcc: ",email->to[REC_BCC],crnl,fp);

    /* Print Organization: line */
    if (cf_email_organization != NULL)
	fprintf(fp,"Organization: %s%s\n",cf_email_organization,cr);

    /* Print X-Mailer: line */
    fprintf(fp,"X-Mailer: Backtalk (%d.%d.%d)%s\n",
	VERSION_A,VERSION_B,VERSION_C,cr);

    /* Print MIME-Version: line */
    fprintf(fp,"MIME-Version: 1.0%s\n",cr);

    /* Encode message body, if not already encoded */
    email_encode(email,crnl);

    /* Headers for multipart/mixed and multipart/related messages */
    if (multipart= (email->cont->next != NULL))
    {
	/* Construct a boundary */
	email->boundary= email_boundary(email->cont);

	/* Start Multipart document */
	fprintf(fp,"Content-Type: multipart/related;%s\n"
		   "  boundary=\"%s=\"%s\n", cr,email->boundary,cr);
	if (crnl) putc('\r',fp); putc('\n',fp);

	email_write_text(MIME_MESSAGE, -1, crnl, dotmode, fp);
	if (crnl) putc('\r',fp); putc('\n',fp);

    }

    /* Message body */
    for (c= email->cont, n= 1; c != NULL; c= c->next, n++)
    {
	if (multipart) fprintf(fp,"--%s=%s\n",email->boundary,cr);
	email_write_hunk(c, n, &email->boundary, crnl, dotmode, fp);
    }
    if (multipart) fprintf(fp,"--%s=--%s\n", email->boundary, cr);
}

#endif /* EMAIL */
