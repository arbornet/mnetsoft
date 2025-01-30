/*
 * formvars.c
 * ------------------------------------------
 * Copyright (c) 1997 by John H. Remmers
 * ------------------------------------------
 * $Id: formvars.c 1629 2017-09-20 14:28:14Z cross $
 * ------------------------------------------
 * Decode www-url-encoded form data. Compatible with
 * NCSA 'getcgivars()' function. More efficient, though.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "formvars.h"

/* Convert hex digit to int */
#define Hex2Int(c)  ((c >= 'A') ? (c - ('A' - 10)) : (c - '0'))

static char **table = NULL;	// (name, value) pairs table
static int nvars = -1;		// number of (name,value) pairs
static char **savedtp = NULL;	// saved table ptr from cgival()

/*
 * ------------------------------------------------------------
 * Decode encoded string in-place, build and return list of of
 * form: name1, value1, name2, value2, ..., NULL
 */
static char **
urldecode(char *url)
{
	char *cp;	// pointer into url
	char **vartab;	// pointer to (name,value) list
	char **vtp;	// variable pointer into said list
	char  *dbrk;	// end of decoded segment
	unsigned char c;

	// Pass 1: Count pairs
	nvars = 0;
	for (cp = url; (c = *cp) != '\0'; ++cp)
		if (c == '=')
			++nvars;

	// Allocate list
	vartab = (char **)malloc(2 * nvars * sizeof(char *) + 1);
	if (vartab == NULL) {
		return NULL;
	}
	if (nvars == 0)
		return vartab;

	// Pass 2: Decode url string in-place, build list
	*vartab = dbrk = url;	// append ptr to first name field
	vtp = vartab + 1;
	while ((c = *(url++)) != '\0') {
		switch (c) {
		case '%':{	// decode hex string
				unsigned char   d;
				c = *(url++);
				d = Hex2Int(c);
				c = *(url++);
				d = 16 * d + Hex2Int(c);
				*(dbrk++) = d;
				if ((d == 0xD) && (*url == '%')	// special case: CRLF
				    &&(*(url + 1) == '0') && (*(url + 2) == 'A')) {
					*(dbrk - 1) = '\n';
					url += 3;
				}
				break;
			}
		case '+':	// substitute ' ' for '+'
			*(dbrk++) = ' ';
			break;
		case '=':	// name/value separator
			*(dbrk++) = '\0';	// null-terminate the name
			*(vtp++) = dbrk;	// append ptr to next value field
			break;
		case '&':	// pair separator
			*(dbrk++) = '\0';	// null-terminate the value
			*(vtp++) = dbrk;	// append ptr to next name field
			break;
		default:
			*(dbrk++) = c;
			break;
		}
	}
	*dbrk = '\0';		// null-terminate the decoded string

	*vtp = NULL;		// NULL-terminate the list
	return table = vartab;
}

/*
 * ------------------------------------------------------------
 * Get encoded string from appropriate place depending on REQUEST_METHOD,
 * then Build table of (name,value) pairs and return address of first entry.
 * Table has form {..., name-i, value-i, ..., NULL}. Return NULL on error.
 */
char **
getcgivars(void)
{
	char *rm;	// REQUEST_METHOD
	char *qs;	// QUERY_STRING
	char *ct;	// CONTENT_TYPE
	char *cl;	// CONTENT_LENGTH
	char *url;	// string to decode
	int contlen;	// CONTENT_LENGTH as int
	int readct;	// # bytes read by fread()

	rm = getenv("REQUEST_METHOD");
	if (rm == NULL) {
		return NULL;
	}
	if (!strcmp(rm, "GET")) {
		if ((qs = getenv("QUERY_STRING")) == NULL) {
			return NULL;
		}
		if ((url = strdup(qs)) == NULL) {
			return NULL;
		}
	} else if (!strcmp(rm, "POST")) {
		ct = getenv("CONTENT_TYPE");
		if (!ct || strcasecmp(ct, "application/x-www-form-urlencoded")) {
			return NULL;
		}
		if ((cl = getenv("CONTENT_LENGTH")) == NULL) {
			return NULL;
		}
		contlen = atoi(cl);
		if ((url = (char *) malloc(sizeof(char) * (contlen + 1))) == NULL) {
			return NULL;
		}
		if ((readct = fread(url, 1, contlen, stdin)) != contlen) {
			return NULL;
		}
		url[contlen] = '\0';
	} else {
		return NULL;
	}
	return urldecode(url);
}

/*
 * ------------------------------------------------------------
 * Get first value for specified name. If called with an argument
 * of NULL, will search for next occurrence of the name.
 */
char *
cgival(char *name)
{
	char **tp;

	if (table == NULL) {
		return NULL;
	}
	if (name == NULL) {	// caller specified continued search?
		if (savedtp == NULL) {
			return NULL;
		}
		name = *savedtp;	// use last name found
		tp = savedtp + 2;	// start search at next name
	} else
		tp = table;	// if new search, go to start of table

	for (; *tp; tp += 2)
		if (!strcmp(*tp, name)) {	// search succeeds
			savedtp = tp;
			return *(tp + 1);
		}
	savedtp = NULL;		// search fails
	return NULL;
}
