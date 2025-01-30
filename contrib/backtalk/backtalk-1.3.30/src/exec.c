/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <ctype.h>

#include "exec.h"
#include "readcomp.h"
#include "dict.h"
#include "printtoken.h"
#include "stack.h"
#include "free.h"
#include "spell.h"
#include "str.h"
#include "format.h"
#include "sysdict.h"
#include "dynaload.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* EXECUTION STACK:  Each entry on the execution stack describes an execution
 * context.
 *
 * The stack may grow arbitrarily large, since push_exec() realloc()s it when
 * needed.  The stack size may be artificially limited (to trap infinite
 * recursion) by setting the system variable "exec_limit" to a positive value.
 *
 * Using a union here would save a little memory, but isn't worth the bother.
 */

typedef struct estack {
    int flag;		/* Various flags */
    Array *proc;	/* procedure being executed */
    int pc;		/* Current index into proc */

    int i;		/* Current iteration (EX_FOR, EX_FORALL, EX_REPEAT) */
    int inc;		/* Increment (EX_FOR) or nmatch (EX_RGREP) */
    int limit;		/* Termination value (EX_FOR and EX_REPEAT) */

    Token thru;		/* Structure being looped thru (EX_FORALL, EX_RGREP) */
    Regex *reg;		/* Regular expression (EX_RGREP only) */

    } Exec;

#define EX_CALL         0x00000001      /* procedure corresponds to a file */
#define EX_STOPPED	0x00000002	/* "stopped" command */
#define EX_FILE		(EX_CALL|EX_STOPPED)
#define EX_LOOP		0x00000010      /* "loop" command */
#define EX_REPEAT	0x00000020	/* "repeat" command */
#define EX_FOR		0x00000040	/* "for" command */
#define EX_WHILE	0x00000080	/* "while" command */
#define EX_FORALL	0x00000100	/* "forall" command */
#define EX_RGREP	0x00000200	/* "rgrep" command */
#define EX_SPELLCHECK	0x00000400	/* "spellcheck" command */
#define EX_ITERATE	(EX_LOOP|EX_REPEAT|EX_FOR|EX_WHILE|EX_FORALL|EX_RGREP|EX_SPELLCHECK)
#define EX_CMD		0x00001000	/* misc commands like "if" and "exec" */
#define EX_MACRO	0x00002000	/* running body of macro */
#define EX_INLINE	0x00004000	/* inline include or stopped command */

Exec *exec_stack;		/* Base of execution stack */
int exec_top= 0;		/* First unused element of exec_stack */
int exec_bottom= 0;		/* First used element of exec_stack */
int exec_size= 0;		/* Maximum elements on exec_stack */
#define EXEC_UNIT 40		/* Allocation granularity for exec_stack */


/* PUSH_EXEC - Push a execution context on the execution stack.  Flag is a
 * union of EX_* flags indicating the type of execution this is.  The other
 * arguments depend on the flag:
 *
 *  EX_CALL or EX_STOPPED without EX_INLINE flag:
 *    push_exec(int flag, char *filename)
 *    File is loaded into memory, and root TK_PROCEDURE is put on stack.
 *    Memory will be freed up when execution of file is complete.
 *
 *  EX_CALL or EX_STOPPED with EX_INLINE flag:
 *    push_exec(int flag, Array *proc)
 *    Given TK_PROCEDURE is put on stack.
 *
 *  EX_LOOP or EX_CMD or EX_MACRO:
 *    push_exec(int flag, Array *proc)
 *    Execute a procedure already in memory.  Do not free memory after
 *    execution.
 *
 *  EX_REPEAT or EX_FOR:
 *    push_exec(int flag, Array *proc, int init, int inc, int limit)
 *    Same as previous, but with loop counters.
 *
 *  EX_FORALL:
 *    push_exec(int flag, Array *proc, Token *thru)
 *    Same as EX_LOOP, but with pointer to token to loop through.
 *
 *  EX_RGREP:
 *    push_exec(int flag, Array *proc, Regex *reg, char *str, int nmatch)
 *    Complicated.
 *
 *  EX_SPELLCHECK:
 *    push_exec(int flag, Array *proc, char *text, int mode, int texttype)
 *    Execute body once for each misspelled word in text.  Texttype is 0
 *    for text/plain, 1 for text/html.
 *
 * Note that push_exec (eventually) consumes "proc" tokens that are passed to
 * it, so you should uniquify them before calling this if you want to preserve
 * a copy.
 */

