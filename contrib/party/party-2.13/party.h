/* PARTY PROGRAM - GLOBAL DECLARATIONS */

#include "config.h"

/* Select code works very, very badly. */

#define NOSELECT

/* The name of the partytab file must be compiled into the program.  This
 * is the file that determines the rest of the system configuration.
 */

#ifndef PARTYTAB
#define PARTYTAB "/usr/local/etc/party/partytab"
#endif /*PARTYTAB*/

/* either SUID or SGID should be set, depending on whether the party
 * program is to be installed suid or sgid.
 */

#ifndef SUID
#ifndef SGID
#define SUID
#endif /*SGID*/
#endif /*SUID*/

/* MAILDELAY determines how often the program checks for new mail and for
 * idleness.  The test will be made roughly every 4*MAILDELAY seconds.
 */

#ifndef MAILDELAY
#define MAILDELAY 30
#endif /*MAILDELAY*/

/* The following options give default values for options.  Many of these are
 * now set by config.h.  All of these can be overridden by the partytab
 * file, and most can be overridden by the user in various ways.
 */

#ifndef DFLT_CHANTAB
#define DFLT_CHANTAB "/usr/local/etc/party/chantab"
#endif /*DFLT_CHANTAB*/

#ifndef DFLT_DIR
#define DFLT_DIR "/usr/local/var/party/log"
#endif /*DFLT_DIR*/

#ifndef DFLT_HELP
#define DFLT_HELP "/usr/local/etc/party/partyhlp"
#endif /*DFLT_HELP*/

#ifndef DFLT_MAKENOISE
#define DFLT_MAKENOISE "/usr/local/etc/party/noisetab"
#endif /*DFLT_MAKENOISE*/

#ifndef DFLT_WHOFILE
#define DFLT_WHOFILE "/usr/local/var/party/partytmp"
#endif /*DFLT_WHOFILE*/

#ifndef DFLT_FULLMESG
#define DFLT_FULLMESG "Sorry, Party is full.  Please try again later."
#endif /*DFLT_FULLMESG*/

#ifndef DFLT_INTRO
#define DFLT_INTRO "Welcome to PARTY!  Type '?' for help:"
#endif /*DFLT_INTRO*/

#ifndef DFLT_PROMPT
#define DFLT_PROMPT ">"
#endif /*DFLT_PROMPT*/

#ifndef DFLT_TPMORP
#define DFLT_TPMORP ""
#endif /*DFLT_TPMORP*/

#ifndef DFLT_SPACEONLY
#define DFLT_SPACEONLY "Type '?' for help."
#endif /*DFLT_SPACEONLY*/

#ifndef DFLT_SHELL
#define DFLT_SHELL "/bin/sh"
#endif /*DFLT_SHELL*/

#ifndef DFLT_BACK
#define DFLT_BACK "10"
#endif /*DFLT_BACK*/

#ifndef DFLT_COLS
#define DFLT_COLS "80"
#endif /*DFLT_COLS*/

#ifndef DFLT_FILTER
#define DFLT_FILTER ""
#endif /*DFLT_FILTER*/

#ifndef DFLT_START
#define DFLT_START "party"
#endif /*DFLT_START*/

#ifndef DFLT_READLIM
#define DFLT_READLIM "0"
#endif /*DFLT_READLIM*/

#ifndef DFLT_MAILDIR
#define DFLT_MAILDIR "/var/mail"
#endif /*DFLT_MAILDIR*/

#ifndef DFLT_KNOCKWAIT
#define DFLT_KNOCKWAIT "30"
#endif /*DFLT_KNOCKWAIT*/

#ifndef DFLT_IDLEOUT
#define DFLT_IDLEOUT "10"
#endif /*DFLT_IDLEOUT*/

#ifndef DFLT_CAPACITY
#define DFLT_CAPACITY "50"
#endif /*DFLT_CAPACITY*/

#ifndef DFLT_DEBUG
#define DFLT_DEBUG "party.debug"
#endif /*DFLT_DEBUG*/

#define BFSZ 1035

#include <paths.h>

#include <unistd.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_RANDOM
#define RAND() random()
#define MAXRAND 2147483647
#define SEEDRAND(x) srandom(x)
#else
#define RAND() rand()
#define MAXRAND 32767
#define SEEDRAND(x) srand(x)
#endif /*HAVE_RANDOM*/

/* Method to set cbreak/cooked modes */

#ifdef HAVE_TERMIOS_H
# define F_TERMIOS
#else
# ifdef HAVE_TERMIO_H
#  define F_TERMIO
# else
#  ifdef HAVE_SGTTY_H
#   define F_STTY
#  endif /* HAVE_SGTTY_H */
# endif /* HAVE_TERMIO_H */
#endif /* HAVE_TERMIOS_H */

