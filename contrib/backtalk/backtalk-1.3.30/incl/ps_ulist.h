/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int in_ulist(char *id, char *file);
int in_glist(char *id, gid_t gid, char *file);
int cpass_ok(char *cpass);
#ifdef KEEP_ULIST
void add_ulist(char *id);
#endif
void func_check_partic(void);
void func_first_partic(void);
void func_next_partic(void);