#if __STDC__
void push_exec(int flag, ...)
#else
void push_exec(flag, va_alist)
    int flag;
    va_dcl
#endif
{
    va_list ap;
    Exec *es;

    VA_START(ap, flag);

    /* Check if we have exceeded soft limit */
    if (idict(VAR_EXEC_LIMIT) > 0 && exec_top >= idict(VAR_EXEC_LIMIT))
	die("Execution stack size exceeded exec_limit (%d)",
		idict(VAR_EXEC_LIMIT));

    /* Enlarge the stack if we have exceeded current size */
    if (exec_top >= exec_size)
    {
	exec_size+= EXEC_UNIT;
	if (exec_size == EXEC_UNIT)
	    exec_stack= (Exec *)malloc(exec_size*sizeof(Exec));
	else
	    exec_stack= (Exec *)realloc(exec_stack,exec_size*sizeof(Exec));
    }

    /* Do the push */
    es= exec_stack + exec_top;
    es->flag= flag;
    es->pc= 0;
    exec_top++;

    if (flag & EX_FILE)
    {
    	if (flag & EX_INLINE)
	    /* First argument is procedure */
	    es->proc= va_arg(ap, Array *);
    	else
    	{
	    /* First argument is filename */
	    Token *pt;

	    /* Load a file into procedure structure and put it on the stack */
	    pt= read_script(va_arg(ap, char *));
	    es->proc= aval(*pt);
	    free(pt);
	}
    }
    else
    {
    	/* Put a procedure structure already in memory onto the stack */
    	es->proc= va_arg(ap, Array *);

    	/* Set up loop counter type things */
    	if (flag & (EX_FOR | EX_REPEAT))
	{
	    es->i= va_arg(ap, int);
	    es->inc= va_arg(ap, int);
	    es->limit= va_arg(ap, int);
	}
	else if (flag & EX_FORALL)
	{
	    es->thru= *va_arg(ap, Token *);
	    es->i= 0;  es->inc= 1;
	    if (class(es->thru) == TKC_STRING)
		es->limit= strlen(sval(es->thru)) - 1;
	    else if (class(es->thru) == TKC_ARRAY)
		es->limit= aval(es->thru)->sz - 1;
	}
	else if (flag & EX_RGREP)
	{
	    es->reg= va_arg(ap, Regex *); /* regular expression */
	    es->thru.flag= TK_STRING|TKF_FREE;
	    es->thru.val= (void *)va_arg(ap, char *); /* string */
	    es->i= 0;	/* Current index into string */
	    es->inc= va_arg(ap, int); /* nmatch value */
	    if (rgrep_step(es->reg, sval(es->thru), &es->i, es->inc))
	    	pop_exec();
	}
#if SPELLCHECK == 1
	else if (flag & EX_SPELLCHECK)
	{
	    es->thru.flag= TK_STRING|TKF_FREE;
	    es->thru.val= (void *)va_arg(ap, char *); /* string */
	    es->i= 0;	/* Index of end of last bad word */
	    es->inc= va_arg(ap, int); /* mode value */
	    es->limit= va_arg(ap, int); /* texttype code */
	    if (spellcheck_step(sval(es->thru), &es->i, es->inc, es->limit))
	    	pop_exec();
	}
#endif /*SPELLCHECK*/
    }

    va_end(ap);
}


/* POP_EXEC - Discard the top element from the exec_stack, closing and freeing
 * as needed.  Returns nonzero if there is nothing left on the execution stack
 * after the pop.
 */

int pop_exec()
{
    Exec *es;

    if (exec_top <= exec_bottom) return 1;
    exec_top--;
    es= exec_stack+exec_top;

    /* Discard the loaded program from memory */
    free_array(es->proc);

    /* For "forall" loops, discard structure we looped through */
    if (es->flag & EX_FORALL)
	free_val(&es->thru);

    /* For "rgrep", push remaining string, and discard lots of stuff */
    else if (es->flag & EX_RGREP)
    {
	push_string(sval(es->thru)+es->i, TRUE);
	free_val(&es->thru);
        free_regex(es->reg);
    }

#if SPELLCHECK == 1
    /* For "spellcheck", push remaining string, and discard some stuff */
    else if (es->flag & EX_SPELLCHECK)
    {
	if (es->inc&1) push_string(sval(es->thru)+es->i, TRUE);
	free_val(&es->thru);
	close_spellcheck();
    }
#endif /*SPELLCHECK*/

    return (exec_top == exec_bottom);
}


