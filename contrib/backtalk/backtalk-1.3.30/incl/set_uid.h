/* Copyright 1999, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#if SERVER_UID == CFADM_UID

#define set_httpd()
#define set_cfadm()
#define set_httpd_if_shared()
#define set_cfadm_if_shared()
#define set_httpd_if_httpd_home()
#define set_cfadm_if_httpd_home()

#else

void set_httpd(void);
void set_cfadm(void);
void set_root(void);

#ifdef SHARE_AUTH_IDENT
#define set_httpd_if_shared() set_httpd()
#define set_cfadm_if_shared() set_cfadm()
#else
#define set_httpd_if_shared()
#define set_cfadm_if_shared()
#endif

#ifdef HTTPD_OWNS_HOMEDIR
#define set_httpd_if_httpd_home() set_httpd()
#define set_cfadm_if_httpd_home() set_cfadm()
#else
#define set_httpd_if_httpd_home()
#define set_cfadm_if_httpd_home()
#endif

#endif

#ifdef ID_GETUID

void as_user(void);
void as_cfadm(void);

#else

#define as_user()
#define as_cfadm()

#endif
