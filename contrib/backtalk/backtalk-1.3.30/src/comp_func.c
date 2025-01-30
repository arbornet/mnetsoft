/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* COMPILE-TIME FUNCTIONS - these routines do compile-time optimizations on
 * the given functions.  Mostly optimization is possible when a function with
 * no side effects is invoked with all constant string or integer arguments.
 *
 * We'd be able to do much more if we always know the arrity of functions.
 */

#include "backtalk.h"
#include "sysdict.h"
#include "str.h"
#include "token.h"
#include "comp.h"
#include "comp_tok.h"
#include "comp_util.h"
#include "authident.h"
#include "userfunc.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define bv(c) (bval((c)->t))
#define iv(c) (ival((c)->t))
#define sv(c) (sval((c)->t))

void func_not()
{
    CToken *bool;
    int val;
    word line;
    byte sfid;

    if (template("-b",&bool)) return;

    val= bv(bool);
    drop_token(2,TRUE,&sfid,&line);
    add_tok(TK_INTEGER, sfid, line, (void *)(long)!val);
}


void func_concatinate()
{
    CToken *a, *b;
    char *sa,*sb;
    int la,lb;
    word line;
    byte sfid;

    if (!template("-`"))
    {
	/* Have ` ' on stack - turn into a null string */
	sa= (char *)malloc(1);
	*sa= '\0';
	drop_token(2, TRUE, &sfid,&line);
	add_tok(TK_STRING|TKF_FREE, sfid, line, sa);
	return;
    }

    while (!template("-cc", &b, &a))
    {
	sb= token_to_string(&(b->t));
	if (type(a->t) == TK_MARK && iv(a) == 1)
	{
	    /* Have something like ` <whatever> ' */
	    drop_token(3,TRUE, &sfid,&line);
	    add_tok(TK_STRING|TKF_FREE, sfid, line, sb);
	    return;
	}
	else
	{
	    /* Have something like <constant> <constant> ' */
	    sa= token_to_string(&(a->t));
	    lb= strlen(sb);
	    if (lb > 0)
	    {
		la= strlen(sa);
		sa= (char *)realloc(sa, la + lb + 1);
		strcpy(sa + la, sb);
	    }
	    free(sb);
	    drop_token(3,TRUE,&sfid,&line);
	    add_tok(TK_STRING|TKF_FREE, sfid, line, sa);
	    add_tok(TK_BOUND_SYMBOL, sfid, line, (void*)FUNC_RPRIME);
	}
    }
}


void func_mul()
{
    CToken *a, *b;

    if (template("-ii",&b, &a)) return;
    a->t.val= (void *)(iv(a) * iv(b));
    drop_token(2, TRUE, NULL, NULL);
}

void func_add()
{
    CToken *a, *b;

    if (!template("-ii",&b, &a))
    {
	/* Good old American Addition */
	a->t.val= (void *)(iv(a) + iv(b));
	    drop_token(2, TRUE, NULL, NULL);
    }
    else if (!template("-ss",&b, &a))
    {
	/* String concatination */
	int alen= strlen(sv(a));
	int blen= strlen(sv(b));
	char *new= (char *)malloc(alen+blen+1);
	strcpy(new,sv(a));
	strcpy(new+alen,sv(b));
	free(sv(a));
	a->t.val= (void *)new;
	drop_token(2, TRUE, NULL, NULL);
    }
}


void func_sub()
{
    CToken *a, *b;

    if (template("-ii",&b, &a)) return;
    a->t.val= (void *)(iv(a) - iv(b));
    drop_token(2, TRUE, NULL, NULL);
}

void func_div()
{
    CToken *a, *b;

    if (template("-ii",&b, &a)) return;
    a->t.val= (void *)(iv(a) / iv(b));
    drop_token(2, TRUE, NULL, NULL);
}

void func_mkarray()
{
    /* This should do something someday, but doesn't yet */
}


void func_abs()
{
    CToken *a;

    if (template("-i",&a)) return;
    if (iv(a) < 0) a->t.val= (void *)(-iv(a));
    drop_token(1, TRUE, NULL, NULL);
}

void func_aload() {}