#ifdef CLEANUP
/* FREE_EXEC - Deallocate all memory assigned to the execution stack.  This
 * is normally only used when debugging, when testing for memory leaks.
 */

void free_exec()
{
	/* Pop everything off the stack */
	func_halt();

	/* Deallocate the stack itself */
	if (exec_size > 0)
		free(exec_stack);
	exec_size= 0;
}
#endif /* CLEANUP */


/* GET_TOKEN - get the next token to execute from the current execution
 * context.  If we hit the end of the current procedure, it does the proper
 * thing:  pop the stack if we aren't a loop, check if we need another
 * iteration if we are a loop.
 *
 * Returns NULL on end of program.
 */

Token *get_token()
{
    Exec *es;

    while (exec_top > exec_bottom)
    {
	es= exec_stack + exec_top - 1;

	/* If current procedure contains another token, return it */
	if (es->pc < es->proc->sz)
	    return &(es->proc->a[es->pc++]);

        /* Handle end-of-procedure processing - mostly looping */
        if (!(es->flag & EX_ITERATE) ||

            ((es->flag & (EX_FOR | EX_REPEAT | EX_FORALL)) &&
             ((es->inc > 0 && es->i + es->inc > es->limit) ||
              (es->inc < 0 && es->i + es->inc < es->limit))) ||

            ((es->flag & EX_WHILE) && !pop_boolean()) ||

            ((es->flag & EX_RGREP) &&
	      rgrep_step(es->reg, sval(es->thru), &es->i, es->inc))

#if SPELLCHECK == 1
	    || ((es->flag & EX_SPELLCHECK) &&
	      spellcheck_step(sval(es->thru), &es->i, es->inc, es->limit))
#endif
	    )
        {
            /* Wasn't a loop, or the loop is done -- pop exec stack */
            pop_exec();
            if (exec_stack[exec_top].flag & EX_STOPPED)
                push_integer(0);
	}
	else
	{
	    /* Set up for next iteration of a loop */
	    es->pc= 0;		/* Reset program counter to start of loop */
            if (es->flag & (EX_FOR | EX_REPEAT | EX_FORALL))
	    	es->i+= es->inc;	/* Increment loop counter */

	    /* Some kinds of loops put stuff on stack before each iteration */
	    if (es->flag & EX_FOR)
	    	/* Push next integer */
	    	push_integer(es->i);
            else if (es->flag & EX_FORALL)
            {
                /* Push next element of string or array */
                if (class(es->thru) == TKC_STRING)
                {
                    char ch[2];
                    ch[0]= sval(es->thru)[es->i];
                    ch[1]= '\0';
                    push_string(ch, TRUE);
                }
                else
                    push_token(&aval(es->thru)->a[es->i], TRUE);
            }
	}
    }
    return NULL;
}


/* PRINT_CONTEXT - This is called when printing error messages to tell where
 * the error occured.  It prints the command name, and the source file name
 * and line number of the current token, doing a sort of execution stack
 * backtrace.  It is supposed to print things like:
 *
 *	executing exch on line 27 of test.bt in call to procedure "reindex"
 *      on line 19 of test.bt included by "call" on line 5 of main.bt
 */

void print_context(FILE *fp)
{
    int i,pc;

    if ((i= exec_top-1) < 0) return;

    fputs("executing ",fp);

    for (;;)
    {
	/* Print command name */
	pc= exec_stack[i].pc;
	if (pc > 0 && pc <= exec_stack[i].proc->sz)
	{
		Token *t= &(exec_stack[i].proc->a[pc-1]);
		fputc('"',fp);
		print_token(fp,t);
		fprintf(fp,"\" on line %d of %s", t->line, getlistfile(t->sfid));
	}
	else 
		fputs("somewhere unidentifiable",fp);	/* shouldn't happen */

        for (;;)
        {
	    if (--i < 0)
	    	return;

	    if (exec_stack[i+1].flag & EX_MACRO)
	    {
	    	fputs(" in call to procedure ",fp);
	    	break;
	    }
	    else if (exec_stack[i+1].flag & (EX_CALL|EX_STOPPED))
	    {
	    	fputs(" included by ",fp);
	    	break;
	    }
        }
    }
}


/* FUNC_HALT: - halt -
 * Absolutely positively terminate execution of the program.
 */

void func_halt()
{
    while (!pop_exec())
    	;
}


