/* -------  Unix Version Dependent Stuff ------- */

/* Makefile should usually do -DBSD or -DSYSV */
/* but we recognize some builtin compiler defines */

#define JOBCONTROL
#define F_TERMIOS		/* Use tcsetattr() to set modes */

#include <termios.h>
#include <unistd.h>

typedef struct termios term_mode;
term_mode cooked, cbreak;
#define GTTY(fd, st)	tcgetattr(fd, (st))
#define STTY(fd, st)	tcsetattr(fd, TCSASOFT | TCSADRAIN, (st))
#define EOF_CHAR    (cooked.c_cc[VEOF])
#define ERASE_CHAR  (cooked.c_cc[VERASE])
#define KILL_CHAR   (cooked.c_cc[VKILL])
#define REPRINT_CHAR (cooked.c_cc[VREPRINT])
#define WERASE_CHAR (cooked.c_cc[VWERASE])
#define LNEXT_CHAR (cooked.c_cc[VLNEXT])

/* --------- Function Definitions ----------- */

void initmodes(void);
void set_mode(int into_cbreak);
void intr(void);
void done(int rc);
void susp(void);
