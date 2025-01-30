#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <kvm.h>
#include <sys/types.h>
#include <sys/stat.h>

/* sysproc.c */

extern int      sh_id, ftpd_id;
#ifdef PROTECT_SENDMAIL
extern int      sendmail_id;
#endif
#ifdef PROTECT_POSTFIX
extern int      postfix_id;
#endif
#ifdef PROTECT_SCREEN
extern int      screen_id;
#endif
#ifdef PROTECT_RDIST
extern int      rdist_id;
#endif
#ifdef PROTECT_SSHD
extern int      sshd_id;
#endif
#ifdef PROTECT_SSH_AGENT
extern int      ssh_agent_id;
#endif
#ifdef PROTECT_IRCD
extern int	ircd_id;
#endif
#ifdef PROTECT_DOVECOT
extern int	dovecot_id;
#endif

void            init_sysproc(void);
int             is_sysproc(int i, kvm_t * kd, struct kinfo_proc * pr);
int             check_sysproc(int i, dev_t device, ino_t inode);

/* active.c */

void            init_active(void);
int             get_active(dev_t device);
void            set_active(dev_t device);
int             hashfunc(char *s, int n, int tblsize);

/* scanumtp.c */

void            scan_utmp(void);

/* ttynumber.c */

void            init_ttynumber(void);
dev_t           getdevnum(char *tty);

/* usermem.c */

void            init_usermem(void);
void            add_usermem(uid_t uid, unsigned long mem);
void            check_mem(kvm_t * kd);

/* snooze.c */
extern int      awakened;
int             snooze(time_t secs, int always);

/* loguser.c */
void            loguser(kvm_t * kd, uid_t target_uid, FILE * logfp);
void            logproc(struct kinfo_proc * pr, FILE * logfp);

/* killuser.c */
void            killuser(kvm_t * kd, uid_t target_uid, FILE * logfp);

/* config.c */
extern int      protect_uids[];

/* scanproc.c */
extern int      out_of_memory;
void            scan_proc(void);
#ifdef VERBOSE
void            write_list(void);
#endif
int             find_orph(void);
void            kill_orph(void);

/* hen.c */
void            init_hen(void);
int             is_hen(pid_t pid);
void            add_hen(pid_t pid);

/* restart.c */
#ifdef RESTART_INETD
void            run_inetd(void);
#endif
#ifdef RESTART_NAMED
void            run_named(void);
#endif

/* nprint.c */
extern int      nprint(int, const char *, ...);
extern int      nread(int, char *, size_t);

/* reboot.c */
extern int      robolisten(void);
extern void     roborje(int);

/* ps.c */
extern void     printprocs(int);

/* main.c */
extern time_t   curr_time;
extern kvm_t   *kd;
extern int      mypid;
extern FILE    *logfp;
extern long     kperpage, pagesize;

/* screen.c */
#ifdef PROTECT_SCREEN
void            init_screener();
int             is_screener(uid_t uid);
void            add_screener(uid_t uid);
#endif

#define STREQ(s1, s2) ((s1)[0] == (s2)[0] && strcmp((s1), (s2)) == 0)