void func_and()
{
    CToken *a, *b;
    int val;
    word line;
    byte sfid;

    /* Optimzing "and" is harder than it looks.
     * We cannot map  "0 <something> and"  =>  "0"
     *   because the expression might be "1 1 0 + and"
     * We cannot map  "<something> 0 and" => "0".
     *   because the expression might have been "1 0 + 0 and"
     *   which would leave us with "1 0 0" instead of "0".
     * So we map "<something> 0 and" => "<something> pop 0".
     * Probably we could do better, but this will do for now.
     */

    if (!template("-bb",&b, &a))
    {
	val= (bv(a) && bv(b));
	drop_token(3, TRUE, &sfid, &line);
	add_tok(TK_INTEGER, sfid, line, (void *)(long)val);
    }
    else if (!template("-b-",&a) && !bv(a))
	  /* || (!template("--b",&a) && !bv(a)) NO! NO! NO! */
    {
	drop_token(2, TRUE, &sfid, &line);
	add_tok(TK_BOUND_SYMBOL, sfid, line, (void*)FUNC_POP);
	add_tok(TK_INTEGER, sfid, line, (void *)0);
    }
}

void func_asort() {}
void func_backtalkurl() {}

void func_band()
{
    CToken *a, *b;

    if (template("-ii",&b, &a)) return;
    a->t.val= (void *)(iv(a) & iv(b));
    drop_token(2, TRUE, NULL, NULL);
}

void func_begin() {}

void func_bor()
{
    CToken *a, *b;

    if (template("-ii",&b, &a)) return;
    a->t.val= (void *)(iv(a) | iv(b));
    drop_token(2, TRUE, NULL, NULL);
}

void func_break() {}
void func_browser() {}
void func_cache_conflist() {}

void func_cap()
{
    CToken *a;

    if (template("-s",&a)) return;
    capfirst(sv(a));
    drop_token(1, TRUE, NULL, NULL);
}

void func_caps()
{
    CToken *a;

    if (template("-s",&a)) return;
    capall(sv(a));
    drop_token(1, TRUE, NULL, NULL);
}

void func_call() {}
void func_cflist_array() {}

void func_cgiquote()
{
    CToken *a;
    char *output;

    if (template("-s",&a)) return;
    output= cgiquote(sv(a));
    free(sv(a));
    a->t.val= (void *)output;
    drop_token(1, TRUE, NULL, NULL);
}

void func_chain()
{
    CToken *file;
    char *filename;
    int fileindex;
    word line;
    byte sfid;

    if (template("-s",&file))
	die("chain requires a constant string argument");

    /* Discard the chain command */
    filename= strdup(sv(file));
    nodotdot(filename);
    drop_token(2, TRUE, &sfid, &line);

    /* Generate the inlined code */
    do_inline(FUNC_CHAIN, filename, sfid, line);
    free(filename);
}

void func_changegroup() {}

void func_changeemail()
{
    /* Purely by coincidence, the optimization is the same as for useremail */
    func_useremail();
}

void func_changename() {}
void func_changepass() {}
void func_check_partic() {}
void func_chomp() {}	   /* Should do this */
void func_cleanhtml() {}   /* Could do this */
void func_clear() {}
void func_cleartomark() {}
void func_clip() {}
void func_close_conf() {}
void func_conf_alias() {}

void func_conf_bull()		/* MACRO for "(*bull) read" */
{
    word line;
    byte sfid;

    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup("*bull"));
    add_tok(TK_BOUND_SYMBOL, sfid, line, (void*)FUNC_READ);
}

void func_conf_dir() {}

void func_conf_index()		/* MACRO for "(*index) read" */
{
    word line;
    byte sfid;

    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup("*index"));
    add_tok(TK_BOUND_SYMBOL, sfid, line, (void *)FUNC_READ);
}

void func_conf_login()		/* MACRO for "(*login) read" */
{
    word line;
    byte sfid;

    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup("*login"));
    add_tok(TK_BOUND_SYMBOL, sfid, line, (void *)FUNC_READ);
}

void func_conf_logout()		/* MACRO for "(*logout) read" */
{
    word line;
    byte sfid;

    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup("*logout"));
    add_tok(TK_BOUND_SYMBOL, sfid, line, (void *)FUNC_READ);
}

