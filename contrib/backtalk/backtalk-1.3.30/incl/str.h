/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

char *firstin(char *, char *);
char *firstout(char *, char *);
char *nthchar(char *s, int n, char c);
void reflect(char *, char *);
int inlist(char *, char *, char *);
void nodotdot(char *path);
int match(char *name, char *pat, int null_term);
char *version_string(char *bf);
int sb_name(char *name, char **source, char **binary);
char *cgiquote(char *input);
char *jsquote(char *input);
void capall(char *p);
void capfirst(char *p);
/*char *strndup(char *str, int n);*/
int strcnt(char *s, char c);
#ifdef HIDE_HTML
void nometa(char *p);
#endif
void unslash(char *input);
#ifndef HAVE_STRERROR
char *strerror(int);
#endif
#ifndef HAVE_STRDUP
char *strdup(char *);
#endif
#ifdef MYSTRDUP
#define strdup(x) mystrdup(x)
char *mystrdup(char *);
#endif
#ifndef HAVE_STRSTR
char *lstrstr(char *, char *);
#define strstr(a,b) lstrstr((char *)(a), (char *)(b))
#endif
#ifndef HAVE_MEMMOVE
char *lmemmove(char *, char *, int);
#define memmove(x,y,n) lmemmove((char *)(x), (char *)(y), n)
#endif
#ifndef HAVE_MEMSET
char *lmemset(char *, int, int);
#define memset(x,c,n) lmemset((char *)(x), c, n)
#endif
#ifndef HAVE_MEMCMP
int lmemcmp(char *, char *, int);
#define memcmp(x,y,n) lmemcmp((char *)(x), (char *)(y), (int)n)
#endif
#ifndef HAVE_STRCASECMP
int lstrcasecmp(unsigned char *s1, unsigned char *s2);
#define strcasecmp(a,b) lstrcasecmp((unsigned char *)(a), (unsigned char *)(b))
int lstrncasecmp(unsigned char *s1, unsigned char *s2, int n);
#define strncasecmp(a,b,n) lstrcasecmp((unsigned char *)(a), (unsigned char *)(b), (int)n)
#endif
#ifndef HAVE_MKDIR
int mkdir(char *path, int mode);
#endif
#ifndef HAVE_VPRINTF
#if __STDC__
int vprintf(const char *fmt, ...);
int vfprintf(FILE *fp, const char *fmt, ...);
#else
int vfprintf(), vprintf();
#endif
#endif
