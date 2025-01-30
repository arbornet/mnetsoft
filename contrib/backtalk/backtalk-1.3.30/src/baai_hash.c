/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"

#if ATTACHMENTS
#include "baai.h"
#include "str.h"
#include "hashfile.h"

#include <ctype.h>

/* Backtalk Attachment Archive Index - Hash file version
 *
 * The attachment archive index is keyed by the attachment handle (really the
 * filename).  For each attachment, a string like this is stored:
 * 
 *    <link>:<type>:<size>:<user>:<uid>:<conf>:<access>:<date>:<desc>
 *
 * except the delimiter is whatever is defined by DELIM, not a ':'.  Here
 *
 *    <link> = 1 if an item points to this.  0 otherwise
 *    <type> = mime-type of attachment.  Empty string = 'plain/text'.
 *    <size> = size in bytes of attachment.
 *    <user> = login name of user who submitted attachment.
 *    <uid> = uid number of user who submitted attachment.
 *    <conf> = name of conference where attachment was originally posted.
 *    <access> = 0 if accessible by anyone,
 *               1 if accessible by registered users,
 *               2 if accessible by anyone who can read <conf>
 *    <date> = Date attachment submitted - a unix time int
 *    <desc> = Description of attachment.
 */

/* The following must match */
#define DELIM '\001'	    /* Use control-A for delimiter */
#define D "\001"	    /* Use control-A for delimiter */

/* Max field size is not an exact science.  We count it up like this:
 *   colons     8 chars
 *   <linked>   1 char
 *   <type>    30 chars
 *   <size>    10 chars
 *   <user>    MAX_LOGIN_LEN
 *   <uid>     10 chars
 *   <conf>    20 chars
 *   <access>   1 char
 *   <date>    10 chars
 *   <desc>    ATTACH_DESC_LEN
 *   nil        1 char
 *   misc      10 chars
 */
#define ATTACH_BFSZ (100 + MAX_LOGIN_LEN + ATTACH_DESC_LEN)

/* BAAI_PARSE:  Parse apart a line */

int baai_parse(char *bf, int *link, char **type, off_t *size, char **user,
	uid_t *uid, char **conf, int *access, time_t *date, char **desc)
{
    char *b= bf, *e;

    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (link != NULL) *link= atoi(b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (type != NULL) *type= strndup(b, e-b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (size != NULL) *size= atol(b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (user != NULL) *user= strndup(b, e-b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (uid != NULL) *uid= atoi(b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (conf != NULL) *conf= strndup(b, e-b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (access != NULL) *access= atoi(b);

    b= e+1;
    if ((e= strchr(b,DELIM)) == NULL) return 1;
    if (date != NULL) *date= atoi(b);

    b= e+1;
    if (desc != NULL) *desc= strdup(b);
    return 0;
}


/* BAAI_SET - Write an attachment index entry.  Creates a new one if it wasn't
 * there yet, overwrites the old one if it was.  Dies on failure.
 */

void baai_set(char *fname, int link, char *type, off_t size, char *user,
	uid_t uid, char *conf, int access, time_t date, char *desc)
{
    char bf[ATTACH_BFSZ];
    char *p;

    /* Delete various delimiters */
    while ((p= strchr(type,DELIM)) != NULL) *p= ' ';
    while ((p= strchr(user,DELIM)) != NULL) *p= ' ';
    while ((p= strchr(conf,DELIM)) != NULL) *p= ' ';
    while ((p= strchr(desc,DELIM)) != NULL) *p= ' ';

    /* Construct the data line - there is some small risk of truncating part
     * of the description, but that is OK with us
     */
    snprintf(bf,ATTACH_BFSZ,"%d"D"%s"D"%ld"D"%s"D"%d"D"%s"D"%d"D"%ld"D"%s",
	    link, type, (long)size, user, uid, conf, access, date, desc);

    if (storedbm(ATTACH_INDEX_HASH, fname, strlen(fname), bf, strlen(bf), TRUE))
	die("Cannot write to attachment index ("ATTACH_INDEX_HASH"): %s",
		errdbm());
}


/* BAAI_GET - Get values associated with a key.  Returns -1 if not found.
 * String value are in dynamic memory.  Any of the arguments can be NULL if
 * we don't want them.
 */

int baai_get(char *fname, int *link, char **type, off_t *size, char **user,
	uid_t *uid, char **conf, int *access, time_t *date, char **desc)
{
    char bf[ATTACH_BFSZ];
    int rc;

    /* Get value */
    rc= fetchdbm(ATTACH_INDEX_HASH, fname, strlen(fname), bf, ATTACH_BFSZ);
    if (rc > 0) die("Cannot read attachment index ("ATTACH_INDEX_HASH")");
    if (rc < 0) return -1;

    if (baai_parse(bf,link,type,size,user,uid,conf,access,date,desc))
	die("Bad format in attachment index entry for '%s': %s", fname, bf);

    return 0;
}


/* BAAI_DEL - Delete an attachment index entry.  Returns -1 if key was not
 * found.  Dies for other errors.
 */

int baai_del(char *fname)
{
    int rc= deletedbm(ATTACH_INDEX_HASH, fname, strlen(fname), FALSE);
    if (rc > 0) die("Cannot open attachment index ("ATTACH_INDEX_HASH"): %s",
		    errdbm());
    return (rc < 0) ? -1 : 0;
}


/* BAAI_LINK - Set link field to 1.
 */

int baai_link(char *fname)
{
    char bf[ATTACH_BFSZ];
    int rc;

    /* Get value */
    rc= fetchdbm(ATTACH_INDEX_HASH, fname, strlen(fname), bf, ATTACH_BFSZ);
    if (rc > 0) die("Cannot read attachment index ("ATTACH_INDEX_HASH"): %s",
	    errdbm());
    if (rc < 0) return -1;

    if (bf[0] == '0')
    {
	bf[0]= '1';
	if (storedbm(ATTACH_INDEX_HASH, fname, strlen(fname),
		    bf, strlen(bf), TRUE))
	    die("Cannot write to attachment index ("ATTACH_INDEX_HASH"): %s",
		    errdbm);
    }
}


/* BAAI_WALK() - Get first or next attachment index entry.
 */

int baai_walk(int next, char **fname, int *link, char **type, off_t *size,
	char **user, uid_t *uid, char **conf, int *access, time_t *date,
	char **desc)
{
    char bf[ATTACH_BFSZ];
    char key[BFSZ];

    if (!next)
    {
	if (walkdbm(ATTACH_INDEX_HASH))
	    die("Cannot open attachment index ("ATTACH_INDEX_HASH"): %s",
		    errdbm);

	if (firstdbm(key,BFSZ)) return 0;
    }
    else
    {
	if (nextdbm(key,BFSZ)) return 0;
    }
    baai_get(key,link,type,size,user,uid,conf,access,date,desc);
    *fname= strdup(key);

    return 1;
}

#endif /* ATTACHMENTS */
