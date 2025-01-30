/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include "stack.h"
#include "free.h"
#include "dict.h"
#include "str.h"
#include "print.h"
#include "printtoken.h"
#include "token.h"
#include "sysdict.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* ARGUMENT STACK - The stack is simply an array of Tokens.  The <top> variable
 * gives the index of the first unused slot on the stack.  The size of the
 * array is given by stacksize, while top points to the lowest-numbered empty
 * position of the stack.
 *
 * We use a regular Token structure here.  All tokens on the stack should
 * have TKF_FREE set (at least those for which it is meaningful, like
 * strings and unbound literals).
 */

Token *stack= NULL;
int stacksize= 0;
int top= 0;

/* The stack is initially allocated to size STACKUNIT, and is realloc()ed
 * another STACKUNIT blocks larger everytime it overflows.  This means that
 * pointers into the stack should not be expected to remain valid after a
 * push.  Indexing is generally prefered.  Once enlarged, the stack never
 * shrinks again.
 */

#define STACKUNIT 50


/* GROW_STACK - Make sure the has room for n more elements.
 */

void grow_stack(int n)
{
    if (idict(VAR_STACK_LIMIT) > 0 && top + n > idict(VAR_STACK_LIMIT))
	die("Operand stack size exceeded stack_limit (%d)",
	    idict(VAR_STACK_LIMIT));

    while (top + n > stacksize)
    {
	stacksize+= STACKUNIT;
	if (stack == NULL)
	    stack= (Token *)malloc(stacksize*sizeof(Token));
	else
	    stack= (Token *)realloc(stack,stacksize*sizeof(Token));
    }
}


#ifdef CLEANUP
/* FREE_STACK - Deallocate the stack.  Used only when testing for memory
 * leaks.  You should make sure the stack is empty by calling func_print() or
 * func_clear() first.
 */
void free_stack()
{
    free(stack);
    stack=NULL;
}
#endif /*CLEANUP*/


/* PUSH_TOKEN - Push a token on the stack.  If copy is true, malloc new
 * storage for string values and copy them over instead of sharing them.
 *
 * Note that copy should *always* be true unless the original value was
 * (1) popped off the stack, or (2) uses malloced memory and is never
 * going to be referenced again.  All string tokens on the stack should
 * have TKF_FREE set.
 */

void push_token(Token *t, int copy)
{
    grow_stack(1);
    stack[top]= *t;

    if (class(*t) == TKC_STRING || class(*t) == TKC_ARRAY ||
	class(*t) == TKC_REGEX || class(*t) == TKC_DICT)
    {
	stack[top].flag|= TKF_FREE;
	if (copy) uniquify_token(&(stack[top]));
    }
    top++;
}


/* PUSH_INTEGER - Push an integer value on the stack.
 */

void push_integer(long n)
{
    grow_stack(1);
    stack[top].flag= TK_INTEGER;
    stack[top].val= (void *)n;
    top++;
}


/* PUSH_TIME - Push an time value on the stack.
 */

void push_time(time_t n)
{
    grow_stack(1);
    stack[top].flag= TK_TIME;
    stack[top].val= (void *)n;
    top++;
}


/* PUSH_STRING - Push a string value on the stack.  If copy is true, we
 * put a copy of the string passed in on the stack (that is, we strdup it).
 * If it isn't true, then the string passed in should be in malloced memory,
 * which will be freed when it is popped.
 */

void push_string(char *s, int copy)
{
    grow_stack(1);
    stack[top].flag= TK_STRING|TKF_FREE;
    stack[top].val= (void *)s;
    if (copy) uniquify_token(&(stack[top]));
    top++;
}


/* PUSH_PROC - Push a procedure value on the stack.
 */

void push_proc(Array *p, int copy)
{
    grow_stack(1);
    stack[top].flag= TK_PROCEDURE;
    stack[top].val= (void *)p;
    if (copy) uniquify_token(&(stack[top]));
    top++;
}


/* PUSH_ARRAY - Push an array value on the stack.
 */

void push_array(Array *a, int copy)
{
    grow_stack(1);
    stack[top].flag= TK_ARRAY;
    stack[top].val= (void *)a;
    if (copy) uniquify_token(&(stack[top]));
    top++;
}


/* PUSH_REGEX - Push a regular expression value on the stack.
 */

