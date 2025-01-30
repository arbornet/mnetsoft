/* Copyright 1996-1999, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

char *format(char *, int, int, int, char *, char *, char *);
char *format_html(char *, char *, int, int);
char *format_html2text(char *);
char* sysformat(char *);
void func_quote(void);
void func_expand(void);
void func_wrap(void);
void func_cleanhtml(void);
void func_unhtml(void);
#ifdef CLEANUP
void free_format(void);
#endif
