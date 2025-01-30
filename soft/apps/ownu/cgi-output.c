#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

/* VERSION HISTORY:
 * 12/14/95 - Last version before I started collecting version history. [jdw]
 * 06/24/97 - Added version history. [jdw]
 * 12/29/03 - converted to stdargs. [jdw]
 */

char digits[]= "0123456789";

char *index();


/* PRINT_CGI_HEADER - start a CGI document with the given title.
 */

print_cgi_header(char *title)
{
	printf("Content-type: text/html\n\n");
	printf("<HTML><HEAD>\n<TITLE>%s</TITLE>\n</HEAD><BODY>\n",title);
	printf("<H1>%s</H1>\n",title);

}

/* PRINT_CGI_TRAILER - end a CGI document.
 */

print_cgi_trailer()
{
	printf("</BODY></HTML>\n");
}


/* SHOW_PAGE - Tell the client to show the given file. Return non-zero if it
 * doesn't exist.
 */

int show_page(char *file)
{
FILE *fp;
int ch;

	if ((fp= fopen(file,"r")) == NULL)
		return(1);

	printf("Content-type: text/html\n\n");

	while((ch= getc(fp)) != EOF)
		putchar(ch);
	
	fclose(fp);
	return(0);
}


/* PRINT_EXPAND - print out at most "maxlen" characters of the given string, in
 * a field of width "minlen", left-justifying it if "left" is true, right-
 * justifying it otherwise.  And, oh yeah, expand out <, >, and & characters
 * while we are at it, still counting the expanded characters as just one.
 * If "cntl" is true, also do control-character expansion.
 */

void print_expand(FILE *fp, char *s, int left, int cntl, int minlen, int maxlen)
{
int len,pad;
int i;
char ch;

    len= strlen(s);
    if (maxlen != -1 && len > maxlen)
	len= maxlen;
    if (len < minlen)
	pad= minlen - len;
    else
	pad= 0;

    if (!left)
	for (i= 0; i < pad; i++)
	    putc(' ',fp);
	
    for (i= 0; i < len; i++)
    {
	    switch(ch= s[i])
	    {
	case '<':
	    fputs("&lt;",fp);
	    break;
	case '>':
	    fputs("&gt;",fp);
	    break;
	case '&':
	    fputs("&amp;",fp);
	    break;
	default:
	    if (cntl)
	    {
		if (!isascii(ch))
		{
		    putc('M',fp);
		    putc('-',fp);
		    ch= toascii(ch);
		}
		if (!isprint(ch))
		{
		    putc('^',fp);
		    ch= (ch == 0177) ? '?' : (ch + '@');
		}
	    }
	    putc(ch,fp);
	    break;
	}
    }

    if (left)
	for (i= 0; i < pad; i++)
	    putc(' ',fp);
}


/* VCGIPRINTF and CGIPRINTF - These are like printf and (sort of) vprintf,
 * but add two additional format directives:
 *
 *   %S - print a string, expanding out the <, > and & characters.
 *   %C - print a character, expanding it if it is <, >, or &.
 *
 * If a # option is given, these also expand out control-characters.  The
 * usual width, precision and justification flags may be used with this.
 * Most standard printf() format direct are supported, but %n is not.
 */

void vcgifprintf(FILE *fp,char *fmt, va_list ap)
{
#define LFSZ 40
char lfmt[LFSZ+15];	/* Oversize because we only check limit in loops */
int width, precision;
char dwflag,left,cntl;
char cbuf[2];
int i,n;

    while (*fmt != '\0')
    {
	if (*fmt == '%')
	{
	    if (*(++fmt) == '%')
	    {
		putc(*(fmt++),fp);
		continue;
	    }

	    width= -1;
	    precision= -1;
	    lfmt[0]= '%';
	    i= 1;
	    cntl= left= 0;

	    /* Scan past flags */
	    while (index("#0-+ ",*fmt) != NULL && i < 8)
	    {
		if (*fmt == '-') left= 1;
		if (*fmt == '#') cntl= 1;
		lfmt[i++]= *(fmt++);
	    }

	    /* Scan the width */
	    if (isdigit(*fmt))
	    {
		width= atoi(fmt);
		do {
			lfmt[i++]= *(fmt++);
		} while (isdigit(*fmt) && i < LFSZ);
	    }
	    else if (*fmt == '*')
	    {
		width= va_arg(ap, int);
		if (width > 10000) width= 10000;
		sprintf(lfmt+i,"%d%n", width,&n);
		i+= n;
		fmt++;
	    }
	    else
		width= -1;

	    /* Check for . separator */
	    if (*fmt == '.')
	    {
		fmt++;
		lfmt[i++]= '.';
	    }

	    /* Scan the precision */
	    if (isdigit(*fmt))
	    {
		precision= atoi(fmt);
		do {
			lfmt[i++]= *(fmt++);
		} while (isdigit(*fmt) && i < LFSZ);
	    }
	    else if (*fmt == '*')
	    {
		precision= va_arg(ap, int);
		if (precision > 10000) precision= 10000;
		sprintf(lfmt+i,"%d%n", precision,&n);
		i+= n;
		fmt++;
	    }
	    else
		precision= -1;
	    
	    /* Scan data-width modifier */
	    dwflag= *fmt;
	    if (*fmt == 'h' || *fmt == 'l')
		lfmt[i++]= *(fmt++);
	    
	    lfmt[i++]= *fmt;
	    lfmt[i]= '\0';

	    switch (*fmt)
	    {
	    case 'S':
		print_expand(fp,va_arg(ap, char *),left,cntl,width,precision);
		break;

	    case 'C':
		cbuf[0]= va_arg(ap, int);
		cbuf[1]= '\0';
		print_expand(fp,cbuf,left,cntl,width,precision);
		break;

	    case 'p': case 's':
		fprintf(fp,lfmt, va_arg(ap, char *));
		break;

	    case 'f': case 'e': case 'g':
		fprintf(fp,lfmt, va_arg(ap, double));
		break;

	    case 'c':
		fprintf(fp,lfmt, va_arg(ap, int));
		break;

	    case 'd': case 'i': case 'o': case 'u': case 'X': case 'x':
		if (dwflag == 'l')
		    fprintf(fp,lfmt, va_arg(ap, long));
		else if (dwflag == 'h')
		    fprintf(fp,lfmt, va_arg(ap, short));
		else
		    fprintf(fp,lfmt, va_arg(ap, int));
		break;

	    default:
		printf("%s",lfmt);
		break;
	    }
	    fmt++;
	}
	else
	    putc(*(fmt++),fp);
    }
}


void vcgiprintf(char *fmt, va_list ap)
{
    vcgifprintf(stdout,fmt,ap);
}


void cgiprintf(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vcgiprintf(fmt,ap);
    va_end(ap);
}
