/*
 * GLOBAL VARIABLES, DEFINES AND FUNCTION DECLARATIONS
 *
 * Mar 16, 1993 - Jan Wolter:  Original version
 */

/* ====================  CONFIGURATION DATA ================ */

#ifndef PAGER
#define PAGER		"/usr/bin/more"
#endif

#ifndef CONFLIST
#define CONFLIST        "/cyberspace/grex/bbs/conflist"
#endif

#define BFSZ 1024

/* ============================================================ */

#include <stdbool.h>
#include <stdio.h>

/* A structure for item and response ranges -- needs to be fancier */
struct range {
	int first;	/* Lowest number to read */
	int last;	/* Highest number to read (-1 means "last" ) */
};

/* global variables */
extern char *progname;
extern char *confdir;
extern bool read_brandnew, read_old;
extern int read_newresp, read_forgotten;
extern struct range item, resp;
extern bool prompt_rfp;
extern bool reverse;
extern bool noresponse;
extern bool dont_page;
extern bool extracting;

/* bbsread functions */
char *firstin(char *, char *);
char *firstout(char *, char *);
char *cfpath(char *);
char *pfname(char *);
void pipeintr(int);
void readconfig(char *, char *, int *);
unsigned long atolh(char *);
int match(char *, char *);
int pfread(char *);
int open_sum(char *);
int do_pass(void);
int rfp_command(char *);
int prev_item(int *, int *);
int next_item(int *, int *);
int dispitem(FILE *, char *, int, int, int, int, int, int);
void parse_arg(char *);
int do_extract(void);
void pfnew(char *);
void pfwrite(void);
void seek_item(int);
void last_read(int, int *, bool *);
void update_item(int, int);
void usystem(char *);
void remember_item(int);
void upclose(void);
void reset_item(int);
void lost_items(int, int);
FILE *upopen(char *, char *);
void parse_range(struct range *, char *);
