/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"
#include "str.h"

#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* Firstin() returns a pointer to the first character in s that is in l.
* \0 is always considered to be in the string l.
*
* Firstout() returns a pointer to the first character s that is not in l.
* \0 is always considered to be not in the string l.
*
* Note that unlike strpbrk() these never return NULL.  They always return
* a valid pointer into string s, if only a pointer to it's terminating
* \0.  They are amazingly useful for simple tokenizing.
 */

#ifdef HAVE_STRSPN

char *firstin(char *s, char *l)
{
    return s+strcspn(s,l);
}

char *firstout(char *s, char *l)
{
    return s+strspn(s,l);
}

#else

char *firstin(char *s, char *l)
{
    for (;*s;s++)
	if (strchr(l,*s)) break;
    return s;
}

char *firstout(char *s, char *l)
{
    for (;*s;s++)
	if (!strchr(l,*s)) break;
    return s;
}

#endif /* HAVE_STRSPN */


/* nthchar(string, n, char)
*  Return a pointer to the nth occurance of the given character in the
*  given string.  If there are fewer than n, return a pointer to the
*  terminating '\0' of the string.
 */

char *nthchar(char *s, int n, char c)
{
    if (n > 0)
	for ( ; *s != '\0'; s++)
	    if (*s == c && --n == 0) break;
    return s;
}


/* REFLECT - Reverse the sequence of characters between f and l.
 */

void reflect(char *f, char *l)
{
    char t;

    while (f < l)
    {
	t= *f; *f= *l; *l= t;
	f++; l--;
    }
}


/* INLIST - Check if the given word is in the list given.  The list is a
* text string with words separated by any combination of characters in
* the del delimiter string.
 */

int inlist(char *word, char *list, char *del)
{
    char *p= list;
    int i= strlen(word);

    if (word == NULL || word[0] == '\0')
	return 0;
    while ( (p= strstr(p,word)) != NULL)
    {
	if ( (p == list || strchr(del,p[-1]) != NULL) &&
		(p[i] == '\0' || strchr(del,p[i]) != NULL) )
	    return 1;
	p++;
    }
    return 0;
}


/* NODOTDOT - Check a path name to make sure it has no .. components and is
* not an absolute pathname.  This would be bad in PATH_INFO, since it could
* be exploited to execute arbitrary files on the server.  The Apache httpd
* already defends against this, but I don't know if all httpd's do.  Anyway,
* we check this on file included from the script too.
 */

void nodotdot(char *path)
{
    char *e;

    if (path[0] == '/')
	die("Illegal absolute path name \"%s\"",path);

    for (;;)
    {
	e= firstin(path,"/");
	if (e == path + 2 && path[0] == '.' && path[1] == '.')
	    die("Illegal .. in path \"%s\"",path);
	if (*e == '\0') break;
	path= e+1;
    }
}


/* MATCH -- Check if a name matchs a pattern string.  Letters after the _ in
* the pattern are optional to a successful match.  The pattern may be
* terminated either by a null or a colon.  If null_term is true, the name is
* terminated by a null, if false, a null or any nonalphanumeric character will
* terminate it.  The comparison is case insensitive.  The pattern is always
* terminated by a null or a colon.
 */

int match(char *name, char *pat, int null_term)
{
    int passed_dash= 0;
    char cn, cp;
#define pend(c) ((c) == ':' || (c) == '\0')
#define nend(c) ((c) == '\0' || (!null_term && (!isascii(c) || !isalnum(c))))

    for ( ; !pend(*pat) && !nend(*name); name++, pat++)
    {
	if (*pat == '_')
	{
	    passed_dash= 1;
	    name--;
	    continue;
	}

	cn= (isascii(*name) && isupper(*name)) ? tolower(*name) : *name;
	cp= (isascii(*pat) && isupper(*pat)) ? tolower(*pat) : *pat;
	if (cn != cp)
	    return 0;
    }

    if (pend(*pat))
	return nend(*name);
    else
	return (passed_dash || *pat == '_');
}


/* VERSION_STRING - store the Backtalk version string in the given buffer
* and return a pointer to it.
*
* Note that we don't normally print VERSION_D.  It's used only for internal
* development increments.  Each actual release of Backtalk will differ in the
* first three numbers.
 */

char *version_string(char *bf)
{
    sprintf(bf,"Backtalk version %d.%d.%d"VERSION_NOTE,
	    VERSION_A, VERSION_B, VERSION_C);
    return bf;
}