/* FUNC_BREAK: - break -
 * Terminate execution of the first enclosing iterative context.  This works
 * even if the break is in a file "included" into a loop body.  If used
 * without an enclosing iterative context, it is equivalent to "halt".
 */

void func_break()
{
    while (!pop_exec() && !(exec_stack[exec_top].flag & EX_ITERATE));
    	;
}


/* FUNC_CONTINUE: - continue -
 * Terminate the current iteration of first enclosing iterative context.
 * This works even if the continue is in a file "included" into a loop body.
 */

void func_continue()
{
    /* Find 1st enclosing iterative context, discarding all intervening ones */
    while (!(exec_stack[exec_top-1].flag & EX_ITERATE))
    {
	if (pop_exec())
	    die("no enclosing LOOP context");
    }

    /* Point program counter to end of procedure */
    exec_stack[exec_top-1].pc= exec_stack[exec_top-1].proc->sz;
}


/* FUNC_STOP: - stop -
 * Terminate execution of the first enclosing file.  This will terminate
 * "include" and "stopped" commands prematurely, or do a "halt" if we are in
 * the top-level script.
 */

void func_stop()
{
    do {
	if (pop_exec()) return;
    } while (!(exec_stack[exec_top].flag & EX_FILE));

    if (exec_stack[exec_top].flag & EX_STOPPED)
	push_integer(1);
}


/* FUNC_JUMP:  <file> jump -
 * Terminate execution of the first enclosing file and begin executing the
 * named file.  Chain does this at compile time.  Jump does it at run time.
 * This is roughly equivalent to, but more efficient than, doing
 * "include stop" or "call stop".
 */

void func_jump()
{
    Exec *es;
    char *src= pop_string();

    nodotdot(src);

    /* Pop off everything on top of the first enclosing FILE context */
    while (!(exec_stack[exec_top-1].flag & EX_FILE))
    {
	if (pop_exec())
	    die("no enclosing FILE context");
    }

    pop_exec();

    push_exec(EX_CALL,src);
    free(src);
}


/* FUNC_CHAIN: <proc> chain -
 * Same as jump, except that compiler loaded the file for us inline.
 */

void func_chain()
{
    Exec *es;
    Array *proc= pop_proc();

    /* Pop off everything on top of the first enclosing FILE context */
    while (!(exec_stack[exec_top-1].flag & EX_FILE))
    {
	if (pop_exec())
	    die("no enclosing FILE context");
    }

    pop_exec();

    push_exec(EX_CALL|EX_INLINE, proc);
}


/* FUNC_EXEC:  <any> exec
 * Execute the token given.  If it is a procedure or array, run it now.
 * If it is a literal, execute that symbol.
 */

void func_exec()
{
    Token t;
    Token *v;

    pop_any(&t);
    switch (type(t))
    {
    case TK_MARK:
    case TK_STRING:
    case TK_INTEGER:
	push_token(&t,FALSE);
	break;

    case TK_ARRAY:
    case TK_PROCEDURE:
	push_exec(EX_CMD, aval(t));
	break;
    
    case TK_BOUND_LITERAL:
    case TK_UNBOUND_LITERAL:
	if ((v= get_symbol(&t)) == NULL)
	    die("Undefined symbol '%s'",sval(t));
	free_val(&t);

	if (type(*v) == TK_FUNCTION)
	    (*(fval(*v)))();
#ifdef DYNALOAD
	else if (type(*v) == TK_DYNAMIC)
	    (*(dynaload(v)))();
#endif
	else if (type(*v) == TK_PROCEDURE)
	{
	    uniquify_token(v);
	    push_exec(EX_MACRO, aval(*v));
	}
	else
	    push_token(v,TRUE);
	break;
    }
}


/* FUNC_CALL:     <filename> call -
 * Execute the contents of the named file, then continue execution here.
 * Call does this at run time, include does it at compile time.
 */

void func_call()
{
    char *file= pop_string();

    nodotdot(file);
    push_exec(EX_CALL, file);
    free(file);
}


/* FUNC_STOPPED:  <proc> stopped <bool>
 * This is just like "include", but upon termination, either a zero or a one
 * is pushed on the stack.  The 1 indicates that the execution of the file
 * was terminated with a "stop".  0 indicates otherwise.
 */

void func_stopped()
{
    Array *proc= pop_proc();

    push_exec(EX_STOPPED|EX_INLINE, proc);
}


