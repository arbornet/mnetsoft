/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int read_val(int fd, int size, void *val);
void read_token(int fd, Token *t);
Token *read_script(char *fname);
void recompile(char *file);
char *getlistfile(int index);
#ifdef CLEANUP
void free_file_list(void);
#endif