/* SB_NAME - Given a filename which may be either a source file name, or a
* binary file name, or a stub containing no suffix, return both the source
* and binary filenames.  Both of the returned names will be in static
* storage.  Returns 1 if for some reason it can't generate the names.  If
* either name is passed in as a NULL, that one won't be generated.
 */

int sb_name(char *name, char **source, char **binary)
{
    int len= strlen(name);
    static char sbf[BFSZ], bbf[BFSZ];

    if (len >= 4 && name[len-3] == '.' && name[len-2] == 'b')
    {
	if (name[len-1] == 't')
	{
	    /* .bt suffix indicates source name */
	    if (source != NULL) *source= strcpy(sbf,name);
	    if (binary == NULL) return 0;
	    if (len + 1 > BFSZ) return 1;
	    strcpy(bbf, name);
	    bbf[len-1]= 'b';
	    *binary= bbf;
	    return 0;
	}
	else if (name[len-1] == 'b')
	{
	    /* .bb suffix indicates binary name */
	    if (binary != NULL) *binary= strcpy(bbf,name);
	    if (source == NULL) return 0;
	    if (len + 1 > BFSZ) return 1;
	    strcpy(sbf, name);
	    sbf[len-1]= 't';
	    *source= sbf;
	    return 0;
	}
    }

    /* Name with no .bt or .bb extension */

    if (len + 4 > BFSZ) return 1;

    if (source != NULL)
    {
	strcpy(sbf, name);
	strcpy(sbf+len, ".bt");
	*source= sbf;
    }
    if (binary != NULL)
    {
	strcpy(bbf, name);
	strcpy(bbf+len, ".bb");
	*binary= bbf;
    }
    return 0;
}


/* CGIQUOTE - Expand out a string for use as a parameter to a cgi program.
* Returned string is in malloced memory, and should be freed by the caller.
*   SPACE   -> +
*   "       -> %22
*   #       -> %23
*   %       -> %25
*   &       -> %26
*   '       -> %27
*   +       -> %2B
*   =       -> %3D
*   >       -> %3E
*   <       -> %3C
*   ?       -> %3F
*   NEWLINE -> %0A
 */

char *cgiquote(char *input)
{
    char *output,*i,*o;

    /* Allocate a buffer big enough to handle string of all pluses */
    output= (char *)malloc(3*strlen(input)+1);

    /* Copy string, making expansions/substitutions as we go */
    for (i=input, o=output; *i != '\0'; i++, o++)
    {
	if (*i == ' ')
	    *o= '+';
	else if (!isascii(*i) || !isprint(*i) ||
		strchr("+&=%#<>?'\"",*i) != NULL)
	{
	    sprintf(o,"%%%02X",*i);
	    o+= 2;
	}
	else
	    *o= *i;
    }
    *o= '\0';

    /* Correct size of output */
    output= (char *)realloc(output, o - output + 1);

    return output;
}


/* JSQUOTE - Expand out a string for use as a Javascript string.  Returned
 * string is in malloced memory and should be freed by the caller.  This
 * encloses the string in single quote marks and also makes the following
 * substitutions:
 *
 *   \       -> \\
 *   '       -> \'
 *   <       -> \x3c
 *   >       -> \x3e
 *   NEWLINE -> \n
 *   TAB     -> \t
 *   CR      -> \r
 * any other non printable character is converted to \xHH where HH is it's
 * hexadecimal value.
 */

char *jsquote(char *input)
{
    char *output,*i,*o;

    /* Allocate a buffer big enough to handle string of all */
    o= output= (char *)malloc(4*strlen(input)+3);

    *(o++)= '\'';

    /* Copy string, making expansions/substitutions as we go */
    for (i=input; *i != '\0'; i++, o++)
	switch (*i)
	{
	    case '\\': *(o++)= '\\'; *o= '\\';   break;
	    case '\'': *(o++)= '\\'; *o= '\'';   break;
	    case '<':  *(o++)= '\\'; *(o++)= 'x'; *(o++)= '3'; *o= 'c';   break;
	    case '>':  *(o++)= '\\'; *(o++)= 'x'; *(o++)= '3'; *o= 'e';   break;
	    case '\n': *(o++)= '\\'; *o= 'n';    break;
	    case '\r': *(o++)= '\\'; *o= 'r';    break;
	    case '\t': *(o++)= '\\'; *o= 't';    break;
	    default:
		       if (!isprint(*i))
		       {
			   sprintf(o,"\\x%02x",*i);
			   o+= 3;
		       }
		       else
			   *o= *i;
	}
    *(o++)= '\'';
    *o= '\0';

    /* Correct size of output */
    output= (char *)realloc(output, strlen(output)+1);

    return output;
}


