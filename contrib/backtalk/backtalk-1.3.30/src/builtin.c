/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Miscellaneous built-in function defintions -- not that there aren't others
 * all over the place.
 */

#include "backtalk.h"

#include "sysdict.h"
#include "builtin.h"
#include "stack.h"
#include "free.h"
#include "str.h"
#include "http.h"
#include "date.h"
#include "showopt.h"

#ifdef HAVE_RANDOM
#define RANDNUM(r) (random() % r)
#define SEEDRAND(x) srandom(x)
#else
#define RANDNUM(r) ((int)(rand()*((double)r/RAND_MAX)))
#define SEEDRAND(x) srand(x)
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

extern char *re_comp(char *);
extern int re_exec(char *);

int seeded= FALSE;

/* FUNC_RAND:  <range> rand <int>
 * Return a random number between zero and range minus one.
 * If srand has never been called, we seed with the time of day.
 */

void func_rand()
{
    int range= pop_integer();

    if (range <= 0) die("Range must be positive");

    if (!seeded)
    {
	SEEDRAND(time(0));
	seeded= TRUE;
    }
    push_integer(RANDNUM(range));
}


/* FUNC_SRAND:  <seed> srand -
 * Seed the random number generator with the given value.  Normally, you don't
 * call this, but just let the first call to rand() generate its own seed, but
 * if you want reproducability for debugging, this is good.
 */

void func_srand()
{
    SEEDRAND(pop_integer());
    seeded= TRUE;
}

/* FUNC_TIME:  - time <seconds>
 * Return current time in seconds.
 */

void func_time()
{
    push_time(time(0));
}


/* FUNC_CTIME:  <seconds> ctime <string>
 * Convert a time in seconds to the standard unix-format time string
 * (except we delete the trailing newline and use the timezone in the
 * timezone variable).
 */

void func_ctime()
{
    time_t clock= pop_time();
    char *c;

    c= ctimez(&clock,sdict(VAR_TIMEZONE));
    *firstin(c,"\n")= '\0';
    push_string(c,TRUE);
}


/* FUNC_ZTIME:  <seconds> ztime <string>
 * Convert a time in seconds to a time string formatted like the output
 * of the unix "date" command (which differs from "ctime" in including the
 * timezone abbreviation before the year, for example:
 *    Mon Jan  5 09:09:53 EST 1998
 * Note that the time zone abbreviation is not always three characters, and
 * may sometimes be missing altogether.
 */

void func_ztime()
{
#ifdef HAVE_TM_ZONE
#define TZN tm->tm_zone
#else
#ifdef HAVE_TZNAME
    extern char *tzname[2];
#define TZN tzname[tm->tm_isdst]
#endif
#endif
    time_t clock= pop_time();
    struct tm *tm;
    char *c, *z;
    int len;

    tm= localtimez(&clock,sdict(VAR_TIMEZONE));
    c= asctimez(tm,sdict(VAR_TIMEZONE));
    *firstin(c,"\n")= '\0';

    /* If time zone abbreviation unknown, just return ctime string */
#ifdef HAVE_TZNAME
    if (tm->tm_isdst < 0 || tzname[tm->tm_isdst][0] == '\0')
#else
    if (TZN == NULL || TZN[0] == '\0')
#endif
    {
	push_string(c,TRUE);
	return;
    }

    /* Insert Time Zone into string */
    len= strlen(TZN);
    z= (char *)malloc(27 + len);
    strncpy(z,c,20);
    strncpy(z+20,TZN,len);
    strcpy(z+20+len,c+19);

    push_string(z,FALSE);
}


/* FUNC_HTIME: <time> htime <string>
 * Convert a time in seconds to an http time string, like
 *     Wdy, DD-Mon-YYYY HH:MM:SS GMT
 */

void func_htime()
{
    time_t clock= pop_time();
    char bf[30];
    (void)http_time(clock,bf);
    push_string(bf,TRUE);
}

