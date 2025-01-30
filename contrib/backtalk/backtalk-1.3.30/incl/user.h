/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int getuser(int);
char *user_fname(void);
char *user_id(void);
char *user_dir(void);
int user_uid(void);
int user_cfadm();
int user_gradm();
int user_gid(void);
char *user_gname(void);
#ifdef CLEANUP
void free_user(void);
#endif