#include <termios.h>
extern struct termios cooked, cbreak;
#define GTTY(fd, st)    tcgetattr(fd, (st))
#ifdef TCSASOFT
#define STTY(fd, st)    tcsetattr(fd, TCSASOFT | TCSANOW, (st))
#else
#define STTY(fd, st)    tcsetattr(fd, TCSANOW, (st))
#endif
#define EOF_CHAR    (cooked.c_cc[VEOF])
#define ERASE_CHAR  (cooked.c_cc[VERASE])
#define KILL_CHAR   (cooked.c_cc[VKILL])
#ifdef VREPRINT
#define REPRINT_CHAR (cooked.c_cc[VREPRINT])
#else
#define REPRINT_CHAR '\022'
#endif
#ifdef VWERASE
#define WERASE_CHAR (cooked.c_cc[VWERASE])
#else
#define WERASE_CHAR '\027'
#endif
#ifdef VLNEXT
#define LNEXT_CHAR (cooked.c_cc[VLNEXT])
#else
#define LNEXT_CHAR '\026'
#endif

#ifdef TIOCGWINSZ
#define WINDOW		/* Get terminal size from kernal */
#endif

#include <fcntl.h>
#define LOCK(fd)      setlock(fd, F_WRLCK);
#define UNLOCK(fd)    setlock(fd, F_UNLCK);

#ifdef SUID
#define CHN_MODE 0644
#define DEP_MODE 0600
#define USR_MODE 0600

#define be_user()  seteuid(real_uid)
#define be_party() seteuid(eff_uid)
#endif /*SUID*/

#ifdef SGID
#define CHN_MODE 0664
#define USR_MODE 0664
#define DEP_MODE 0660

#define be_group()  setegid(real_gid)
#define be_partyg() setegid(eff_gid)
#endif /*SGID*/

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#ifndef UT_NAMESIZE
#define UT_NAMESIZE 32
#endif
#ifndef UT_LINESIZE
#define UT_LINESIZE 16
#endif

/* UTMP is the location of the utmp file, which varies on some systems */

#ifndef UTMP
#ifdef _PATH_UTMP
#define UTMP _PATH_UTMP
#else
#define UTMP "/var/run/utx.active"
#endif
#endif

#ifdef HAVE_SIGLONGJMP
#define setjump(e,f) sigsetjmp(e,f)
#define longjump(e,v) siglongjmp(e,v)
#define jump_buf sigjmp_buf
#else
#define setjump(e,f) setjmp(e)
#define longjump(e,v) longjmp(e,v)
#define jump_buf jmp_buf
#endif

/* Option table structure */
struct optent {
        char *name;	/* Name for this option */
        char pf;	/* Syntax for this option */
        char yes;	/* Boolean value of this option 0=no, 1=yes, 2=see */
        char *str;	/* String value of this option */
        };

struct cmdent {
        char *name;	/* Name for this command */
        char abbr;	/* Minimum number of characters which must match */
        };

/* Option pf flags - how can the option be used? */
#define PF_SYS	0001		/* Can only be set in partytab */
#define PF_NO	0002		/* Can use "option" and "nooption" form */
#define PF_SEE	0004		/* Can use "seeoption" form */
#define PF_STR	0010		/* Can use "option=string" form */
#define PF_NUM	0020		/* Can use "option=number" form */
#define PF_CHN	0040		/* Reset on chn change.  Not user settable */
#define PF_SHOW	0100		/* Print even without "all" flag */

extern struct optent opt[];	/* Option structure -- see opttab.c */
extern struct cmdent cmd[];	/* Command structure -- see opttab.c */

extern int rst;			/* Party file open for read */
extern FILE *wfd;		/* Party file open for write */
extern int lfd;			/* Party file lock (not the party log) */
extern int out_fd;		/* Stream to terminal or filter */
extern char *progname;		/* Program name */
extern char *channel;		/* Current channel number (NULL is outside)*/
extern off_t tailed_from;	/* File offset we last tailed from */
extern char *version;

extern char name[];		/* User's internal name or alias */
extern char realname[];		/* User's real name (used for mail check) */
extern char logname[];		/* Name user logged in under (utmp) */
extern char logtty[];		/* /dev/ttyXX that user logged into (utmp) */
extern time_t logtime;		/* Time that the user logged in (utmp)*/

extern FILE *debug;		/* Debug output file */
extern uid_t real_uid, eff_uid;	/* Real and Effective uid */
extern gid_t real_gid, eff_gid;
extern RETSIGTYPE (*oldsigpipe)();

