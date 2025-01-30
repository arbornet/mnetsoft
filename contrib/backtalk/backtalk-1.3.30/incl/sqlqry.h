/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "sql.h"

#if defined(SQL_Q_CREATE)
void qry_auth_create();
#endif

#if defined(HAVEAUTH_GETPASS)
SQLhandle qry_auth_getpass(char *login);
#endif

#if defined(HAVEAUTH_WALK)
SQLhandle qry_auth_logins(int n);
#endif

#if defined(HAVEAUTH_SEEK)
SQLhandle qry_auth_logins_after(char *login, int n);
#endif

#if defined(HAVEAUTH_NEWPASS)
void qry_auth_newpass(char *login, char *pass);
#endif

#if defined(HAVEAUTH_ADD)
void qry_auth_add(char *login, char *pass, int uid, int gid, char *fname, char *dir);
#endif

#if defined(HAVEAUTH_DEL)
void qry_auth_del(char *login);
#endif

#if defined(SQL_Q_CREATE)
void qry_ident_create();
#endif

SQLhandle qry_ident_getpwnam(char *login);

#if defined(HAVEIDENT_GETPWUID)
SQLhandle qry_ident_getpwuid(int uid);
#endif

#if defined(HAVEIDENT_WALKPW)
SQLhandle qry_ident_users(int n);
#endif

#if defined(HAVEIDENT_WALK)
SQLhandle qry_ident_logins(int n);
#endif

#if defined(HAVEIDENT_SEEK)
SQLhandle qry_ident_logins_after(char *login, int n);
#endif

#if defined(HAVEIDENT_NEWFNAME)
void qry_ident_newfname(char *login, char *fname);
#endif

#if defined(HAVEIDENT_NEWGID)
void qry_ident_newgid(char *login, int gid);
#endif

#if defined(HAVEIDENT_ADD)
void qry_ident_add(char *login, int uid, int gid, char *fname, char *dir);
#endif

#if defined(HAVEIDENT_DEL)
void qry_ident_del(char *login);
#endif

#if defined(NEXTUID_SQL)
#if defined(SQL_Q_CREATE)
void qry_uid_create(void);
#endif
SQLhandle qry_uid_next(void);
SQLhandle qry_uid_curr(void);
#endif

#if defined(SQL_Q_CREATE)
void qry_group_create();
#endif

SQLhandle qry_group_gid_to_name(int gid);
SQLhandle qry_group_name_to_gid(char *gname);
SQLhandle qry_group_in_name(char *login, char *gname);
SQLhandle qry_group_in_gid(char *login, int gid);
SQLhandle qry_group_all(int n);
SQLhandle qry_group_mine(char *login, int n);
void qry_group_add(int gid, char *gname);
void qry_group_memadd(int gid, char *login);
void qry_group_del(char *gname);
void qry_group_memdel(char *gname, char *login);


#if defined(SQL_Q_CREATE)
void qry_sess_create();
#endif

SQLhandle qry_sess_get(char *sid);
void qry_sess_refresh(char *sid, int time);
void qry_sess_new(char *sid, char *login, int time, char *ip);
void qry_sess_kill(char *sid);
void qry_sess_reap(int time);
SQLhandle qry_sess_salt();

#if defined(SQL_Q_CREATE)
void qry_baai_create();
#endif

SQLhandle qry_baai_get(char *handle);
SQLhandle qry_baai_list();
void qry_baai_add(char *handle, char *type, int size, char *login,
    int uid, char *conf, int access, int date, char *desc, int link);
void qry_baai_edit(char *handle, char *type, int size, char *login,
    int uid, char *conf, int access, int date, char *desc, int link);
void qry_baai_link(char *handle);
void qry_baai_del(char *handle);
