/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#ifndef _BT_BACKTALK_H
#define _BT_BACKTALK_H

#include "common.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#else
char *getenv();
void *malloc(), *realloc();
#endif

#include "../regex/regex.h"

typedef unsigned char byte;
typedef unsigned short word;

/* TOKEN - Data structure used in stack and symbol table to represent a
 * typed token.  We should really use a union for "val" but that would
 * prevent automatic initialization, so we just trust that pointers are
 * no smaller than integers.
 */

typedef struct {
	word flag;	/* various TK_* flags indicating type, etc */
	word line;	/* line no. token comes from (lib ID for TK_DYNAMIC) */
	void *val;	/* Value of token */
	byte sfid;	/* index of source file in file name list */
	} Token;

/* Macros to reference the value of a token under various interpretations    */
#define sval(t)  ((char *)((t).val))				/* string    */
#define ival(t)  ((long)((t).val))				/* integer   */
#define tval(t)  ((time_t)((t).val))				/* time      */
#define fval(t)  ((void (*)())((t).val))			/* function  */
#define aval(t)  ((Array *)((t).val))				/* array     */
#define bval(t)  (((t).flag & TKC_STRING) ? *sval(t) : ival(t))	/* boolean   */
#define hval(t)  ((HashTable *)((t).val))			/* dictionary*/
#define lval(t)  (((t).flag == TK_BOUND_LITERAL) ? sval(t) : sysdict[ival(t)].label)
#define rval(t)	 ((Regex *)((t).val))				/* regexp   */

/* Various token that make up possible data types */
#define TK_CLASS     0x0007	/* data storage class is one of: 	*/
#define TKC_INTEGER  0x0000	/*    integer in ival(t)		*/
#define TKC_TIME     0x0001	/*    time in tval(t)			*/
#define TKC_STRING   0x0002	/*    string in sval(t)			*/
#define TKC_ARRAY    0x0003	/*    array in aval(t)			*/
#define TKC_FUNCTION 0x0004     /*    function pointer in fval(t)	*/
#define TKC_REGEX    0x0005     /*    compiled regular expr. in rval(t)	*/
#define TKC_DICT     0x0006     /*    hashtable pointer in hval(t)	*/
#define TKC_NONE     0x0007     /*    undefined				*/

#define TK_TYPE      0x007F	/* type flags:				*/
#define TKT_SYMBOL   0x0008	/*   is a symbol or literal		*/
#define TKT_MARK     0x0010	/*   is a mark				*/
#define TKT_CONSTANT 0x0020	/*   constant (not executable)		*/
#define TKT_DYNAMIC  0x0040	/*   is dynamic function   		*/

#define TK_FLAGS     0x0080	/* run-time modifiers:		        */
#define TKF_FREE     0x0080     /* should we free memory when done?	*/

/* All possible data types:  may appear in S=Stack/D=Dictionary/P=Program */

#define TK_INTEGER	   (TKC_INTEGER|TKT_CONSTANT)		 /* SDP */
#define TK_BOUND_SYMBOL	   (TKC_INTEGER|TKT_SYMBOL)		 /*   P */
#define TK_BOUND_LITERAL   (TKC_INTEGER|TKT_SYMBOL|TKT_CONSTANT) /* S P */
#define TK_MARK		   (TKC_INTEGER|TKT_MARK|TKT_CONSTANT)   /* SD  */

#define TK_TIME		   (TKC_TIME|TKT_CONSTANT)		 /* SDP */

#define TK_STRING	   (TKC_STRING|TKT_CONSTANT)             /* SDP */
#define TK_UNBOUND_SYMBOL  (TKC_STRING|TKT_SYMBOL)               /*   P */
#define TK_UNBOUND_LITERAL (TKC_STRING|TKT_SYMBOL|TKT_CONSTANT)  /* S P */

#define TK_ARRAY	   (TKC_ARRAY|TKT_CONSTANT)		 /* SD  */
#define TK_PROCEDURE	   (TKC_ARRAY)				 /* S P */

#define TK_FUNCTION 	   (TKC_FUNCTION)			 /*  D  */
#define TK_DYNAMIC 	   (TKC_FUNCTION|TKT_DYNAMIC)		 /*  D  */

#define TK_REGEX	   (TKC_REGEX|TKT_CONSTANT)		 /* SDP */

#define TK_DICT		   (TKC_DICT)				 /* SD  */

#define TK_UNDEF	   (TKC_NONE)				 /*  D  */

#define class(t) ((t).flag&TK_CLASS)	/* returns the TKC_* value */
#define type(t)  ((t).flag&TK_TYPE)	/* returns the TK_* value */

/* DICTIONARIES - Just hash tables.  */

#include "hash.h"

#define DT_PROTECT  0x0001      /* May be redefined only by config.bt */
#define DT_CONSTANT 0x0002      /* Ditto, plus has same value every run */
#define DT_MULTI    0x0004	/* May be set more than once in query */

/* The system dictionary is a weird hash - we store the entries in a static
 * array instead of malloced memory.  This is OK because we never add or
 * delete its keys.  It's is all built at compile time.
 */

#include "hash.h"
extern HashTable syshash;
extern HashEntry sysdict[];

/* These macros are for use with the VAR_* defines in sysdict.h and allow
 * direct access to the value of any system dictionary value
 */
#define sdict(i) sval(sysdict[i].t)
#define idict(i) ival(sysdict[i].t)
#define tdict(i) tval(sysdict[i].t)
#define fdict(i) fval(sysdict[i].t)
#define bdict(i) bval(sysdict[i].t)
#define rdict(i) rval(sysdict[i].t)

/* This macro sets a VAR_* value of the integer type to an integer value.
 * There is also a sets(), but its a function.
 */

#define seti(i,v) (long)(sysdict[i].t.val= (void *)(long)(v))
#define sett(i,v) (time_t)(sysdict[i].t.val= (void *)(v))

/* ARRAYS - This is used to store arrays.  Just a link counter, a size and a
 * pointer to an array of tokens.
 */

typedef struct {
	int lk;		/* Number of variables or stack slots pointing here */
	int sz;		/* size of the array */
	Token *a;
	} Array;

/* REGULAR EXPRESSIONS - This is used to store compiled regular expressions.
 * We add a link counter to the usual structure.
 */

typedef struct {
	int lk;		/* Number of variables or stack slots pointing here */
	regex_t re;	/* Compiled regular expression */
	} Regex;

/* When we save a string, we need to indicate how it's memory is to be managed.
 */

#define SM_COPY 0	/* Save a copy, not this one */
#define SM_FREE 1	/* Use this copy, but free() it when you are done */
#define SM_KEEP 2	/* Use this copy, but don't ever free() it */

#define xor(a,b) ((!(a)) != (!(b)))

/************************** GLOBAL VARIABLES **************************/

extern int initializing;	/* May we write protected variables? */
extern int ttymode;		/* Not running as cgi program */
extern int httpd_uid, cfadm_uid;/* Read and effective uid numbers */
extern FILE *output;		/* Where to write script output */
extern int direct_exec;		/* Determine user by getuid() */

#if __STDC__
void die(const char *, ...);
#else
void die();
#endif /* __STDC__ */
void die_noscript(char *);

#endif /* _BT_BACKTALK_H */