/* The following buffer gets used for all sorts of things in all sorts of
 * places.  Beware!
 */

#define INDENT UT_NAMESIZE+2	/* Just changing this value won't work */
extern char inbuf[BFSZ+INDENT+2]; /* Text buffer - first 10 for "name:   " */
extern char *txbuf;		/* Text buffer - pointer to respose portion */
				/*               of inbuf */

#define CHN_LEN 12		/* Maximum length of channel name */

/* Data structure used in compiling list of channels for :list command */
struct chnname {
        char name[CHN_LEN+1];	/* Name of this command */
        int users;	        /* number of users in this channel */
        struct chnname *next;   /* Next entry */
        };

#ifndef NOIGNORE
/* Structure of linked list of ignored user.  Initially null */
struct ign {
	struct ign *next;
	char name[UT_NAMESIZE+1];
	};
extern struct ign *ignoring;
#endif /*NOIGNORE*/

/* FUNCTION PROTOTYPES */

/* party.c */
void help(char *, int);
char *exptilde(char *);
void readfile(char *);
char *chn_file_name(char *, int);
char *chn_lockfile_name(char *, int);
int join_party(char *);
void setmailfile(void);
char *leafname(char *);
int badname(char *);
int convert(char *);
void setlock(int, int);
off_t backup(int);
void done(int);
RETSIGTYPE alrm(void);
RETSIGTYPE intr(void);
RETSIGTYPE term(void);
RETSIGTYPE hup(void);
#ifdef SIGTSTP
RETSIGTYPE susp(void);
#endif /*SIGTSTP*/
void db(char *, ...);
void err(char *, ...);

/* input.c */
void docmd(int);
void speak(int);
void cmd_colon(char *);
void cmd_scrollback(char *);
void cmd_join(char *);
void savelog(int, char *);
void cmd_noise(char *);
void listcmds(FILE *);
int command_code(char **);
void setrealname();
void setname(char *);
void changename(char *);
void stashname(void);
void checkname(char *);
void cmd_shell(char *buf);
int makenoise(char *);
void append(char *, FILE *, int);
void initmodes(void);
char *pgetline(char *, int, int);
RETSIGTYPE setcols(void);

/* output.c */
int output(void);
void initoutput(void);
void bufwrite(int);
void bputc(int, char);
void bputn(int, char *, int);
void bufopen(int);
void bufflush(int);
int bgetc(int in);
char *bgets(int in);
void expand(int);
void wrap(int);
void eatline(int);
int linecmp(char *, char *);
char *linecpy(char *, char *);
int linelen(char *);
char *lineindex(char *, char);
RETSIGTYPE setcol(void);

/* proc.c */
void start_filter(void);
void stop_filter(void);
void kill_filter(void);
FILE *upopen(char *, char *);
void upclose(void);
void upkill(void);
void printexec(FILE *, char *);
void usystem(char *cmd);
void execcmd(char *cmd);

/* users.c */
int who_enter(void);
int who_open(void);
void who_exit(void);
void who_chan(void);
void who_shout(char *);
void who_shin(void);
int who_isout(void);
void who_list(FILE *, char);
#ifndef NOCLOSE
void who_ison(FILE *, char *);
#endif
void who_clear(off_t, char *);
int who_empty(char *);
int who_count(void);
struct chnname *who_clist(struct chnname *);
int who_uniqalias(char *, char *, char *);
void ncstrncpy(char *, char *, int);
int finduser(char *, char *, time_t *);

/* opt.c */
void readtab(void);
int chnopt(char *);
int hasdefval(int);
void initopts(void);
void parseopts(char *, int);
void setnum(int,int);
void setstr(int, char *, int);
int printopts(FILE *, int, char, char *);
int inlist(char *, char *);
char *firstin(char *, char *);
char *firstout(char *, char *);
int listchn(void);
struct chnname *addchn(struct chnname *, char *, int);
FILE *openchn(void);
int patmatch(char *, char *);
void set_debug(void);

/* close.c */
#ifndef NOCLOSE
char *usr_file_name(char *);
int check_open(char *);
int is_closed(void);
void open_chan(void);
void close_chan(void);
void invite(char *);
int read_yn(void);
int knock(char *);
int enter_closed(char *);
#endif /* !NOCLOSE */

/* ignore.c */
#ifndef NOIGNORE
int addignore(char *);
int delignore(char *);
void noignore(void);
void listignore(void);
int am_ignoring(char *);
int ignore_line(char *);
#endif /* !NOINGORE */