void push_regex(Regex *r, int copy)
{
    grow_stack(1);
    stack[top].flag= TK_REGEX;
    stack[top].val= (void *)r;
    if (copy) uniquify_token(&(stack[top]));
    top++;
}

/* PUSH_DICT - Push a hashtable value on the stack.
 */

void push_dict(HashTable *h, int copy)
{
    grow_stack(1);
    stack[top].flag= TK_DICT;
    stack[top].val= (void *)h;
    if (copy) uniquify_token(&(stack[top]));
    top++;
}


/* PUSH_LITERAL - Push a literal value on the stack.
 */

void push_literal(char *s, int copy)
{
    grow_stack(1);
    stack[top].flag= TK_UNBOUND_LITERAL|TKF_FREE;
    stack[top].val= (void *)s;
    if (copy) uniquify_token(&(stack[top]));
    top++;
}


/* PUSH_BOUND_LITERAL - Push a bound literal value on the stack
 */

void push_bound_literal(long offset)
{
    grow_stack(1);
    stack[top].flag= TK_BOUND_LITERAL;
    stack[top].val= (void *)offset;
    top++;
}


/* POP_ANY - Pop a token of any type off the stack.  It is an error if the
 * stack is empty.  If it happens to be a string type or array, it is the
 * caller's responsibility to see to it that its memory eventually gets
 * freed (by calling free_val).
 */

void pop_any(Token *t)
{
    if (top <= 0)
	die("Stack Underflow");

    *t= stack[--top];
}


/* POP_STRING - Pop a string type off the stack.  It is an error if the
 * stack is empty, or the top stack element is not a string.  It is the
 * caller's responsibility to free the returned string.
 */

char *pop_string()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_STRING)
	die("Expected String Argument");
    top--;

    return sval(stack[top]);
}


/* POP_PROC - Pop a procedure type off the top of the stack.  It is an
 * error if the stack is empty, or the top stack element is not an procedure.
 * It is the callers responsibility to free the procedure array when he is
 * done with it (by calling free_array).
 */

Array *pop_proc()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_PROCEDURE)
	die("Expected Procedure Argument");
    top--;

    return aval(stack[top]);
}


/* POP_LITERAL - Pop a bound or unbound literal token off the stack.  It is
 * an error if the stack is empty, or the top stack element is not a literal.
 * It is the caller's responsibility to free memory used in unbound literals.
 * Returns true if the symbol was unbound.
 */

int pop_literal(Token *t)
{
    int rc;

    if (top <= 0)
	die("Stack Underflow");

    if (type(stack[top-1]) == TK_UNBOUND_LITERAL)
	rc= TRUE;
    else if (type(stack[top-1]) == TK_BOUND_LITERAL)
	rc= FALSE;
    else
	die("Expected Literal Argument");
    top--;

    *t= stack[top];

    return rc;
}


/* POP_LITNAME - Pop a string or a bound or unbound literal token off the
 * stack.  It is an error if the stack is empty, or the top stack element is
 * not a literal or a string.  A pointer to the string is return.  It is the
 * caller's responsibility to free it.
 */

char* pop_litname()
{
    if (top <= 0)
	die("Stack Underflow");

    top--;
    if (type(stack[top]) == TK_UNBOUND_LITERAL ||
	type(stack[top]) == TK_STRING)
    {
	return sval(stack[top]);
    }
    else if (type(stack[top]) == TK_BOUND_LITERAL)
    {
	return strdup(sysdict[ival(stack[top])].key);
    }
    else
	die("Expected Literal or String Argument");
}


/* POP_INTEGER - Pop an integer type off the stack.  It is an error if the
 * stack is empty, or the top stack element is not an integer.
 */

long pop_integer()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_INTEGER)
	die("Expected Integer Argument");
    top--;

    return ival(stack[top]);
}


/* POP_TIME - Pop a time type off the stack.  It is an error if the
 * stack is empty, or the top stack element is not a time or an integer zero.
 */

time_t pop_time()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_TIME &&
	(type(stack[top-1]) != TK_INTEGER || ival(stack[top-1]) != 0))
	die("Expected Time Argument");
    top--;

    return tval(stack[top]);
}


/* POP_BOOLEAN - Pop an integer, time or string type off the stack, returning
 * false if it is the integer 0 or the string {}, and true otherwise.  It is
 * an error if the stack is empty.
 */