void func_conf_new() {}

void func_constant()
{
    CToken *lit;
    byte sfid;
    word line;
    int is_constant;

    if (template("-l",&lit))
	die("constant requires a literal argument");

    is_constant= ((type(lit->t) == TK_BOUND_LITERAL &&
		  sysdict[iv(lit)].flag&DT_CONSTANT) ||
		 (type(lit->t) == TK_UNBOUND_LITERAL &&
		  FindHashEntry(&constdict, sv(lit)) != NULL));

    drop_token(2, TRUE, &sfid, &line);
    add_tok(TK_INTEGER, sfid, line, (void *)(long)is_constant);
}

void func_continue() {}
void func_copy() {}
void func_count() {}
void func_count_sel() {}
void func_counttomark() {}
void func_ctime() {}

void func_cvi()
{
    CToken *a;
    long value;
    byte sfid;
    word line;

    if (!template("-i",&a))
    {
	/* cvi is no-op on an integer -- so delete it */
	drop_token(1, TRUE, NULL, NULL);
    }
    else if (!template("-s",&a))
    {
	value= atol(sv(a));
	drop_token(2, TRUE, &sfid, &line);
	add_tok(TK_INTEGER, sfid, line, (void *)value);
    }
}

void func_cvn()
{
    CToken *a;

    if (!template("-s",&a))
    {
	drop_token(1, TRUE, NULL, NULL);
	a->t.flag= TK_UNBOUND_LITERAL;
	/* Should bind it if it is in the system dictionary */
	/* But I'm too lazy */
    }
}

void func_cvs()
{
    CToken *a;
    char buf[30];
    byte sfid;
    word line;

    if (!template("-s",&a))
    {
	/* cvs is no-op on a string -- so delete it */
	drop_token(1, TRUE, NULL, NULL);
    }
    else if (!template("-c",&a))
    {
	/* Other kinds of literals */
	char *s= token_to_string(&(a->t));
	drop_token(2, TRUE, &sfid, &line);
	add_tok(TK_STRING, sfid, line, (void *)s);
    }
}


void func_cvscol()
{
    CToken *val,*ncol;
    char buf[30];
    byte sfid;
    word line;
    int cols;

    if (template("-ii",&ncol,&val)) return;

    cols= iv(ncol);
    if (cols > 29) cols=29;
    sprintf(buf,"%*d",iv(val),cols);
    drop_token(3, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, (void *)strdup(buf));
}

void func_cvt() {}	/* could be expanded out, but hasn't been */
void func_dec() {}
void func_def() {}

void func_default()
{
    CToken *lit, *def;
    HashEntry *h;
    byte sfid;
    word line;
    Token *dv;

    if (template("-cl",&def, &lit)) return;

    if (type(lit->t) == TK_BOUND_LITERAL)
    {
	/* We know system variables are defined */
	/* "/whatever value default"  =>  "whatever" */
	drop_token(2, TRUE, NULL, NULL);
	lit->t.flag= TK_BOUND_SYMBOL;
    }
    else if ((h= FindHashEntry(&constdict, sv(lit))) != NULL)
    {
	/* We know system constants are defined */
	/* "/whatever value default"  =>  value of whatever */
	drop_token(3, TRUE, &sfid, &line);
	dv= &(h->t);
	if (class(*dv) == TKC_STRING)
	    add_tok(dv->flag, sfid, line, (void *)strdup(sval(*dv)));
	else
	    add_tok(dv->flag, sfid, line, dv->val);
    }
}

void func_defconstant()
{
    CToken *val, *lit;

    if (template("-cl",&val,&lit)) return;

    /* Add the constant to the compile-time constant dictionary */
    if (def_constant(&(lit->t), &(val->t), 0) != 2)
    {
	/* Unless we are resetting a system constant, strip the
	 * defconstant command out of the binary.  Need to leave it
	 * in if it is a system constant because backtalk interpreter
	 * will be looking at dictionary value (or else why would it
	 * be a system constant?).
	 */
	drop_token(3, TRUE, NULL, NULL);
    }
}

