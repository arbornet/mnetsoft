/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

char *showopt_hash_lib;

#if defined(HASH_ODBM) || defined(HASH_NDBM) || defined(HASH_GDBM) || defined(HASH_DB)
int storedbm(char *file, char *k, int klen, char *c, int clen, int create);
int fetchdbm(char *file, char *k, int klen, char *c, int cmax);
int deletedbm(char *file, char *k, int klen, int bycontent);
int walkdbm(char *file);
int firstdbm(char *k, int kmax);
int seekdbm(char *k, int klen);
int nextdbm(char *k, int kmax);
char *errdbm(void);
#endif