int pop_boolean()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) == TK_INTEGER)
	return ival(stack[--top]) != 0;
    if (type(stack[top-1]) == TK_TIME)
	return tval(stack[--top]) != 0;
    if (type(stack[top-1]) == TK_STRING)
    {
	char *s= sval(stack[--top]);
	int rc= s[0];
	free(s);
	return rc;
    }
    return TRUE;
}


/* POP_ARRAY - Pop an array type off the stack.  It is an error if the
 * stack is empty, or the top stack element is not an array.  It is the
 * caller's responsibility to free the returned array (by calling free_array).
 */

Array *pop_array()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_ARRAY)
	die("Expected Array Argument");
    top--;

    return aval(stack[top]);
}


/* POP_REGEX - Pop a regular expression type off the stack.  It is an error
 * if the stack is empty, or the top stack element is not a regular expression.
 * It is the caller's responsibility to free the returned regular expression
 * (by calling free_regex).
 */

Regex *pop_regex()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_REGEX)
	die("Expected Regular Expression Argument");
    top--;

    return rval(stack[top]);
}


/* POP_DICT - Pop a dictionary type off the stack.  It is an error
 * if the stack is empty, or the top stack element is not a dictionary.
 * It is the caller's responsibility to free the returned dictionary
 * (by calling free_dict).
 */

HashTable *pop_dict()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_DICT)
	die("Expected Dictionary Argument");
    top--;

    return hval(stack[top]);
}


/* PEEK_ANY - Return the top stack token of any type.  It is an error if the
 * stack is empty.
 */

void peek_any(Token *t)
{
    if (top <= 0)
	die("Stack Underflow");

    *t= stack[top-1];
}


/* PEEK_INTEGER - Return the integer off the top of the stack.  It is an error
 * if the stack is empty, or the top stack element is not an integer.
 */

long peek_integer()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_INTEGER)
	die("Expected Integer Argument");

    return ival(stack[top-1]);
}


#ifdef UNUSED
/* PEEK_TIME - Return the time off the top of the stack.  It is an error
 * if the stack is empty, or the top stack element is not a time.
 */

time_t peek_time()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_TIME)
	die("Expected Time Argument");

    return tval(stack[top-1]);
}
#endif


/* PEEK_STRING - Return the string off the top of the stack.  It is an error
 * if the stack is empty, or the top stack element is not an string.
 */

char *peek_string()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_STRING)
	die("Expected String Argument");

    return sval(stack[top-1]);
}


#ifdef UNUSED
/* PEEK_PROC - Return the procedure off the top of the stack.  It is an error
 * if the stack is empty, or the top stack element is not an procedure.
 */

Array *peek_proc()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_PROCEDURE)
	die("Expected Procedure Argument");

    return aval(stack[top-1]);
}


/* PEEK_LITERAL - Return the literal off the top of the stack.  It is an error
 * if the stack is empty, or the top stack element is not an literal.
 */

char *peek_literal()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) != TK_LITERAL)
	die("Expected Literal Argument");

    return sval(stack[top-1]);
}


/* PEEK_BOOLEAN - check the integer or string on top of the stack, returning
 * false if it is the integer 0 or the string {}, and true otherwise.  It is
 * an error if the stack is empty.
 */

int peek_boolean()
{
    if (top <= 0)
	die("Stack Underflow");
    if (type(stack[top-1]) == TK_INTEGER)
	return ival(stack[top]) != 0;
    if (type(stack[top-1]) == TK_TIME)
	return tval(stack[top]) != 0;
    if (type(stack[top-1]) == TK_STRING)
	return sval(stack[top])[0];
    return TRUE;
}
#endif /* UNUSED */


/* REPL_TOP - Replace the value of the top stack element with the given value.
 * It is presumed that you know the old type and are replacing it with
 * something of the same type.  If you are replacing a string, it is assumed
 * that you have freed the old value.  No error checking here.
 */

void repl_top(void *val)
{
    stack[top-1].val= val;
}


/* UNIQUIFY_TOKEN - Replace the value of a token with a copy of the same
 * in freshly allocated memory.  For arrays, dicts, and regular expressions,
 * this just increments the link count.  It should do so for strings, but
 * we haven't got a link count for strings (yet).
 */

