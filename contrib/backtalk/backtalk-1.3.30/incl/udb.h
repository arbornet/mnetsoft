/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

char *homedirpath(char *name);

struct passwd *getdbnam(char *name);
#if defined(ID_GETUID) && !defined(UID_MAP_FILE)
struct passwd *getdbuid(uid_t uid);
#endif

struct passwd *walkdb(int flag);
#define firstdb() walkdb(0)
#define nextdb() walkdb(1)

char *walkusr(int flag);
#define firstusr() walkusr(0)
#define nextusr() walkusr(1)
void seekusr(char *name);

char *gethomedir(char *name);
void flushusercache(void);

char *getpassword(char *login, int noshare);
#ifdef ID_SESSION
int checkpassword(char *login, char *passwd);
#endif

void change_fname(struct passwd *pw, char *newfname);
void change_email(struct passwd *pw, char *newemail);
void change_gid(struct passwd *pw, gid_t gid);
void change_passwd(struct passwd *pw, char *newpass);

void mk_directory(char *dir, char *lid);
char *mkpasswd(char *clear, int status);

int save_email(void);
