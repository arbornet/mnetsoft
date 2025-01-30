/* VERSION HISTORY:
 * 12/14/95 - Last version before I started collecting version history. [jdw]
 * 06/24/97 - Added version history. [jdw]
 */

/* -------------- DEFINES ---------------- */

#define USER	0x0001		/* User Error */
#define SYSTEM  0x0002		/* System Error */
#define HOSTILE 0x0004		/* Hostile Error - someone cracking? */

/* ------------- GLOBAL VARIABLES ------------- */

extern int reject;
extern char *system_name, *welcome_html, *wnu_log;

/* From Marcus: the following is all the stuff we set up for au() to use */

extern char *loginid, *userpass, *full_name, *shell, *editor, *birthday, *sex;
extern char *email, *occupation, *interests, *computers, *address, *telephone;
extern char *edpath, *terminal, *how, *shellpath, *bang, *birthdate, cpt_pass[];

void err(int, char *, ...);
void end_err();

/* ------------- PROCEDURE DECLARATIONS ------------- */

char *getvar(char *name);
char *get_query(void);
void print_cgi_header(char *title);
void print_cgi_trailer(void);
char *firstin(char*, char *);
char *firstout(char*, char *);

/* From Marcus: */

int kwscan(char *word, char *tbl[]);
