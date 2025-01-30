/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include "math.h"
#include "token.h"
#include "stack.h"
#include "free.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int do_cmp(void);


/* FUNC_ADD:  <a> <b> + <a+b>
 * Could be either add two integers, a time and an integer, or do a string
 * concatination.
 */

void func_add()
{
    Token t1;

    pop_any(&t1); 
    if (type(t1) == TK_TIME)
	push_time(pop_integer() + tval(t1));
    else if (type(t1) == TK_INTEGER)
    {
	Token t2;
	pop_any(&t2);
	if (type(t2) == TK_INTEGER)
	    push_integer(ival(t1) + ival(t2));
	else if (type(t2) == TK_TIME)
	    push_time(ival(t1) + tval(t2));
	else
	    die("Bad type for second operand");
    }
    else if (type(t1) == TK_STRING)
    {
	/* Concatinate two strings */
	char *a= peek_string();
	int alen= strlen(a);
	int blen= strlen(sval(t1));
	char *new= (char *)malloc(alen+blen+1);
	strcpy(new,a);
	free(a);
	strcpy(new+alen,sval(t1));
	free(sval(t1));
	repl_top((void *)new);
    }
    else
	die("Bad type for first operand");
}


/* FUNC_SUB:  <a> <b> - <a-b>
 * FUNC_MUL:  <a> <b> * <a*b>
 * FUNC_DIV:  <a> <b> / <a/b>
 * FUNC_MOD:  <a> <b> mod <a%b>
 * FUNC_NEG:  <a> neg <a>
 * These work only for integers.
 */

void func_sub()
{
    Token b,a;
    pop_any(&b);
    pop_any(&a);

    if (type(a) == TK_INTEGER)
    {
	if (type(b) == TK_INTEGER)
	    push_integer(ival(a) - ival(b));
	else
	    die("Bad type for second operand");
    }
    else if (type(a) == TK_TIME)
    {
	if (type(b) == TK_INTEGER)
	    push_time(tval(a) - ival(b));
	else if (type(b) == TK_TIME)
	    push_integer(tval(a) - tval(b));
	else
	    die("Bad type for second operand");
    }
    else
	die("Bad type for first operand");
}

void func_mul()
{
    long b= pop_integer(); 
    repl_top((void *)(peek_integer() * b));
}

void func_div()
{
    long b= pop_integer(); 
    if (b == 0) die("Divide by zero");
    repl_top((void *)(peek_integer() / b));
}

void func_mod()
{
    long b= pop_integer(); 
    if (b == 0) die("Zero modulus");
    repl_top((void *)(peek_integer() % b));
}

void func_neg()
{
    repl_top((void *)(-peek_integer()));
}

void func_abs()
{
    long n= peek_integer();
    if (n < 0) n= -n;
    repl_top((void *)(n));
}


/* FUNC_AND:  <a> <b> and <a/\b>
 * FUNC_OR:   <a> <b> or <a\/b>
 * FUNC_NOT:  <a> ! <a>	
 * These may take integers and strings as inputs, but return integers.
 */

void func_and()
{
    long b= pop_boolean(); 
    push_integer(pop_boolean() && b);
}

void func_or()
{
    long b= pop_boolean(); 
    push_integer(pop_boolean() || b);
}

void func_not()
{
    push_integer((!pop_boolean()));
}


/* FUNC_BAND:  <a> <b> & <a/\b>
 * FUNC_BOR:   <a> <b> | <a\/b>
 * FUNC_BNOT:  <a> ~ <a>	
 * These may take integers as inputs, and do bit-wise operations.
 */

void func_band()
{
    long b= pop_integer(); 
    push_integer(pop_integer() & b);
}

void func_bor()
{
    long b= pop_integer(); 
    push_integer(pop_integer() | b);
}

void func_bnot()
{
    push_integer((~pop_integer()));
}


/* DO_CMP - Pop and compare the top two elements off the stack and return
 * an integer that is >, =, or < zero as the second argument is >, =, or <
 * the first.  Works for strings and integers and times.
 */

