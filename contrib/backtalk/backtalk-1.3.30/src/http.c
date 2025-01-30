/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Functions to generate http headers */

#include "backtalk.h"

#include "sysdict.h"
#include "http.h"
#include "stack.h"
#include "str.h"
#include "cgi_cookie.h"
#include "date.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* Linked list of cookies to be set in the current page */

struct cook {
	char *name;		/* Name of cookie */
	char *value;		/* Value of cookie */
	time_t expire;		/* Expiration time of cookie - 0= none */
	struct cook *next;	/* Next cookie to set */
	} *cookies= NULL;
int http_sent= FALSE;		/* Have we sent the http headers? */

extern FILE *output;


/* SET_COOKIE - Add a cookie to the list of cookies to be set by the
 * current page.  If the expiration date is zero, the cookie expires when
 * the user exits his browser.  "name" and "value" should be in malloc'ed
 * memory, which we will eventually be freeing.  The cookie value should not
 * contain spaces or semicolons.  If it is too late to add a cookie (we have
 * already printed something), then return nonzero.
 */

int set_cookie(char *name, char *value, time_t expire)
{
    struct cook *nck,*ck,*pck;

    if (http_sent) return 1;

    /* Walk through the list of cookies, looking for duplicates */
    for (ck= cookies, pck= NULL; ck != NULL; pck= ck, ck= ck->next)
	if (!strcmp(name,ck->name))
	{
	    /* Overwrite an existing cookie */
	    free(name);
	    free(ck->value);
	    ck->value= value;
	    ck->expire= expire;
	    return 0;
	}

    /* Make a new cookie list entry */
    nck= (struct cook *)malloc(sizeof(struct cook));
    nck->name= name;
    nck->value= value;
    nck->expire= expire;
    nck->next= NULL;

    /* Append the new cookie to the list */
    if (pck == NULL)
	cookies= nck;
    else
	pck->next= nck;

    return 0;
}


/* FUNC_SETCOOKIE:   <name> <value> <expires> setcookie -
 * Set a cookie.  The value should contain no spaces or semicolons.  Expires
 * is a unix time, or zero if the cookie should expire when the browser exits.
 */

void func_setcookie()
{
    time_t expire= pop_time();
    char *value= pop_string();
    char *name= pop_string();

    set_cookie(name, value, expire);
}


/* SEND_HTTP_HEADERS - Output the http headers for the current page based
 * on the current system dictionary settings.  If they have already been
 * sent, do nothing.
 */

void send_http_headers()
{
    struct cook *ck, *nck;

    /* If headers already sent, or have been turned off, do nothing */
    if (http_sent || !bdict(VAR_HTTP_HEADERS))
	return;

    http_sent= TRUE;

    /* Always output a content type header */
    fprintf(output,"Content-Type: %s\n",sdict(VAR_HTTP_CONTENT_TYPE));

    /* If caching has been turned off, do various things to suppress it */
    if (bdict(VAR_HTTP_NO_CACHE))
    {
	fprintf(output,"Expires:  Thu, 01 Jan 1970 00:00:00 GMT\n");
	fprintf(output,"Pragma:  no-cache\n");
	fprintf(output,"Cache-Control: no-cache\n");
    }
    /* Otherwise, if an expiration time has been set, send that */
    else if (tdict(VAR_HTTP_EXPIRES) != 0)
    {
	char bf[30];
	fprintf(output,"Expires: %s\n", http_time(tdict(VAR_HTTP_EXPIRES),bf));
    }

    /* Set our cookies */
    for (ck= cookies; ck != NULL; ck= nck)
    {
	fprintf(output, "Set-Cookie: %s=%s; path=/", ck->name, ck->value);
	if (ck->expire > 0L)
	{
	    char bf[30];
	    fprintf(output,"; expires=%s",http_time(ck->expire,bf));
	}
	putc('\n',output);

	nck= ck->next;
	free(ck->name);
	free(ck->value);
	free(ck);
    }
    cookies= NULL;

    /* If an redirect has been set, send that */
    if (*sdict(VAR_HTTP_LOCATION) != '\0')
	fprintf(output,"Location: %s\n", sdict(VAR_HTTP_LOCATION));

    /* End the http headers */
    putc('\n',output);
}


/* FREE_COOKIES - discard any cookies still sitting in memory.  This is for
 * debugging memory management only.
 */

#ifdef CLEANUP
void free_cookies()
{
    struct cook *ck, *nck;

    for (ck= cookies; ck != NULL; ck= nck)
    {
	nck= ck->next;
	free(ck->name);
	free(ck->value);
	free(ck);
    }
    cookies= NULL;
}
#endif /*CLEANUP*/


/* FUNC_GETCOOKIE:   <name> getcookie <value> 0
 *                   <name> getcookie 1
 * 
 * Look up the value of the named cookie.  If the cookie is not defined, just
 * push a one.  Otherwise, push its value and a zero.
 */

void func_getcookie()
{
    char *name= pop_string();
    char *value= get_cookie(name);

    free(name);
    if (value != NULL)
    {
	push_string(value,FALSE);
	push_integer(0);
    }
    else
	push_integer(1);
}


/* GET_HTTP_HOST
 * Return the HTTP_HOST value.  Usually this comes from the environment, but
 * we provide the ability to override or default it.
 */

char *get_http_host()
{
#ifdef OVERRIDE_HTTP_HOST
    return OVERRIDE_HTTP_HOST;
#else
    char *http_host= getenv("HTTP_HOST");
#ifdef DFLT_HTTP_HOST
    if (http_host == NULL) return DFLT_HTTP_HOST;
#endif
    return http_host;
#endif
}


/* FUNC_BACKTALKURL:  - backtalkurl <url>
 *
 * Return the URL that Backtalk was invoked as in the current query. 
 * Typically something like 'http://www.yourhostname.com/cgi-bin/pw/backtalk'.
 * Returns () if unknown.
 */

void func_backtalkurl()
{
    char *url;
    int hostlen;
    char *http_host= get_http_host();
    char *script_name= getenv("SCRIPT_NAME");
    char *https= getenv("HTTPS");
    int secure= (https != NULL && (!strcmp(https,"on") || !strcmp(https,"ON")));

#ifdef DFLT_SCRIPT_NAME
    if (script_name == NULL) script_name= DFLT_SCRIPT_NAME;
#endif

    if (http_host == NULL || script_name == NULL)
    {
	push_string("",TRUE);
	return;
    }

    hostlen= strlen(http_host);
    url= (char *)malloc(hostlen + strlen(script_name) + 8 + secure);
    strcpy(url, secure ? "https://" : "http://");
    strcpy(url+7+secure, http_host);
    strcpy(url+7+secure+hostlen, script_name);

    push_string(url,FALSE);
}


/* FUNC_SERVERURL:  - serverurl <url>
 *
 * Return the URL of the host as invoked in the current query.   Typically
 * something like 'http://www.yourhostname.com'  Returns () if unknown.
 */

void func_serverurl()
{
    char *url;
    char *http_host= get_http_host();
    char *https= getenv("HTTPS");
    int secure= (https != NULL && (!strcmp(https,"on") || !strcmp(https,"ON")));

    if (http_host == NULL)
    {
	push_string("",TRUE);
	return;
    }

    url= (char *)malloc(strlen(http_host) + 8 + secure);
    strcpy(url, secure ? "https://" : "http://");
    strcpy(url+7+secure, http_host);

    push_string(url,FALSE);
}