void func_defined()
{
    CToken *lit;
    byte sfid;
    word line;

    if (template("-l",&lit)) return;

    /* We know system variables and constants are defined */
    if (type(lit->t) == TK_BOUND_LITERAL ||
	FindHashEntry(&constdict, sv(lit)) != NULL)
    {
	drop_token(2, TRUE, &sfid, &line);
	add_tok(TK_INTEGER, sfid, line, (void *)1);
    }
}

void func_del_conf() {}
void func_delgroup() {}
void func_delgroupmem() {}

/* If we have a defines telling us what the default group is, insert it */
void func_dfltgroup()
{
#ifdef USER_GROUP 
    word line;
    byte sfid;

    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup(USER_GROUP));
#else
#ifndef USER_GID
    word line;
    byte sfid;

    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup(""));
#endif
#endif
}

void func_dict() {}
void func_directory() {}
void func_dtime() {}
void func_dumpstack() {}
void func_dup() {}
void func_edit_attach() {}
void func_edit_resp() {}
void func_email_body() {}
void func_email_close() {}
void func_email_new() {}
void func_email_recip() {}
void func_email_send() {}
void func_end() {}

int *do_cmp()
{
    CToken *a, *b;
    word line;
    byte sfid;
    long cmp;

    if (!template("-ii", &b, &a))
	cmp= iv(a) - iv(b);
    else if (!template("-ss", &b, &a))
	cmp= strcmp(sv(a),sv(b));
    else
	return NULL;

    drop_token(3, TRUE, &sfid, &line);
    a= add_tok(TK_INTEGER, sfid, line, (void *)cmp);
    return((int *)(&(a->t.val)));
}

void func_eq()
{
    int *cmp; if ((cmp= do_cmp()) != NULL) *cmp= (*cmp == 0);
}

void func_erase_resp() {}

void func_exch() 
{
    CToken *a, *b;

    if (template("-cc", &b, &a)) return;
    drop_token(1, TRUE, NULL, NULL);
    drop_token(2,FALSE, NULL, NULL);
    add_ctoken(b);
    add_ctoken(a);
}

void func_exec() {}
void func_exists() {}
void func_expand() {}	/* A good one to add in the future */
void func_fav_sel() {}  /* Could do flag=0 case */
void func_filedate() {}
void func_firstdir() {}
void func_first_partic() {}
void func_firstuser() {}
void func_for() {}	/* could do this, but rarely useful */
void func_forall() {}	/* could do this, but rarely useful */
void func_forget_item() {}
void func_freeze_item() {}

void func_ge()
{
    int *cmp; if ((cmp= do_cmp()) != NULL) *cmp= (*cmp >= 0);
}

void func_get() {}	/* Maybe after we add constant arrays */
void func_getcookie() {}
void func_get_attach() {}
void func_get_conf_note() {}
void func_get_item_note() {}
void func_grep() {}	/* Possible, but too rarely useful */
void func_groupid() {}
void func_groupname() {}
void func_groups() {}

void func_gt()
{
    int *cmp; if ((cmp= do_cmp()) != NULL) *cmp= (*cmp > 0);
}

void func_halt() {}
void func_hide_resp() {}
void func_htime() {}

void func_if()
{
    CToken *bool, *proc;

    if (template("-pb",&proc,&bool)) return;

    if (bv(bool))
    {
	drop_token(1, TRUE, NULL, NULL);	/* discard "if" */
	drop_token(1, FALSE, NULL, NULL);	/* remove procedure */
	drop_token(1, TRUE, NULL, NULL);	/* discard boolean */
	add_proc(proc, TRUE);			/* insert procedure inline */
    }
    else
	drop_token (3, TRUE, NULL, NULL);	/* discard the whole if */
}

void func_ifelse()
{
    CToken *bool, *then_proc, *else_proc;

    if (template("-ppb",&else_proc,&then_proc,&bool)) return;

    if (bv(bool))
    {
	drop_token(2, TRUE, NULL, NULL);	/* discard "if" and else_proc*/
	drop_token(1, FALSE, NULL, NULL);	/* remove then_proc */
	drop_token(1, TRUE, NULL, NULL);	/* discard boolean */
	add_proc(then_proc, TRUE);		/* insert then_proc inline */
    }
    else
    {
	drop_token(1, TRUE, NULL, NULL);	/* discard "if" */
	drop_token(1, FALSE, NULL, NULL);	/* remove else_proc */
	drop_token(2, TRUE, NULL, NULL);	/* discard then_proc and bool */
	add_proc(else_proc, TRUE);		/* insert then_proc inline */
    }
}

