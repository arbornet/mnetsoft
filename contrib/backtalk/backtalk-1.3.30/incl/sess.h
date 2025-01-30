/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

char *showopt_session_module;

#ifdef ID_SESSION
char *new_session(char *user, char *ip);
int session_lookup(char *sid, char *ip, char *user);
void session_delete(char *sid);

#define SESSION_ID_LEN 24
#endif