/* CAPALL - convert all letters in a string to upper case.
 */

void capall(char *p)
{
    for (; *p != '\0'; p++)
	if (isascii(*p) && islower(*p))
	    *p= toupper(*p);
}


/* CAPFIRST - convert first letter of each word in string to upper case.
 */

void capfirst(char *p)
{
    int was_char= 0;

    for (; *p != '\0'; p++)
    {
	if (!was_char && islower(*p))
	{
	    *p= toupper(*p);
	    was_char= 1;
	}
	else
	    was_char= (isalpha(*p) || *p == '-' || *p == '\'');
    }
}


#ifdef HIDE_HTML
/* NOMETA - delete meta characters from the given string, replacing a few.
 */

void nometa(char *p)
{
    for ( ;*p != '\0'; p++)
    {
	if ((unsigned)*p > 127)
	{
	    switch ((unsigned)*p)
	    {
		case  145: *p= '`';  break;		/* Smart single left quote */
		case  146: *p= '\''; break;		/* Smart single right quote */
		case  147:  			/* Smart double left quote */
		case  148: *p= '"';  break;		/* Smart double right quote */
		case  169: *p= 'c';  break;		/* Copyright */
		default:   *p= ' ';  break;		/* Anything else */
	    }
	}
    }
}
#endif /* HIDE_HTML */


/* UNSLASH - convert "\n" to a newline, "\t" to tab, and "\\" to a backslash.
 */

void unslash(char *input)
{
    char *p, *q;

    for(p= q= input; *p != '\0'; p++,q++)
    {
	if (*p == '\\')
	    switch (*(++p))
	    {
		case 'n':
		    *q= '\n';
		    break;
		case 't':
		    *q= '\t';
		    break;
		case '\\':
		    *q= '\\';
		    break;
		default:
		    *q= *p;
	    }
	else
	    *q= *p;
    }
    *q= '\0';
}


/* STRCNT - Count number of times a character appears in a string */

int strcnt(char *s, char c)
{
    int n= 0;
    if (s == NULL) return 0;
    if (c == '\0') return 1;

    for (;*s != '\0'; s++)
	if (*s == c)
	    n++;

    return n;
}


/* STRERROR - Return an error message string for the given errno value.
 * This is built-in on many unixes.
 */

#if !defined(HAVE_STRERROR)
char *strerror(int errno)
{
    extern int sys_nerr;
    extern char *sys_errlist[];

    if (errno < 0 || errno > sys_nerr)
	return "Unknown Error";
    else
	return sys_errlist[errno];
}
#endif /* HAVE_STRERROR */


/* STRDUP - Duplicate a string.  This is actually built-in on many unixes.
 */

#if !defined(HAVE_STRDUP) || defined(MYSTRDUP)
#ifdef MYSTRDUP
char *mystrdup(char *str)
#else
char *strdup(char *str)
#endif
{
    char *dup;

    dup= (char *)malloc( strlen(str)+1 );
    if (dup) strcpy(dup,str);
    return dup;
}
#endif


/* STRNDUP - Duplicate at most n characters of a string.  Actually allocates
 * n+1 bytes, to accomodate a trailing nul.
 */

/*
char *strndup(char *str, int n)
{
    char *dup;
    int l;

    if (n < 0) return NULL;
    l= strlen(str);
    if (l < n) n= l;
    dup= (char *)malloc( n+1 );
    if (dup) {strncpy(dup, str, n); dup[n]= '\0'; }
    return dup;
}
*/


/* STRSTR() find a string in a string.  This is the dumb algorithm, meant only
 * for short strings.  It is only here for those Unixes who don't have one in
 * the system library.
 */

#ifndef HAVE_STRSTR
char *lstrstr(s,p)
char *s, *p;
{
    register char *sp, *pp;

    for(sp= s, pp= p; *sp && *pp; )
    {
	if (*sp == *pp)
	{
	    sp++;
	    pp++;
	}
	else
	{
	    sp= sp - (pp - p) + 1;
	    pp= p;
	}
    }
    return (*pp ? NULL : sp-(pp-p));
}
#endif /* HAVE_STRSTR */


/* MEMMOVE moves a string.  It is like memcpy(), but works correctly with
 * overlapping strings.  (Actually, on some systems memcpy() does too, but
 * not on all.  Sigh.)
 */

#ifndef HAVE_MEMMOVE
char *lmemmove(char *dst, char *src, int n)
{
    char *d= dst;
    if (src > dst)
	for ( ; n > 0; n--)
	    *(dst++)= *(src++);
    else
	for (dst+= n-1, src+= n-1; n > 0; n--)
	    *(dst--)= *(src--);
    return d;
}
#endif /* HAVE_MEMMOVE */