void uniquify_token(Token *t)
{
    if (class(*t) == TKC_STRING)
	t->val= (void *)strdup(sval(*t));
    else if (class(*t) == TKC_ARRAY)
    {
	aval(*t)->lk++;

	/* Old code to REALLY uniqify an array:
	 * Array *a= (Array *)malloc(sizeof(Array));
	 * a->sz= aval(*t)->sz;
	 * a->a= (Token *)malloc(a->sz * sizeof(Token));
	 * memmove(a->a, aval(*t)->a, a->sz * sizeof(Token));
	 * t->val= (void *)a;
	 */
    }
    else if (class(*t) == TKC_REGEX)
    {
	rval(*t)->lk++;
    }
    else if (class(*t) == TKC_DICT)
    {
	hval(*t)->lk++;
    }
}


/* MAKE_ARRAY - Collapse elements from the mark up the top of the stack into
 * an array and push the array on the stack.  If code is 0, this is a regular
 * array, if it is 1, this is an executable array (a procedure).  Return 0 on
 * success, 1 if there is no mark with the appropriate code on the stack.
 */

int make_array(int code)
{
    int i,n;
    Token t;

    /* Count elements */
    for (i= top-1; i >= 0; i--)
	if (type(stack[i]) == TK_MARK && ival(stack[i]) == code)
	    break;

    if (i < 0) return 1;

    /* Create array */
    t.flag= (code ? TK_PROCEDURE : TK_ARRAY);
    t.val= (void *)malloc(sizeof(Array));
    aval(t)->lk= 1;
    n= aval(t)->sz= top-1 - i;
    aval(t)->a= (n == 0) ? NULL : (Token *)malloc(n * sizeof(Token));

    /* Copy stack elements into array */
    for (i= n-1; i >= 0; i--)
	pop_any(&(aval(t)->a[i]));
    
    /* discard mark */
    top--;

    push_token(&t,FALSE);	/* Push array */

    return 0;
}

/* PUSH_ARRAY_MARK - Put start-of-array mark on the stack.  This is used with
 * make_array() by various functions that need to generate arrays as output.
 */

int push_array_mark()
{
    Token mark;
    mark.flag= TK_MARK;
    mark.val= (void *)0;
    mark.line= mark.sfid= 0;
    push_token(&mark,TRUE);
}


/* FUNC_POP:  <any> pop -
 * Discard the top element of the stack.
 */

void func_pop()
{
    if (top <= 0)
	die("Stack Underflow");
    top--;
    free_val(&(stack[top]));
}


/* FUNC_CLEAR:  |- <any1> ... <anyn> clear |-
 * Empty the stack.
 */

void func_clear()
{
    while (top > 0)
	func_pop();
}


/* FUNC_CLEARTOMARK:  mark <any1> ... <anyn> cleartomark
 * Delete all stack elements up to the mark.
 */

void func_cleartomark()
{
    int i;

    for (i= top-1; i >= 0; i--)
    {
	if (type(stack[i]) == TK_MARK)
	{
	    while (top > i)
		func_pop();
	    return;
	}
    }
    die("No matching mark");
}


/* FUNC_EXCH:  <any1> <any2> exch <any2> <any1>
 * Swap the top two elements on the stack.
 */

void func_exch()
{
    Token t;

    if (top < 2)
	die("Stack Underflow");
    
    t= stack[top-1];
    stack[top-1]= stack[top-2];
    stack[top-2]= t;
}


/* FUNC_DUP:  <any> dup <any> <any>
 * Duplicate the top stack element.
 */

void func_dup()
{
    grow_stack(1);
    peek_any(&stack[top]);
    uniquify_token(&stack[top]);
    top++;
}


/* FUNC_COPY:  <any1>...<anyn> <n> copy <any1>...<anyn> <any1>...<anyn>
 * Duplicate the top n stack elements.
 */

void func_copy()
{
    long n= pop_integer();
    int i,j;

    if (top < n) die("Stack Underflow");
    if (n < 0) die("Number of elements must be positive");
    grow_stack(n);

    for (i= 0; i < n; i++)
    {
	stack[j= top+i]= stack[top-n+i];
	uniquify_token(&(stack[j]));
    }
    top+= n;
}


/* GCD - Return the Greatest Common Divisor of two integers, using Euclids
 * algorithm.  Always nice to have world's oldest algorithm in your program,
 * even if it isn't the fastest known gcd algorithm.  Note that by definition,
 * gcd(0,x) == x.
 */

int gcd(int u, int v)
{
    if (u < 0) u= -u;
    if (v < 0) v= -v;

    while (v != 0)
	{ int r= u % v; u= v; v= r; }
    return u;
}