void func_in() {}
void func_inc() {}

void func_include()
{
    CToken *file;
    char *filename;
    int fileindex;
    byte sfid;
    word line;

    if (template("-s",&file))
	die("include requires a constant string argument");

    /* Discard the include command */
    filename= strdup(sv(file));
    nodotdot(filename);
    drop_token(2, TRUE, &sfid, &line);

    /* Generate the inlined code */
    do_inline(FUNC_INCLUDE,filename,sfid,line);
    free(filename);
}

void func_index() {}
void func_ingroup() {}
void func_inlist() {}
void func_in_sel() {}
void func_itime() {}
void func_jump() {}
void func_jointomark() {}	/* this could be done */

void func_jsquote()
{
    CToken *a;
    char *output;

    if (template("-s",&a)) return;
    output= jsquote(sv(a));
    free(sv(a));
    a->t.val= (void *)output;
    drop_token(1, TRUE, NULL, NULL);
}

void func_killsession() {}
void func_kill_item() {}
void func_known() {} 		/* this could be done */

void func_last_in_conf() {}

void func_le()
{
    int *cmp; if ((cmp= do_cmp()) != NULL) *cmp= (*cmp <= 0);
}

void func_length()
{
    CToken *str;
    byte sfid;
    word line;
    long len;

    if (template("-s",&str)) return;

    len= strlen(sv(str));
    drop_token(2, TRUE, &sfid, &line);
    add_tok(TK_INTEGER, sfid, line, (void *)len);

    /* Add case for constant arrays, if we ever get them */
}

void func_line() {}
void func_link_item() {}
void func_loadvar() {}
void func_loop() {}

void func_lt()
{
    int *cmp; if ((cmp= do_cmp()) != NULL) *cmp= (*cmp < 0);
}

void func_macro() {}
void func_make_attach() {}
void func_mark_unseen() {}

void func_mimename()
{
#if !ATTACHMENTS
    /* Without attachments, mimetype is a no-op */
    drop_token(1,TRUE, NULL, NULL);
#endif
}

void func_mod()
{
    CToken *a, *b;

    if (template("-ii",&b, &a)) return;
    a->t.val= (void *)(iv(a) % iv(b));
    drop_token(2, TRUE, NULL, NULL);
}

void func_ne()
{
    int *cmp; if ((cmp= do_cmp()) != NULL) *cmp= (*cmp != 0);
}

void func_neg()
{
    CToken *a;

    if (template("-i",&a)) return;
    a->t.val= (void *)(-iv(a));
    drop_token(1, TRUE, NULL, NULL);
}

void func_new_conf() {}
void func_newgroup() {}
void func_newgroupmem() {}
void func_newsession() {}
void func_newuser() {}
void func_next_conf() {}
void func_next_int() {}
void func_next_item() {}
void func_next_resp() {}
void func_nextdir() {}
void func_next_partic() {}
void func_nextuser() {}
void func_ogrep() {}
void func_open_conf() {}

void func_or()
{
    CToken *a, *b;
    int val;
    byte sfid;
    word line;

    /* See big long comment on func_and */

    if (!template("-bb",&b, &a))
    {
	val= (bv(a) || bv(b));
	drop_token(3, TRUE, &sfid, &line);
	add_tok(TK_INTEGER, sfid, line, (void *)(long)val);
    }
    else if ((!template("-b-",&a) && bv(a))
	  /* || (!template("--b",&a) && bv(a)) NO! NO! NO! */ )
    {
	drop_token(2, TRUE, &sfid, &line);
	add_tok(TK_BOUND_SYMBOL, sfid, line, (void*)FUNC_POP);
	add_tok(TK_INTEGER, sfid, line, (void *)1);
    }
}

