#include <stdio.h>
#include <sys/types.h>
#include <string.h>

char *strdup(), *strchr(), *strrchr(), *getenv(), *getlogin(), *cuserid();
void *realloc(), *malloc();

/* ====== SITE-DEPENDENT DEFINES ======================================== */

#define BFSZ 1024

#define CONFIG_FILE "/cyberspace/etc/zapuser.conf"

#define MAX_LOGIN 32

/* ====== SITE-DEPENDENT MACROS ========================================= */

/* A macro like fileno() that works on DIR * arguments */

#define dir_fileno(d) dirfd(d)

/* An strerror() function */

#ifdef linux
#define strerr(n) strerror(n)
#else
extern char *sys_errlist[];
#define strerr(n) sys_errlist[n]
#endif

#ifdef linux
#define Realloc(x,y) realloc(x,y)
#else
#define Realloc(x,y) (x?realloc(x,y):malloc(y))
#endif

/* ====== USER LIST DATA STRUCTURE ====================================== */

struct udat {
	char *login;		/* Login name */
	uid_t uid;		/* uid number */
	char *home;		/* home_directory */
	char *line;		/* passwd file line (for logging) */
	char *mailbox;		/* full path of his mailbox file */
};

/* ====== GLOBAL VARIABLES ============================================== */

extern struct udat *user;
extern int nuser, usersz;
extern int print_errors, verbose;
extern char *log_file, *err_file, *mathom_dir, *immortal_file, *outbound_file;
extern int batch_size, sleep_time, min_uid;
extern char *curr_user, *myid;

/* ====== FUNCTION PROTOTYPES =========================================== */

/* zapuser.c */
int in_user_list(char *u);

/* member.c */
void protect_group(char *gname);
void find_members(void);
int is_member(char *user, gid_t gid);

/* rmdir.c */
int delete_directory(uid_t uid, char *dir);

/* log.c */
void error(char *fmt,...);
void zulog(char *pwline, char *why);

/* pwadm.c */
FILE *start_pwadm(void);
void end_pwadm(void);

/* config.c */
char *firstin(char *s, char *l);
char *firstout(char *s, char *l);
void read_config(void);

/* homedir.c */
void dir_prefix(char *prefix);
int dir_ok(char *dir);

/* mvdir.c */
char *move_directory(char *homedir);
void move_mail(char *mailfile, char *deldir);

/* mathom.c */
void delete_mathom(void);

/* immortal.c */
void load_immortals(void);
int is_immortal(char *name);
void free_immortals(void);

/* outbound.c */
void reap_outbound(void);