/* FUNC_ROLL: <a_n-1>...<a_0> <n> <j> roll <a_(j-1) mod n>...<a_0>
 *					       <a_n-1>...<a_j mod n>
 * Roll <n> elements up j times.  j may be negative, in which case we
 * roll down instead (or really, roll up n-j times).
 */

void func_roll()
{
    int j= pop_integer();
    int n= pop_integer();
    int d,k,i;
    Token *st= stack+top-1;
    int start;
    Token tmp;

    if (n < 0) die("Number of elements must be positive");
    if (n > top) die("Not that many elements on stack");
    if (n == 0) return;

    j= (j+n)%n;		/* This gives us a positive j between 0 and n */
    d= gcd(n,j);	/* Number of cycles formed */

    for (start= 0; start < d; start++)
    {
	i= start;
	tmp= st[-i];
	for (;;)
	{
	    k= (i+j) % n;
	    if (k == start) break;
	    st[-i]= st[-k];
	    i= k;
	}
	st[-i]= tmp;
    }
}


/* FUNC_INDEX:  <anyn>...<any0> <n> index <anyn>...<any0> <anyn>
 * Duplicate arbitrary stack element.
 */

void func_index()
{
    int n= pop_integer();

    if (top < n+1) die("Stack Underflow");
    if (n < 0) die("Negative Index");

    stack[top]= stack[top-n-1];
    uniquify_token(&(stack[top]));
    top++;
}


/* FUNC_COUNT:  |- <any1>...<anyn> count |- <any1>...<anyn> <n>
 * Push the number of elements on the stack on the stack.  That would be the
 * number before, not the number after.
 */

void func_count()
{
    push_integer(top);
}


/* FUNC_COUNTTOMARK:  mark <any1>...<anyn> counttomark mark <any1>...<anyn> <n>
 * Count all stack elements up to the mark.
 */

void func_counttomark()
{
    int i;

    for (i= top-1; i >= 0; i--)
    {
	if (type(stack[i]) == TK_MARK)
	{
	    push_integer(top-1 - i);
	    return;
	}
    }
    die("No matching mark");
}


/* FUNC_SELECT:  <anyn-1> ... <any1> <any0> <n> <i> select <anyi>
 * Discards the <n> previous things on the stack, except <anyi>.
 * WARNING:  zero-based indexing.
 */

void func_select()
{
    int i= pop_integer();
    int n= pop_integer();
    int j;
    Token t;

    if (n <= 0) die("Count not positive");
    if (i >= n) die("Index not less than count");

    for (j= 0; j < i; j++)
	func_pop();
    
    pop_any(&t);

    for (j++; j < n; j++)
	func_pop();

    push_token(&t,FALSE);
}


/* FUNC_MKARRAY:  mark <any1>...<anyn> ] <array>
 * Collapse elements from the mark up the top of the stack into an array and
 * push the array on the stack.
 */

void func_mkarray()
{
    if (make_array(0)) die("No matching mark");
}


/* DUMP_STACK - Print out a dump of the stack for debugging.  This is non-
 * destructive.
 */

void dump_stack(FILE *fp, int html)
{
    int i;

    if (top <= 0)
    {
	fputs(html?"(empty)<br>\n":"(empty)\n",fp);
	return;
    }
    i= 0;
    for (;;)
    {
	print_token(fp,&stack[i]);
	if (++i < top)
	    fputs(html?"<br>":"\n",fp);
	else
	    break;
    }
    fputs(html?"<hr>\n":"\n",fp);	
}


/* FUNC_PRINT:  |- <anyn> ... <any1> <any0> print |-
 * Prints the entire contents of the stack. Deepest thing first.
 */

void func_print()
{
    int i;

    send_http_headers();

    for (i= 0; i < top; i++)
    {
	print_value(output,stack+i);
	free_val(stack+i);
    }
    top= 0;
}


/* FUNC_PR:  <any> pr -
 * Prints the top stack element.
 */

void func_pr()
{
    Token t;

    send_http_headers();

    pop_any(&t);
    
    print_value(output,&t);
    free_val(&t);
}


/* FUNC_DUMPSTACK:	- dumpstack -
 * Print out snapshot of stack without changing it.
 */

void func_dumpstack()
{
    send_http_headers();

    dump_stack(output,!ttymode);
}


