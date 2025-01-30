/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

Token *get_dict(char *label);
Token *get_sysdict(char *label);
Token *get_symbol(Token *s);
void func_dict(void);
void func_begin(void);
void func_end(void);
#ifdef CLEANUP
void free_dictstack(void);
#endif
void dump_dict(HashTable *dict, FILE *fp);
void dump_dict_stack(FILE *fp);
void set_dict(HashEntry *d, Token *t, int flag);
void set_sys_dict(char *label, char *val, int copy);
void func_defconstant(void);
void func_xdefconstant(void);
void func_undefconstant(void);
void func_def(void);
void func_xdef(void);
void func_store(void);
void func_xstore(void);
void func_undef(void);
void func_inc(void);
void func_dec(void);
void func_defined(void);
void func_default(void);
void func_constant(void);
char *csdict(int i);
char *sets(int, char *, int);
char *setsn(int i, char *val, int n);
char *dup_dict(int i);
void store_dict(char *label, Token *value, int sysd);
void do_def(Token *label, Token *value, int flag);
void do_store(Token *label, Token *value);
void label_ok(char *l);
void func_known(void);
