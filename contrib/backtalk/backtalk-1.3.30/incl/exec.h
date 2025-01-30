/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int getinch(void);
int ungetinch(int ch);
char *read_string(char open);
char *sread_string(char open);
Token *get_token(void);
void run(char *filename);
void interpret(void);
void exec_token(Token *t);
void rerun(Array *src);
void print_context(FILE *fp);
int pop_exec(void);
#ifdef CLEANUP
void free_exec(void);
#endif /* CLEANUP */

void func_break(void);
void func_continue(void);
void func_stop(void);
void func_chain(void);
void func_jump(void);
void func_call(void);
void func_halt(void);
void func_int_chain(void);
void func_int_include(void);
void func_int_stopped(void);
void func_exec(void);
void func_include(void);
void func_stopped(void);
void func_loop(void);
void func_while(void);
void func_repeat(void);
void func_for(void);
void func_forall(void);
void func_if(void);
void func_ifelse(void);