/* FUNC_CONCATINATE:	` <any>... ' <string>
 * Concatinate all elements on the stack down to the topmost type 1 mark
 * together into a single string.  
 */

void func_concatinate()
{
    int i= 0, n= BFSZ;
    char **strings= (char **)malloc(n*sizeof(char *));
    char *new, *p, *q;
    int len= 0;
    Token *t;

    /* First pass - take things off stack, convert to string, and total up
     * their lengths.
     */

    for (;;)
    {
	if (top <= 0)
	    die("No matching left-quote");

	/* Pop an item off the stack and convert it to string */
	t= stack+(--top);

	if (type(*t) == TK_MARK)
	{
	    /* Mark type 1 means we are done.  Ignore others. */
	    if (ival(*t) == 1)
		break;
	    else
		continue;
	}
	else if (class(*t) == TKC_STRING)
	{
	    new= sval(*t);
	}
	else if (class(*t) == TKC_ARRAY)
	{
	    t->flag= TK_ARRAY;	/* Convert procedures into normal arrays */
	    top++;		/* Sort of put it back on the stack */
	    func_aload();	/* Pop it off and put on all its elements */
	    continue;
	}
	else
	{
	    /* We only use token_to_string for Tokens without dynamically
	     * allocated memory, since we can avoid some memory reallocating
	     * in the other cases with the code above, but you actually could
	     * use it for everything.
	     */
	    new= token_to_string(t);
	}

	/* Add it to our collection of strings to concatinate. */
	len+= strlen(new);
	if (i >= n)
	{
	    n= i+BFSZ;
	    strings= (char **)realloc(strings,n*sizeof(char *));
	}
	strings[i++]= new;
    }

    /* Second pass - string them all together */
    p= new= (char *)malloc(len+1);
    for (i--; i >= 0; i--)
    {
    	for (q= strings[i]; *q != '\0'; q++, p++)
	    *p= *q;
    	free(strings[i]);
    }
    *p= '\0';

    free(strings);
    push_string(new,FALSE);
}


/* FUNC_JOINTOMARK:	mark <any>... <delimiter> jointomark <string>
 * Concatinate all elements on the stack down to the topmost type 0 mark
 * together into a single string.  Delete any empty strings, and separate
 * the rest with the given delimiter.
 */

void func_jointomark()
{
    int i= 0, n= BFSZ;
    char **strings= (char **)malloc(n*sizeof(char *));
    char *new, *p, *q;
    Token *t;
    int len= 0;
    char *delim= pop_string();
    int dlen= strlen(delim);

    /* First pass - take things off stack, convert to string, and total up
     * their lengths.
     */

    for (;;)
    {
	if (top <= 0)
	    die("No mark");

	/* Pop an item off the stack and convert it to string */
	t= stack+(--top);

	if (type(*t) == TK_MARK)
	{
	    /* Mark type 0 means we are done.  Ignore others. */
	    if (ival(*t) == 0)
		break;
	    else
		continue;
	}
	else if (class(*t) == TKC_STRING)
	{
	    if (*sval(*t) == '\0')
		continue;
	    else
		new= sval(*t);
	}
	else if (class(*t) == TKC_ARRAY)
	{
	    t->flag= TK_ARRAY;	/* Convert procedures into normal arrays */
	    top++;		/* Sort of put it back on the stack */
	    func_aload();	/* Pop it off and put on all its elements */
	    continue;
	}
	else
	{
	    /* We only use token_to_string for Tokens without dynamically
	     * allocated memory, since we can avoid some memory reallocating
	     * in the other cases with the code above, but you actually could
	     * use it for everything.
	     */
	    new= token_to_string(t);
	}

	/* Add it to our collection of strings to concatinate. */
	len+= strlen(new);
	if (i >= n)
	{
	    n= i+BFSZ;
	    strings= (char **)realloc(strings,n*sizeof(char *));
	}
	strings[i++]= new;
    }

    /* Second pass - string them all together */
    p= new= (char *)malloc(len + (i-1)*dlen + 1);
    for (i--; i >= 0; i--)
    {
    	for (q= strings[i]; *q != '\0'; q++, p++)
	    *p= *q;
    	free(strings[i]);
	if (i > 0)
	    for (q= delim; *q != '\0'; q++, p++)
		*p= *q;
    }
    *p= '\0';

    free(strings);
    push_string(new,FALSE);
}
