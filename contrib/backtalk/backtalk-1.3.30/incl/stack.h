/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void push_token(Token *, int);
void push_integer(long n);
void push_time(time_t n);
void pop_any(Token *);
char *pop_string(void);
Array *pop_proc(void);
int pop_literal(Token *);
char *pop_litname(void);
Array *pop_array(void);
Regex *pop_regex(void);
HashTable *pop_dict(void);
long pop_integer(void);
time_t pop_time(void);
int pop_boolean(void);
void push_string(char *, int);
void push_literal(char *, int);
void push_bound_literal(long);
void push_array(Array *, int);
void push_regex(Regex *, int);
void push_proc(Array *, int);
void push_dict(HashTable *, int);
long peek_integer(void);
int peek_boolean(void);
char *peek_string(void);
char *peek_literal(void);
Array *peek_proc(void);
void repl_top(void *val);
void uniquify_token(Token *);
void dump_stack(FILE *, int);
int make_array(int);
int push_array_mark(void);
#ifdef CLEANUP
void free_stack(void);
#endif /*CLEANUP*/

void func_pop(void);
void func_clear(void);
void func_cleartomark(void);
void func_exch(void);
void func_dup(void);
void func_copy(void);
void func_roll(void);
void func_count(void);
void func_counttomark(void);
void func_index(void);
void func_macro(void);
void func_bisel(void);
void func_select(void);
void func_dumpstack(void);
void func_print(void);
void func_pr(void);
void func_mkarray(void);
void func_concatinate(void);
void func_jointomark(void);