int do_cmp()
{
    Token b;
    char *a;
    int rc;

    pop_any(&b); 
    if (type(b) == TK_INTEGER)
	return (pop_integer() - ival(b));
    else if (type(b) == TK_TIME)
	return (pop_time() - tval(b));
    else if (type(b) == TK_STRING)
    {
	a= pop_string();
	rc= strcmp(a, sval(b));
	free(a);
	free(sval(b));
	return rc;
    }
    else if (type(b) == TK_BOUND_LITERAL || type(b) == TK_UNBOUND_LITERAL)
    {
	Token a;
	char *bstr;

	if (type(b) == TK_UNBOUND_LITERAL)
	    bstr= sval(b); 
	else
	    bstr= sysdict[ival(b)].key;

	if (pop_literal(&a))
	{
	    /* a is unbound */
	    rc= strcmp(sval(a),bstr);
	    free(sval(a));
	}
	else
	    /* a is bound */
	    rc= strcmp(sysdict[ival(a)].key,bstr);

	if (type(b) == TK_UNBOUND_LITERAL)
	    free(sval(b));

	return rc;
    }
    else
	die("Bad operand type");
}


/* FUNC_EQ:  <a> <b> eq <bool>
 * FUNC_NE:  <a> <b> ne <bool>
 * FUNC_GT:  <a> <b> gt <bool>
 * FUNC_LT:  <a> <b> lt <bool>
 * FUNC_GE:  <a> <b> ge <bool>
 * FUNC_LE:  <a> <b> le <bool>
 * These work for both strings and integers.
 */

void func_eq()
{
    push_integer(do_cmp() == 0);
}

void func_ne()
{
    push_integer(do_cmp() != 0);
}

void func_gt()
{
    push_integer(do_cmp() > 0);
}

void func_lt()
{
    push_integer(do_cmp() < 0);
}

void func_ge()
{
    push_integer(do_cmp() >= 0);
}

void func_le()
{
    push_integer(do_cmp() <= 0);
}


/* FUNC_CVI:  <any> cvi <integer>
 * Converts a string to an integer (if the stack value is an integer, this
 * is a no-op).  This basically does an atoi() so it just returns zero if
 * the string is not an integer.
 */

void func_cvi()
{
    Token t;

    pop_any(&t);
    if (type(t) == TK_INTEGER)
	push_integer(ival(t));
    else if (type(t) == TK_TIME)
	push_integer((int)tval(t));
    else if (type(t) == TK_STRING)
    {
	push_integer(atoi(sval(t)));
	free(sval(t));
    }
    else
	die("Can only convert strings and times to integer");
}


/* FUNC_CVT:  <any> cvi <date>
 * Converts a string or integer to a date (if the stack value is an date, this
 * is a no-op).  This basically does an atoi() so it just returns zero if
 * the string is not an integer.
 */

void func_cvt()
{
    Token t;

    pop_any(&t);
    if (type(t) == TK_TIME)
	push_time(tval(t));
    else if (type(t) == TK_INTEGER)
	push_time((time_t)ival(t));
    else if (type(t) == TK_STRING)
    {
	push_time((time_t)atoi(sval(t)));
	free(sval(t));
    }
    else
	die("Can only convert strings or integers to dates");
}


/* FUNC_CVS:  <any> cvs <string>
 * Converts a anything to a string (if the stack value is a string, this
 * is a no-op).
 */

void func_cvs()
{
    Token t;
    char *s;

    pop_any(&t);
    if (type(t) == TK_STRING || type(t) == TK_UNBOUND_LITERAL)
	/* Shortcut for efficency, if converting strings to strings */
	push_string(sval(t), FALSE);
    else
    {
	push_string(token_to_string(&t), FALSE);
	free_val(&t);
    }
}


/* FUNC_CVSCOL:  <integer> <numcols> cvscol <string>
 * Converts an integer to a string in a fixed column format (right justified)
 */

void func_cvscol()
{
    char buf[30];
    long n,cols;

    cols= pop_integer();
    n= pop_integer();
    if (cols > 29) cols= 29;
    sprintf(buf,"%*d",cols,n);
    push_string(buf,TRUE);
}