/* MEMCMP - Most systems have memcmp(), but some, including 4.1.3 BSD
 * apparantly are confused about 8-bit data (bytes are supposed to be
 * treated as unsigned, even though the pointers passed in are of type
 * (char *) and char is a signed type - actually the arguments should 
 * probably be (void *) but let's not get into that).  I hope this one
 * is correct.
 */

#ifndef HAVE_MEMCMP
int lmemcmp(char *a, char *b, int n)
{
    unsigned char ca,cb;

    while (n-- > 0)
    {
	ca= *((unsigned char *)a);
	cb= *((unsigned char *)b);
	a++; b++;
	if (ca != cb)
	    return ca - cb;
    }
    return 0;
}
#endif /*HAVE_MEMCMP*/


/* MEMSET - This sets a block of memory to a constant value.  Most systems
 * have one of their own.
 */

#ifndef HAVE_MEMSET
char *lmemset(char *dst, int c, int n)
{
    unsigned char *p= dst;
    for ( ; n > 0; n--)
	*(p++)= (unsigned char)c;
    return dst;
}
#endif /* HAVE_MEMSET */


/* STRCASECMP - this is a case independent version of strcmp, for unixes
 * that don't have one built in.  Another suboptimal implementation, since
 * most unixes do seem to have this and I don't use it too much.
 */

#ifndef HAVE_STRCASECMP
#define ccmp(a,b) ((a) == (b) ? 0 : ((a) > (b) ? 1 : -1))
int lstrcasecmp(unsigned char *s1, unsigned char *s2)
{
    unsigned char c1, c2;

    for ( ; ; )
    {
	if (*s1 == '\0' || *s2 == '\0')
	    return ccmp(*s1,*s2);
	c1= (isascii(*s1) && isupper(*s1)) ? tolower(*s1) : *s1;
	c2= (isascii(*s2) && isupper(*s2)) ? tolower(*s2) : *s2;
	if (c1 != c2)
	    return ccmp(c1,c2);
	s1++;
	s2++;
    }
}

int lstrncasecmp(unsigned char *s1, unsigned char *s2, int n)
{
    unsigned char c1, c2;

    for ( ; ; )
    {
	if (n-- == 0) return 0;
	if (*s1 == '\0' || *s2 == '\0')
	    return ccmp(*s1,*s2);
	c1= (isascii(*s1) && isupper(*s1)) ? tolower(*s1) : *s1;
	c2= (isascii(*s2) && isupper(*s2)) ? tolower(*s2) : *s2;
	if (c1 != c2)
	    return ccmp(c1,c2);
	s1++;
	s2++;
    }
}
#endif /* HAVE_STRCASECMP */


/* MKDIR - Systems that don't have mkdir() are pretty rare these days, so I'd
 * be surprised if anyone ever uses this code, but here it is anyway.  This
 * won't work on many newer Unixes, where mknod() can no longer be used to
 * create directories, but those all have mkdir() so we're all right.
 */

#ifndef HAVE_MKDIR
int mkdir(char *path, int mode)
{
    /* We use hard coded constants here because Unixes old enough not to have
     * mkdir likely don't have defines for the modes.
     */
    return mknod(path, (mode & 07777) | 040000, 0);
}
#endif /* HAVE_MKDIR */


/* VPRINTF, VFPRINTF - I think almost any ANSI C system will have these
 * functions.  Darn near required to, but on the principle of maximum paranoia,
 * here they are.  Mostly we just call the portable vasprintf() function to do
 * the work for us.
 */

#ifndef HAVE_VPRINTF
#if __STDC__
int vfprintf(FILE *fp, const char *fmt, ...)
#else
int vfprintf(fmt, fp, va_alist)
    const char *fmt;
    FILE *fp;
    va_dcl
#endif
{
    va_list ap;
    char *b;
    int n;

    VA_START(ap,fmt);
#ifdef HAVE_DOPRNT
    n= _doprnt(fmt, ap, fp);
#else
    n= vasprintf(&b, fmt, ap);
    if (n > 0 && (fputs(b,fp) < 0)) n= -1;
#endif
    va_end(ap);
    return n;
}

#if __STDC__
int vprintf(const char *fmt, ...)
#else
int vprintf(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
    va_list ap;
    int n;

    VA_START(ap,fmt);
    n= vfprintf(stdout,fmt, ap);
    va_end(ap);
    return n;
}

#endif /* HAVE_VPRINTF */