/* FUNC_ITIME: <time> itime <string>
 * Convert a time to an international standard date/time string, like
 *    1997-07-16T19:20:30Z
 * Times are always in UTC (which is what the Z means)
 */

void func_itime()
{
    time_t clock= pop_time();
    struct tm *gmt;
    char bf[30];

    gmt= gmtime(&clock);
#ifdef HAVE_STRFTIME
    strftime(bf, 30, "%Y-%m-%dT%H:%M:%SZ", gmt);
#else
    sprintf(bf,"%04d-%02d-%02dT%02d:%02d:%02dZ",
	    gmt->tm_year+1900, gmt->tm_mon+1, gmt->tm_mday,
	    gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
#endif
    push_string(bf,TRUE);
}


/* FUNC_DTIME: <string> <future> <start> dtime <seconds>
 * Convert a free-format time string to a unix time in seconds.  If <future>
 * is true, resolve ambiguous times (like "Wednesday") in the favor of near
 * future times.  Otherwise, resolve them in favor of near past times.
 * If <start> is true, return the begining of the time intervals with duration
 * (like "March 1996").  Otherwise, return the end.
 *
 * If the string is invalid, return a zero.
 */

void func_dtime()
{
    int rc;
    time_t clock;
    struct tm tm;
    int start= pop_integer();
    int future= pop_integer();
    char *str= pop_string();

    clock= strtime(str, future, start, sdict(VAR_TIMEZONE), &tm);
    free(str);
    push_time((int)clock < 0 ? 0 : (clock == 0 ? 1 : clock));
}


/* FUNC_TIMEOUT:  <seconds> timeout -
 * Bomb if the program is still running after the given number of seconds have
 * elapsed.  This is to control infinite loops while debugging scripts. 
 * Setting it to zero cancels the timeout.
 */

void func_timeout()
{
    alarm(pop_integer());
}


/* FUNC_SLEEP:  <seconds> sleep -
 * Sleep.
 */

void func_sleep()
{
    sleep(pop_integer());
}


/* FUNC_CHOMP <string> chomp <string>
 * Delete all tailing newlines from a string.
 */

void func_chomp()
{
    char *s= peek_string();
    int i= strlen(s);

    for (i--; s[i] == '\n' || s[i] == '\r'; i--) s[i]= '\0';
}


/* FUNC_SUBSTR: <string> <index> <count> substr <substring>
 * Find the <count> character substring of <string> starting at <index>.
 * <Count> may be negative, in which case returns an string of length 
 * |<count>| ending with the character <index>.  Indices are zero-based.
 */

void func_substr()
{
    int count= pop_integer();
    int index= pop_integer();
    char *string= peek_string();
    char *substring;
    int len;

    if (index < 0) die("negative string index");

    if (count < 0)
    {
	count= -count;
	index= index + 1 - count;
    }
    
    len= strlen(string);
    if (index > len)
	count= index= 0;
    else if (count + index > len)
	count= len - index;

    substring= (char *)malloc(count+1);
    strncpy(substring,string+index,count);
    substring[count]= '\0';

    free(string);

    repl_top((void *)substring);
}


/* FUNC_INLIST: <word> <list> <delim> inlist <bool>
 * Return true if the given word appears in the given list, in which words are
 * delimited by the characters in delim.
 */

void func_inlist()
{
    char *del= pop_string();
    char *list= pop_string();
    char *word= pop_string();

    push_integer(inlist(word, list, del));
    free(del);
    free(list);
    free(word);
}


/* FUNC_PARSE: <string> <delim>  parse  <rest> <first>
 * Find the first word of the string, where words are delimited by any
 * number of characters from the list of delimiters.
 */

void func_parse()
{
    char *del= pop_string();
    char *str= peek_string();
    char *b,*e,*w;
    int l;

    /* Find beginning and end of word */
    b= firstout(str,del);
    e= firstin(b,del);

    /* Copy the first word into its own memory */
    w= (char *)malloc(e - b + 1);
    strncpy(w, b, e-b);
    w[e-b]= '\0';

    /* Put rest of string back on the stack */
    b= firstout(e,del);
    l= strlen(b);
    memmove(str,b,l+1);
    /* repl_top((void *)str);  - unnecessary */

    push_string(w,FALSE);

    free(del);
}


/* FUNC_SPLIT: <string> <delim>  split  ...
 * Break up a string into words, and push each word on the stack.  Words are
 * delimited by any number of characters from the list of delimiters.
 */

void func_split()
{
    char *del= pop_string();
    char *str= pop_string();
    char *b,*e,*w;

    b= firstout(str,del);
    while (*b != '\0')
    {
	e= firstin(b,del);
	w= firstout(e,del);
	*e= '\0';
	push_string(b,TRUE);
	b= w;
    }

    free(str);
    free(del);
}


/* FUNC_CLIP: <string> <delim>  parse  <rest> <first>
 * Find the occurance of a delimiter, and return the sections of the string
 * before and after that occurance.  This is exactly like func_parse, except
 * multiple consecutive delimiters aren't rolled together.
 */

void func_clip()
{
    char *del= pop_string();
    char *str= peek_string();
    char *p, *w;
    int l;

    /* Find end of first word */
    p= firstin(str,del);

    /* Copy the first word into its own memory */
    w= (char *)malloc(p - str + 1);
    strncpy(w, str, p-str);
    w[p-str]= '\0';

    /* Put rest of string back on the stack */
    if (*p == '\0')
	str[0]= '\0';
    else
    {
	l= strlen(p+1);
	memmove(str,p+1,l+1);
    }
    /* repl_top((void *)str);  - unnecessary */

    push_string(w,FALSE);

    free(del);
}


/* FUNC_VERSION: version <version_string>
 * Push the version string.  This should really be a variable, but instead
 * it's a function that behaves like a variable.  Actually, this should never
 * get executed, because it is always taken care of by the compiler.
 */

void func_version()
{
    char bf[BFSZ];

    push_string(version_string(bf),TRUE);
}


/* FUNC_CVN: <string> cvn <literal>
 * Convert a string into a literal (name).  Thus, the string (abc) becomes
 * the literal /abc.  If the literal is in the system dictionary, we bind
 * it now.
 */

void func_cvn()
{
    char *str;
    HashEntry *h;

    str= pop_string();
    label_ok(str);
    if ((h= FindHashEntry(&syshash,str)) != NULL)
    {
	push_bound_literal(h-sysdict);
    }
    else
    {
	push_literal(str,FALSE);
    }
}


/* FUNC_CGIQUOTE:  <string> cgiquote <string>
 * Expand out a string for use as a parameter to a cgi program.  See the
 * cgiquote function in str.c for details.
 */

void func_cgiquote()
{
    char *input= peek_string();
    char *output;

    output= cgiquote(input);
    free(input);
    repl_top((void *)output);
}


/* FUNC_JSQUOTE:  <string> jsquote <string>
 * Expand out a string for use as a javascript string, enclosing it in single
 * quotes and escaping any characters that need escaping.  See the jsquote
 * function in str.c for details.
 */

void func_jsquote()
{
    char *input= peek_string();
    char *output;

    output= jsquote(input);
    free(input);
    repl_top((void *)output);
}


/* FUNC_CAP: <string> cap <String>
 * Make the first ascii letter of each word in the given string capital.
 */

void func_cap()
{
    capfirst(peek_string());
}


/* FUNC_CAPS: <string> caps <String>
 * Make all lower case letters upper case.
 */

void func_caps()
{
    capall(peek_string());
}

/* FUNC_SHOWOPT: <html-flag> showopt <options>
 * Generate text describing configuration.  If the flag is true, do it in
 * HTML format instead of plain text.
 */

void func_showopt()
{
    int html= pop_boolean();

    push_string(show_options(html),FALSE);
}
