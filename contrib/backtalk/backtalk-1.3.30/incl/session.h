/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void func_newsession(void);
void func_killsession(void);

#ifdef ID_SESSION
int session_auth(char *user);
#define SESSIONS 1
#else
#define SESSIONS 0
#endif
