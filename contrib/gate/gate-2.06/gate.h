/* -------  Unix Version Dependent Stuff ------- */

/* ------ Includes ------ */

#include "config.h"

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef STDC_HEADERS
#include <string.h>
#include <stdlib.h>
#else
/* Honestly, we don't really support non-ANSI compilers */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr rindex
#endif
char *getenv();
void *malloc(), *realloc();
char *strchr(), *strrchr();
#endif

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

#ifdef F_TERMIOS
#include <termios.h>
typedef struct termios term_mode;
extern term_mode cooked, cbreak;
#define GTTY(fd, st)	tcgetattr(fd, (st))
#ifdef TCSASOFT
#define STTY(fd, st)	tcsetattr(fd, TCSASOFT | TCSADRAIN, (st))
#else
#define STTY(fd, st)	tcsetattr(fd, TCSADRAIN, (st))
#endif /*TCSASOFT*/
#ifdef OXTABS
#define XTABS OXTABS
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
#endif /*F_TERMIOS*/

#ifdef F_TERMIO
#include <termio.h>
typedef struct termio term_mode;
extern term_mode cooked, cbreak;
#define GTTY(fd, st)    ioctl(fd, TCGETA, (st))
#define STTY(fd, st)    ioctl(fd, TCSETAW, (st))
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
#endif /*F_TERMIO*/

#ifdef F_STTY
#include <sgtty.h>
struct tchars tch;
struct ltchars ltch;
typedef struct sgttyb term_mode;
term_mode cooked, cbreak;
#define GTTY(fd, st)    ioctl(fd, TIOCGETP, (st))
#define STTY(fd, st)    ioctl(fd, TIOCSETP, (st))
#define EOF_CHAR (tch.t_eofc)
#define ERASE_CHAR  (cooked.sg_erase)
#define KILL_CHAR   (cooked.sg_kill)
#ifdef TIOCGLTC
#define WERASE_CHAR  (ltch.t_werasc)
#define REPRINT_CHAR (ltch.t_rprntc)
#define LNEXT_CHAR   (ltch.t_lnextc)
#else
#define WERASE_CHAR  '\027'
#define REPRINT_CHAR '\022'
#define LNEXT_CHAR   '\026'
#endif
#endif /*F_STTY*/

/* define ISPELL or UNIX_SPELL */

#ifdef ASPELL
#define ISPELL
#ifndef LANG_DEFAULT
#define LANG_DEFAULT "en"
#endif
#endif

#if defined(IISPELL) || defined(OLDASPELL)
#define ISPELL
#ifndef LANG_DEFAULT
#define LANG_DEFAULT "english"
#endif
#endif

#ifdef GISPELL
#define ISPELL
#endif

#ifdef PLUS_SPELL
#define UNIX_SPELL
#endif

#ifdef SIMPLE_SPELL
#define UNIX_SPELL
#endif

/* ------ Global variables ------ */

extern int use_jump, in_cbreak, resume, mycols, mylines;
extern int lines_above, junk_above, subtask, base_col, outdent;
extern int secure, askok, backwrap, novice, hotcol, maxcol, spellcheck;
extern char language[],cmdchar[],prompt[];
extern int cant_tab, dont_tab;
extern char *progname, *filename, *BC, *UP, *CR;
extern char PC;
extern short ospeed;
extern FILE *tfp;
extern jmp_buf jenv;

/* ------ Global defines ------ */

/* Return codes recognized by PicoSpan (but ignored by Yapp) */
#define RET_ENTER 67	/* Silently enter the response */
#define RET_ABORT 66	/* Discard the response, and print a warning */
#define RET_ASKOK 58	/* Ask which of the above the user wants */

#define BUFSIZE 1024
#define LANGSIZE 60
#define HELP_MESG "Type . to exit or :help"

#define bs() xputs(BC)
#define cr() xputs(CR)

/* column after tab in column c */
#define tabcol(c) ((((c)-base_col+8)&(-8))+base_col)

#define CSEP ('S'-'@')

/* ------ Function Declarations ------ */

/* CMD.C */
void docolon(char *cmd);
void cmd_print(char *arg);
void cmd_help(char *arg);
void cmd_empty(char *arg);
void cmd_read(char *arg);
void cmd_write(char *arg);
void cmd_set(char *arg);
void cmd_quit(char *arg);
void cmd_ok(char *arg);
void cmd_subs(char *cmd);

/* FILE.C */
void read_file(char *rname, int strip);
void copy_fp(FILE *wfp);
FILE *copy_file(char *wname);
void write_file(char *wname);
void typefile(char *name);
void emptyfile(void);
int expand_tilde(char *name);
int mv_file(char *src, char *dst);

/* LINE.C */
int outcol(char *str,int n,int icol);
int ggetline(char *bf, char *wbf, int *wcol, char *prompt);
void back_to_col(int ncol);
void print_to_index(int newi);

/* MAIN.C */
int main(int argc,char **argv);
void version(void);

/* OPT.C */
void parse_opts(char *string, char *source);
void print_opts(void);
int set_prompt(char *str, char *source);
int set_outdent(int val, char *source);

/* PROC.C */
void dopipe(char *cmd);
FILE *pipe_thru(char *cmd);
int dosystem(char *cmd);
int usystem(char *cmd, int noshell, int cook, int sin, int sout);
pid_t cmd_pipe(char *cmd, int noshell, int *to, int *from);
void execcmd(char *cmd, int noshell);
FILE *upopen(char *cmd, int noshell, char *mode);
void upclose();
void upkill();

/* SPEL.C */
#ifndef NO_SPELL
void cmd_spell(char *arg);
void spell_help(int guesses);
#ifndef GISPELL
int set_language(char *str, char *source);
#endif
#endif
#ifdef UNIX_SPELL
char *homedir(void);
void addword(char *word);
#endif

/* SUBS.C */
void decode_pat(char *pattern);
FILE *make_copy(void);
int do_substitute(char *pattern, char *replace);
void do_spell(void);
void make_fail(char *pattern,int m,int *fail);
void show_match(FILE *tfp, long newline, char *text, FILE *cfp,int spell);
int cliplast(char *lastbuf, int bufsize);

/* SYST.C */

#ifdef HAVE_SIGACTION
int sigact(int signum, void(*handler)(), struct sigaction *old);
#endif
void initmodes(void);
void set_mode(int into_cbreak);
RETSIGTYPE intr(void);
RETSIGTYPE hup(void);
void done(int rc);
#ifdef SIGTSTP
RETSIGTYPE susp(void);
#endif /*SIGTSTP*/

/* TERM.C */
void initterm(void);
void xputc(char ch);
void xputs(char *s);
void nxputs(char *s,int n);
void before_backwrap(void);
void erase_eol(void);
void begin_standout(void);
void end_standout(void);

/* UTIL.C */

int openfile(char *filename, int resume);
void typetoend(int pcol);
int readyes(char *prompt);
int askquest(char *prompt, char *answers);
int isprefix(char *a, char *b);
char *firstin(char *s, char *l);
char *firstout(char *s,char *l);
char *strpcpy(char *s1, char *s2);
void do_edit(void);
void novice_error(char *str);