void func_parse() {}
void func_pathname() {}
void func_pop() {}
void func_post_item() {}
void func_post_resp() {}
void func_pr() {}
void func_print() {}
void func_put() {}	/* Maybe after we have constant arrays */
void func_quote() {}	/* This would be nice */
void func_rand() {}
void func_read() {}
void func_read_item() {}
void func_read_resp() {}
void func_read_text() {}
void func_readable() {}
void func_reattach() {}
void func_regex() {}
void func_remember_item() {}
void func_removeuser() {}
void func_repeat() {}	/* Could remove "0 {...} repeat" case, but too rare */
void func_replace() {}
void func_resign_conf() {}
void func_retire_item() {}
void func_retitle_item() {}
void func_rev_sel() {}
void func_rgrep() {}
void func_roll() {}
void func_savevar() {}
void func_search() {}
void func_seekuser() {}
void func_select() {}	/* Could do this */
void func_selectuser() {}
void func_serverurl() {}
void func_setcookie() {}
void func_set_conf_note() {}
void func_set_item_note() {}
void func_sgrep() {}
void func_showopt() {} /* Could do this */
void func_sleep() {}
void func_snoop_conf() {}
void func_spellcheck() {}
void func_split() {} /* Could do this */
void func_srand() {}
void func_stop() {}

void func_stopped()
{
    CToken *file;
    char *filename;
    int fileindex;
    word line;
    byte sfid;

    if (template("-s",&file))
	die("stopped requires a constant string argument");

    /* Discard the stopped command */
    filename= strdup(sv(file));
    nodotdot(filename);
    drop_token(2, TRUE, &sfid, &line);

    /* Generate the inlined code */
    do_inline(FUNC_STOPPED,filename,sfid,line);
    free(filename);
}

void func_store()
{
    CToken *val, *lit;

    if (template("-cl",&val,&lit)) return;

    /* If we are storing to a system constant, save the value */
    def_constant(&(lit->t), &(val->t), 1);
}

void func_substr() {}	/* Could do this */
void func_tail() {}
void func_this_item() {}
void func_time() {}
void func_timeout() {}
void func_undef() {}

void func_undefconstant()
{
    CToken *lit;
    word line;
    byte sfid;

    if (template("-l",&lit)) return;

    /* Remove the constant to the compile-time constant dictionary */
    undef_constant(&(lit->t));

    /* Drop the token */
    drop_token(2, TRUE, &sfid, &line);
}

void func_unhtml() {}   /* Could do this */

void func_useremail()
{
    /* If email addresses are not stored, always optimize this away */
    CToken *user;
    word line;
    byte sfid;
#ifndef IDENT_STOREEMAIL
    if (template("-s",&user))
	drop_token(2, TRUE, &sfid, &line);
    else
    {
	drop_token(1, TRUE, &sfid, &line);
	add_tok(TK_BOUND_SYMBOL, sfid, line, (void*)FUNC_POP);
    }
    add_tok(TK_STRING, sfid, line, strdup(""));
#endif /* !IDENT_STOREEMAIL */
}

void func_userinfo() {}
void func_validate() {}

void func_version()
{
    char bf[BFSZ];
    word line;
    byte sfid;
    CToken *cmd;

    /* We always expand the version -- really should be a variable */
    if (template("a",&cmd)) return;
    drop_token(1, TRUE, &sfid, &line);
    add_tok(TK_STRING, sfid, line, strdup(version_string(bf)));
}

void func_while() {}
void func_wrap() {}
void func_writable() {}
void func_write() {}
void func_xdef() {}

void func_xdefconstant()
{
    CToken *val, *lit;

    if (template("-lc",&lit,&val)) return;

    /* Add the constant to the compile-time constant dictionary */
    if (def_constant(&(lit->t), &(val->t), 0) != 2)
    {
	/* Unless we are resetting a system constant,
	 * Strip the defconstant command out of the binary */
	drop_token(3, TRUE, NULL, NULL);
    }
}

void func_xstore()
{
    CToken *val, *lit;

    if (template("-lc",&lit,&val)) return;

    /* If we are storing to a system constant, save the value */
    def_constant(&(lit->t), &(val->t), 1);
}

void func_bnot()
{
    CToken *a;

    if (template("-i",&a)) return;
    a->t.val= (void *)(~iv(a));
    drop_token(1, TRUE, NULL, NULL);
}

void func_ztime() {}
