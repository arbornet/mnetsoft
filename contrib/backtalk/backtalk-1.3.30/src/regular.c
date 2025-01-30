/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Regular expression routines.
 */

#include "backtalk.h"

#include "sysdict.h"
#include "stack.h"
#include "str.h"
#include "regular.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* FUNC_REGEX: <string> <flags> regex <regex> 0
 *             <string> <flags> regex <errormessage> 1
 * Compile a string into a regular expression.  Flags is one of the strings
 * (), (m), (i), (im) or (mi).  (i) indicates that case should be ignored,
 * (m) says that newlines should match ^ and $ characters.
 */

void func_regex()
{
    Regex *reg;
    char *p, *str, *flagstr;
    int flags, err;
    char errmsg[BFSZ];

    flagstr= pop_string();
    str= pop_string();

    flags= REG_EXTENDED;
    for (p= flagstr; *p != '\0'; p++)
    {
    	if (*p == 'i')
	    flags|= REG_ICASE;
	else if (*p == 'm')
	    flags|= REG_NEWLINE;
	else
	    die("bad flag '%c'", *p);
    }
    free(flagstr);

    reg= (Regex *)malloc(sizeof(Regex));
    reg->lk= 1;
    if (err= regcomp(&reg->re, str, flags))
    {
	regerror(err,&reg->re,errmsg,BFSZ);
	push_string(errmsg, TRUE);
	push_integer(1);
	free(str);
	return;
    }
    free(str);
    push_regex(reg,0);
    push_integer(0);
}


/* FUNC_GREP: <string> <regex> grep <bool>
 * This returns non-zero if the given regular expression matches any part of
 * the given string.
 */

void func_grep()
{
    Regex *reg= pop_regex();
    char *txt= pop_string();

    push_integer(!regexec(&(reg->re), txt, 0, (regmatch_t *)NULL, 0));
    free_regex(reg);
    free(txt);
}


/* FUNC_OGREP: <string> <regex> ogrep <array> 1
 *             <string> <regex> ogrep 0
 * This is the same as func_grep, but it returns information about where the
 * match occured, plus information about what each paranthesized subexpression
 * matched.  The returned array has one more element in it than there are
 * pairs of parentheses in the regular expression.  Each element of the
 * array is a two element array, giving an beginning offset and the length
 * of a substring of the original string.  The zeroth pair indicates the
 * portion of the entire matched by the entire regular expression.  The
 * remaining pairs indicate the substrings matched by each set of parenthesis
 * in the regular expression.
 */

void func_ogrep()
{
    Regex *reg= pop_regex();
    char *txt= pop_string();
    regmatch_t *rm;
    int nmatch, rc;
    Token t, *a, *sa;
    int i;

    nmatch= reg->re.re_nsub + 1;
    rm= (regmatch_t *)malloc(nmatch * sizeof(regmatch_t));
    rc= !regexec(&(reg->re), txt, nmatch, rm, 0);
    free_regex(reg);
    free(txt);
    if (rc)
    {
	/* Create the top-level array */
    	t.flag= TK_ARRAY;
	t.val= (void *)malloc(sizeof(Array));
	aval(t)->lk= 1;
	aval(t)->sz= nmatch;
	a= aval(t)->a= (Token *)malloc(nmatch * sizeof(Token));
	for (i= 0; i < nmatch; i++)
	{
	    /* Create the 2-element subarray */
	    a[i].flag= TK_ARRAY;
	    a[i].val= (void *)malloc(sizeof(Array));
	    aval(a[i])->lk= 1;
	    aval(a[i])->sz= 2;
	    sa= aval(a[i])->a= (Token *)malloc(2 * sizeof(Token));

	    /* Save the offset */
	    sa[0].flag= TK_INTEGER;
	    sa[0].val= (void *)(long)rm[i].rm_so;

	    /* Save the length */
	    sa[1].flag= TK_INTEGER;
	    sa[1].val= (void *)(long)(rm[i].rm_eo - rm[i].rm_so);
	}
	push_token(&t,FALSE);
    }
    free(rm);
    push_integer(rc);
}


/* FUNC_SGREP: <string> <regex> sgrep <array> 1
 *             <string> <regex> sgrep 0
 * This is the same as func_sgrep, but instead of returning an array of
 * offset/length arrays, it returns an array the substrings of the original
 * string that match the expression and each subexpression.
 */

void func_sgrep()
{
    Regex *reg= pop_regex();
    char *txt= pop_string();
    regmatch_t *rm;
    int nmatch, rc;
    Token t, *a;
    int i, n;

    nmatch= reg->re.re_nsub + 1;
    rm= (regmatch_t *)malloc(nmatch * sizeof(regmatch_t));
    rc= !regexec(&(reg->re), txt, nmatch, rm, 0);
    free_regex(reg);
    if (rc)
    {
	/* Create the array */
    	t.flag= TK_ARRAY;
	t.val= (void *)malloc(sizeof(Array));
	aval(t)->lk= 1;
	aval(t)->sz= nmatch;
	a= aval(t)->a= (Token *)malloc(nmatch * sizeof(Token));
	for (i= 0; i < nmatch; i++)
	{
	    /* Create a substring token */
	    a[i].flag= TK_STRING|TKF_FREE;
	    n= rm[i].rm_eo - rm[i].rm_so;
	    a[i].val= (void *)malloc(n+1);
	    strncpy(sval(a[i]), txt + rm[i].rm_so, n);
	    sval(a[i])[n]= '\0';
	}
	push_token(&t,FALSE);
    }
    free(txt);
    free(rm);
    push_integer(rc);
}

/* RGREP_STEP(reg, str, &index, nmatch) - Execute a step of the rgrep command.
 * We look for an occurrance of reg in the string str+index.  If none is found, 
 * we return non-zero.  If a match is found we
 *   - push the section of string before the match.
 *   - push the first <nmatch> elements of the array sgrep would have returned.
 *   - set index to point to the first character after the end of the match.
 *   - return zero.
 */

int rgrep_step(Regex *reg, char *str, int *i, int nmatch)
{
    regmatch_t *rm;
    int j,n;
    char *s;

    n= (nmatch < 1) ? 1 : nmatch;
    rm= (regmatch_t *)malloc(n * sizeof(regmatch_t));
    if (regexec(&(reg->re), str + *i, n, rm, 0))
    {
	/* Match failed */
	free(rm);
    	return 1;
    }

    /* Push unmatched characters before the match */
    s= (char *)malloc(rm[0].rm_so+1);
    memmove(s, str + *i, rm[0].rm_so);
    s[rm[0].rm_so]= '\0';
    push_string(s, FALSE);

    /* Push miscellaneous other subexpressions */
    for (j= 0; j < nmatch; j++)
    {
	n= rm[j].rm_eo - rm[j].rm_so;
	s= (void *)malloc(n+1);
	strncpy(s, str + *i + rm[j].rm_so, n);
	s[n]= '\0';
        push_string(s, FALSE);
    }

    /* Point i to the end of the matching substring */
    *i+= rm[0].rm_eo;

    free(rm);
    return 0;
}
