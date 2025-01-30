/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int set_cookie(char *name, char *value, time_t expire);
void func_setcookie(void);
void send_http_headers(void);
#ifdef CLEANUP
void free_cookies(void);
#endif /*CLEANUP*/
void func_getcookie(void);
char *get_http_host();
void func_serverurl(void);
void func_backtalkurl(void);
