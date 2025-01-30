/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#ifndef AUTHIDENT_H
#define AUTHIDENT_H

#include <pwd.h>

/* Include the appropriate header files for the installed authentication and.
 * and identification modules.
 */
#include AUTH_HDR
#include IDENT_HDR

/* Don't build auth_checkpass() if we aren't using session authentication */
#ifndef ID_SESSION
#undef HAVEAUTH_CHECKPASS
#endif

/* Only build auth_walk() and auth_seek() if we lack
 * both ident_walk() and ident_walkpw() */
#if defined(HAVEIDENT_WALK) || defined(HAVEIDENT_WALKPW)
#undef HAVEAUTH_WALK
#undef HAVEAUTH_SEEK
#endif

/* Almost never build ident_getpwuid() */
#if (!defined(ID_GETUID) || defined(UID_MAP_FILE))
#undef HAVEIDENT_GETPWUID
#endif

/* Don't need ident_add() or ident_del if ident and auth databases are shared */
#ifdef SHARE_AUTH_IDENT
#undef HAVEIDENT_ADD
#undef HAVEIDENT_DEL
#endif

/* Always need ident_getpwnam() */
#ifndef HAVEIDENT_GETPWNAM
#define HAVEIDENT_GETPWNAM
#endif

/* Description */

char *showopt_auth_module;
char *showopt_ident_module;

/* Function prototypes */

#ifdef HAVEAUTH_GETPASS
char *auth_getpass(char *);
#endif
#ifdef HAVEAUTH_CHECKPASS
int auth_checkpass(char *, char *);
#endif
#ifdef HAVEAUTH_WALK
char *auth_walk(int);
#endif
#ifdef HAVEAUTH_SEEK
void auth_seek(char *);
#endif
#ifdef HAVEAUTH_NEWPASS
void auth_newpass(struct passwd *, char *);
#endif
#ifdef HAVEAUTH_ADD
void auth_add(char *,char *,int,uid_t,gid_t,char *,char *);
#endif
#ifdef HAVEAUTH_DEL
int auth_del(char *);
#endif

#ifdef HAVEIDENT_GETPWNAM
struct passwd *ident_getpwnam(char *);
#endif
#ifdef HAVEIDENT_GETPWUID
struct passwd *ident_getpwuid(uid_t);
#endif
#ifdef HAVEIDENT_WALKPW
struct passwd *ident_walkpw(int);
#endif
#ifdef HAVEIDENT_WALK
char *ident_walk(int);
#endif
#ifdef HAVEIDENT_SEEK
void ident_seek(char *);
#endif
#ifdef HAVEIDENT_FNAME
void ident_newfname(struct passwd *, char *);
#endif
#ifdef HAVEIDENT_NEWEMAIL
void ident_newemail(struct passwd *, char *);
#endif
#ifdef HAVEIDENT_NEWGID
void ident_newgid(struct passwd *, gid_t);
#endif
#ifdef HAVEIDENT_ADD
#ifdef IDENT_STOREEMAIL
void ident_add(char *,uid_t, gid_t, char *, char *, char *);
#else
void ident_add(char *,uid_t, gid_t, char *, char *);
#endif
#endif
#ifdef HAVEIDENT_DEL
void ident_del(char *name);
#endif

#endif /* !AUTHIDENT_H */