#if 0
/* FUNC_RUN_TIME_STOPPED:  <filename> stopped <bool>
 * The run-time version of stopped doesn't have a name right now.
 */

void func_run_time_stopped()
{
    char *file= pop_string();

    nodotdot(file);
    push_exec(EX_STOPPED, file);
    free(file);
}
#endif


/* FUNC_INCLUDE:  <proc> include -
 * Same as call, except that compiler loaded the file for us inline.
 */

void func_include()
{
    Array *proc= pop_proc();

    push_exec(EX_CALL|EX_INLINE, proc);
}


/* FUNC_LOOP: <proc> loop -
 * This executes the given procedure forever, or until a "break" or "halt" is
 * done in the procedure.  Most of the brains of this is in get_token().
 */

void func_loop()
{
    push_exec(EX_LOOP, pop_proc());
}


/* FUNC_WHILE: <proc> while -
 * This executes the given procedure repeatedly.  The procedure should always
 * leave an integer on the top of the stack.  "while" will pop off this
 * integer, and execute again if it is non-zero.  Most of the brains of this
 * is in get_token().
 */

void func_while()
{
    push_exec(EX_WHILE, pop_proc());
}


/* FUNC_REPEAT: <count> <proc> repeat -
 * This executes the given procedure <count> times.
 */

void func_repeat()
{
    Array *proc= pop_proc();
    int count= pop_integer();

    if (count < 0) die("Negative iteration count");
    if (count == 0)
    {
	free(proc);
	return;
    }

    push_exec(EX_REPEAT, proc, 1, 1, count);
}


/* FUNC_FOR: <init> <inc> <limit> <proc> for -
 * This executes the given procedure repeatedly.  <init>, <inc>, and <limit>
 * are all integers.  <init> will be pushed on the stack before executing
 * the procedure for the first time.  Before each successive iteration, an
 * integer with <init> added to it.  Iteration ends when the counter passes
 * <limit>, (i.e. grows greater than <limit> if <inc> is positive, or grows
 * less if <inc> is negative).  If <inc> is 0, this is an infinite loop.
 */

void func_for()
{
    Array *proc= pop_proc();
    int limit= pop_integer();
    int inc= pop_integer();
    int init= peek_integer();

    if ((inc > 0 && init > limit) || (inc < 0 && init < limit))
    {
	func_pop();  /* discard init */
	free_array(proc);
	return;
    }

    push_exec(EX_FOR, proc, init, inc, limit);
}


/* FUNC_FORALL: <thru> <proc> while -
 * This executes the given procedure once for each element of <thru>" which
 * may be either a string or an array.
 */

void func_forall()
{
    Array *proc= pop_proc();
    Token t;

    pop_any(&t);
    if (class(t) == TKC_ARRAY)
    {
	if (aval(t)->sz == 0)
	{
	    free_array(proc);
	    return;
	}
	push_token(aval(t)->a, TRUE);	/* Push first element */
	push_exec(EX_FORALL, proc, &t);
    }
    else if (class(t) == TKC_STRING)
    {
	char ch[2];
	if (sval(t)[0] == '\0')
	{
	    free_array(proc);
	    return;
	}
	ch[0]= sval(t)[0];
	ch[1]= '\0';
	push_string(ch, TRUE);	/* Push first element */
	push_exec(EX_FORALL, proc, &t);
    }
    else
    {
	die("Expected string or array");
    }
}


/* FUNC_RGREP: <str> <regex> <nmatch> <proc> rgrep -
 * This searches string for sections matching the regular expression.
 * Each time a match is found, the non-matching substring between the last
 * match and this match is pushed on the stack.  If nmatch is one, the
 * matching substring is also pushed.  If nmatch is greater than one, then
 * nmatch substrings are pushed, the first being the substring that matched
 * the whole expression, and the later ones being the substrings that matched
 * any parenthesized subexpressions.  After all this stuff has been pushed
 * the procedure is executed.  This continues until no more matches are found
 * or until the procedure does a break.  At that time, the remaining unmatched
 * hunk of string will be pushed.  A negative nmatch value means push all
 * subexpression matchs.
 */

void func_rgrep()
{
    Array *proc= pop_proc();
    int nmatch= pop_integer();
    Regex *reg= pop_regex();
    char *str= pop_string();

    if (nmatch < 0 || nmatch > reg->re.re_nsub + 1)
	nmatch= reg->re.re_nsub + 1;

    push_exec(EX_RGREP, proc, reg, str, nmatch);
}


