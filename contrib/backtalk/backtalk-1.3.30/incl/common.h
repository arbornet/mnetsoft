/* Copyright 1996-1999, Jan D. Wolter and Steven R. Weiss, All Rights Reserved.
 */

#ifndef _BT_COMMON_H
#define _BT_COMMON_H

#include <stdio.h>
#include "config.h"
#include "version.h"
#include <sys/types.h>
/*#include <utmp.h>*/

#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#else
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif
char *strchr(), *strdup(), *strrchr(), *strstr(), *getenv();
void *malloc(), *realloc();
#endif

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef DMALLOC
#define CLEANUP
#endif

#define DFLT_CFLIST BBS_DIR"dflt.cflist"
#define CONFLIST BBS_DIR"conflist"
#define CONFINDEX BBS_DIR"confindex"
#define CENSOR_LOG BBS_DIR"censored"
#define IDENT_FILE "id"

#define INDEX_SUBDIR "indexdir"
#define INDEX_PREFIX "@"

#define REPEAT_FILE "/tmp/bt.rep"

#define ATTACH_DIR BBS_DIR"attach/"

#define MAXSYMLEN  32	/* max length of symbol names in internal dictionary */
#define BFSZ 1024	/* standard buffer size for various uses */

/* If using real unix logins, use real login name length */
#ifdef UNIX_ACCOUNTS
#define ALLOW_CFDIR
#ifdef UT_NAMELEN
#ifdef MAX_LOGIN_LEN
#undef MAX_LOGIN_LEN
#endif
#define MAX_LOGIN_LEN UT_NAMELEN
#endif
#endif

#ifdef PICOSPAN_COMPAT
#define HIDE_HTML
#define DEFAULT_PART_TYPE 1
#else
#define PARENT_POINTERS
#endif

#ifdef YAPP_COMPAT
#define HIDE_HTML
#define KEEP_ULIST
#define CONF_LOG
#define DEFAULT_PART_TYPE 1
#endif

#ifdef YAPP3_COMPAT
#define YAPP_COMPAT
#define KEEP_ULIST
#define CONF_LOG
#define PART_DIR BBS_DIR"part/"
#define PART_DIR_LEVEL 2	/* 0, 1 or 2 only */
#define DEFAULT_PART_TYPE 1
#ifdef UNIX_ACCOUNTS
#define HIDE_HTML
#else
#ifdef MAX_LOGIN_LEN
#undef MAX_LOGIN_LEN
#endif
#define HTTPD_OWNS_HOMEDIR
#define CFADM_OWNS_AUTH
#define MAX_LOGIN_LEN 20
#define MIN_LOGIN_LEN 2
#define MIXED_CASE_LOGINS
#endif
#endif

#ifndef DEFAULT_PART_TYPE
#define DEFAULT_PART_TYPE 2
#define CONVERT_PART		/* convert other types of part files to new */
#endif

#if defined(SQL_PGSQL) || defined(SQL_MYSQL) || defined(SQL_MSQL) || defined(SQL_ORACLE) || defined(SQL_SYBASE)
#define USE_SQL
#endif

#ifdef EDITUSER
# define NOPWEDIT 0
#else
# define NOPWEDIT 1
#endif

#ifdef SHARE_AUTH_IDENT

#ifdef PASSWD_FILE
#define AUTH_FILE PASSWD_FILE
#endif

#ifdef DBM_NAM_FILE
#define AUTH_DBM DBM_NAM_FILE
#endif

#endif

#define DFLT_FW "cfadm"

#ifdef ATTACH_SQL
#define ATTACHMENTS 1
#endif

#ifdef ATTACH_HASH
#define ATTACHMENTS 1
#define ATTACH_INDEX_HASH ATTACH_DIR"index"
#endif

#ifdef ATTACHMENTS
#define ATTACH_DESC_LEN 800	/* Maximum length of attachment description */
#else
#define ATTACHMENTS 0
#endif

#if defined(SENDMAIL_PATH) || defined(SMTP)
#define EMAIL 1
#else
#define EMAIL 0
#endif

/* Choose a source of entropy */

#ifdef HAVE_DEV_URANDOM
/* Prefer urandom */
#define NOISE_DEV "/dev/urandom"
#undef NOISE_FILE
#else
#ifdef HAVE_DEV_SRANDOM
/* srandom may block, but our reads are small so it probably won't */
#define NOISE_DEV "/dev/srandom"
#undef NOISE_FILE
#else
#ifdef HAVE_DEV_RANDOM
/* random is similar to srandom, but srandom is more suitable on machines
 * that have both */
#define NOISE_DEV "/dev/random"
#undef NOISE_FILE
#else
/* With no random device, we maintain our own entropy pool, in a file or
 * in the SQL database.  Except the SQL version hasn't been implemented,
 * so we always use the file.
 */
/* #ifdef USE_SQL
   #define NOISE_SQL
   #undef NOISE_FILE
   #endif
*/
#ifdef ID_SESSION
/* make_noise() is only used if we have entropy pool and are using sessions */
#define MAKE_NOISE
#endif /* ID_SESSION */
#endif /* ! HAVE_DEV_RANDOM */
#endif /* ! HAVE_DEV_SRANDOM */
#endif /* ! HAVE_DEV_URANDOM */

/* Standardize some defines that may be missing on systems */

#define TRUE    1
#define FALSE   0

#ifdef _PATH_LASTLOG
#define LASTLOG _PATH_LASTLOG
#else
#define LASTLOG "/usr/adm/lastlog"
#endif

#ifndef UT_NAMESIZE
#define UT_NAMESIZE 8
#endif


#ifdef DEBUG_MEMORY
#define malloc(s) xmalloc(s)
#define realloc(p,s) xrealloc(p,s)
#define free(p) xfree(p)
#define MYSTRDUP
void *xmalloc(size_t);
void *xrealloc(void *,size_t);
void *xfree(void *);
#endif

/* Variadic functions.  Backtalk pretty largely assumes an ANSI C compiler,
 * so we could reasonably get away with just supporting stdarg.h.  The
 * varargs.h support, however, isn't especially in our way, so we've left
 * it in.  The support for systems that have neither has been cut out, since
 * it really did limit us, and probably wouldn't have worked well anyway.
 */

#if __STDC__
#include <stdarg.h>
#define VA_START(args, lastarg) va_start(args, lastarg)
#else
#include <varargs.h>
#define VA_START(args, lastarg) va_start(args)
#endif /* __STDC__ */


/* Snprintf() and friends.  Use Mark Martinec's versions if the operating
 * system doesn't supply suitable ones.  We never use the asnprintf() or
 * vasnprintf() functions defined there, because they depend on va_copy()
 * which we can't find on all systems.
 */

#if !defined(HAVE_SNPRINTF) || !defined(HAVE_VSNPRINTF) || !defined(HAVE_VASPRINTF)

/* The following handles the case of having snprintf() but not vsnprintf(), by
 * causing us to use the Martinec versions of both. */
#ifndef HAVE_VSNPRINTF
#define PREFER_PORTABLE_SNPRINTF
#endif

#ifndef HAVE_VPRINTF
#ifndef HAVE_VASPRINTF
#define NEED_VASPRINTF	/* Might not work, if va_copy is unavailable */
#endif
#endif

#include "snprintf.h"
#endif

#endif /* _BT_COMMON_H */
