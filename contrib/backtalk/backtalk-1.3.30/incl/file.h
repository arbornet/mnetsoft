/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void func_read(void);
void func_write(void);
void func_line(void);
void func_tail(void);
void func_pathname(void);
void func_directory(void);
void func_readable(void);
void func_writable(void);
void func_exists(void);
void func_filedate(void);
void func_firstdir(void);
void func_nextdir(void);
int f_nth_line(FILE *fp, int n, char *bf, int skip);
int nth_line(char *fname, int n, char *bf, int skip);
void pop_litarray(Token *t);
void func_loadvar(void);
void do_savevar(char *fname, Token *vt);
void wrttag(FILE *fp, char *name, Token *t);
void func_savevar(void);
int open_script(char *, char *);
FILE *open_cflist(int mine);