/* FUNC_SPELLCHECK:  <text> <texttype> <mode> <proc> spellcheck -
 *  For each misspelled word in the given text, execute the given
 *  procedure. If mode is 0, just push the misspelled word before running
 *  the procedure.  If mode is 1, push first the section of OK text since
 *  the last spelling error, and then the misspelled word.  The text type
 *  my be (text/plain) or (text/html).  In the latter case, HTML tags and
 *  such are ignored.
 */

#if SPELLCHECK==1
void func_spellcheck()
{
    Array *proc= pop_proc();
    int mode= pop_integer();
    char *texttype= pop_string();
    char *text= pop_string();

    if (mode < 0 || mode > 3) die("mode must be 0, 1, 2, or 3");
    push_exec( EX_SPELLCHECK, proc, text, mode,
	strcmp(texttype, "text/html")?0:1 );

    free(texttype);
}
#else
/* If we don't have Ispell, implement NO-OP version - just discards its
 * arguments (leaving text if mode=1 or mode=3).
 */
void func_spellcheck()
{
    Array *proc= pop_proc();
    int mode= pop_integer();
    char *texttype= pop_string();

    if (mode < 0 || mode > 3) die("mode must be 0, 1, 2, or 3");
    free(texttype);
    free_array(proc);

    if (!(mode & 1))
    {
        char *text= pop_string();
    	free(text);
    }
}
#endif


/* FUNC_IF: <bool> <proc> if -
 * This executes the given procedure only if the boolean is a non-zero integer.
 * In other words, it's the same as "<bool> 0 ne <proc> repeat".
 */

void func_if()
{
    Array *proc= pop_proc();

    if (pop_boolean())
	push_exec(EX_CMD, proc);
    else
	free_array(proc);
}


/* FUNC_IFELSE: <bool> <then-proc> <else-proc> ifelse -
 * This executes the either <then-proc> or <else-proc> depending on whether
 * <bool> is non-zero or not.
 */

void func_ifelse()
{
    Array *elseproc= pop_proc();
    Array *thenproc= pop_proc();

    if (pop_boolean())
    {
	free_array(elseproc);
	push_exec(EX_CMD,thenproc);
    }
    else
    {
	free_array(thenproc);
	push_exec(EX_CMD,elseproc);
    }
}


/* RUN - main entrance to the interpreter. This pushes the file name to
 * interpret onto the stack and invokes the interpreter
 */

void run(char *filename)
{
    push_exec(EX_CALL, filename);
    interpret();
}

/* INTERPRET - This is the main execution loop.  
 * It is *NOT* normally called recursively.
 * Recursion is handled by the fact that get_token reads from whatever is
 * currently on top of the exec_stack.
 */

void interpret()
{
    Token *t, *v;

    while ((t= get_token()) != NULL)
	switch (type(*t))
	{
	case TK_MARK:
	case TK_REGEX:
	case TK_STRING:
	case TK_INTEGER:
	case TK_PROCEDURE:
	case TK_BOUND_LITERAL:
	case TK_UNBOUND_LITERAL:
	    push_token(t,TRUE);
	    break;
	
	case TK_UNBOUND_SYMBOL:
	case TK_BOUND_SYMBOL:
	    if ((v= get_symbol(t)) == NULL)
		die("Undefined symbol '%s'",sval(*t));

	    if (type(*v) == TK_FUNCTION)
		(*(fval(*v)))();
#ifdef DYNALOAD
	    else if (type(*v) == TK_DYNAMIC)
		(*(dynaload(v)))();
#endif
	    else if (type(*v) == TK_PROCEDURE)
	    {
		uniquify_token(v);
		push_exec(EX_MACRO, aval(*v));
	    }
	    else
		push_token(v,TRUE);
	    break;
	}
}

/* RERUN: This is an entry to invoke the interpreter recursively.
 * It is used when some operator wants to interpret a function
 * without losing its context. exec_bottom marks the boundary on the exec
 * stack so that the interpreter will exit back to here when the
 * string has been evaluated. Arguments should be pushed in advance.
 * Note that this consumes the Array that is passed in, so you should uniquify
 * it before calling this if you want to keep a copy.
 */

void rerun(Array *proc)
{
    int saved_bottom= exec_bottom;

    exec_bottom= exec_top;
    push_exec(EX_MACRO, proc);
    interpret();
    exec_bottom= saved_bottom;
}
