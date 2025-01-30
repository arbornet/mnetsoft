/* vote.h
 * ------------------------------------------
 * Copyright (c) 1997 by John H. Remmers
 * ------------------------------------------
 * $Id: vote.h 1006 2010-12-08 02:38:52Z cross $
 * ------------------------------------------
 * Header file for modules that use the vote
 * library.
 */

/* ------------------------------------------------------------
 * ERRORS
 *
 * We try to define a unique code and message for every kind of
 * error, mainly so that the web version can produce an informative
 * page.
 */
extern int err;                 /* error code */

#define ERR_NOERR   0           /* no error occurred */
#define ERR_VALID   1           /* couldn't validate user */
#define ERR_VOTEREC_ALLOC 2     /* couldn't allocate voting record */
#define ERR_NOTITLE 3           /* couldn't get vote title */
#define ERR_NOINFO 4            /* couldn't get info file */
#define ERR_CDVOTEDIR 5         /* couldn't cd to vote directory */
#define ERR_STATFILE 6          /* couldn't stat file */
#define ERR_ALLOCFILE 7         /* couldn't allocate file buffer */
#define ERR_OPENFILE 8          /* couldn't open file */
#define ERR_FREADFILE 9         /* error fread-ing file */
#define ERR_NUMCANDS 10         /* error in number of candidates */
#define ERR_FGETS 11            /* error in fgets */
#define ERR_NUMSLOTS 12         /* error in number of slots */
#define ERR_OPENCANDLIST 13     /* couldn't open candlist file */
#define ERR_ALLOCCANDLIST 14    /* couldn't alloc memory for candlist */
#define ERR_CANDFMT 15          /* error in candlist file format */
#define ERR_CDVOTES 16          /* couldn't cd to 'votes' directory */
#define ERR_PERMUTE 17          /* couldn't alloc permutation vector */
#define ERR_WRTVOTE 18          /* can't open or write users' vote file */
#define ERR_CDHIST 19           /* can't cd to history directory */
#define ERR_WRTHIST 20          /* can't write to voter's history file */
#define ERR_POLLS_CLOSED 21     /* the polls are closed */

/* ------------------------------------------------------------
 * WEB BALLOT
 *
 * When the voter "enters" the voting booth, they are presented
 * with a page containing voting information and a ballot.
 *
 */
#define TITLE       "Grex Voting Booth" /* <TITLE> */
#define BODY_TAG    "<BODY " \
                    "TEXT=\"#000000\" BGCOLOR=\"#FFFFFF\" " \
                    "LINK=\"#CC0000\" VLINK=\"#0066CC\" " \
                    "ALINK=\"#FF3300\">"
#define LOGO_URL    "http://www.hvcn.org/info/grex/minigrx1.gif"
#define LOGO_IMG    "<IMG SRC=\"" LOGO_URL "\" ALT=\"\" " \
                    "WIDTH=36 HEIGHT=18 ALIGN=TOP BORDER=0>"
#define WELCOME     "Welcome to the Grex Voting Booth"
#define HEADING     "<H1><CENTER>" LOGO_IMG "&nbsp;" \
                    WELCOME "&nbsp;" LOGO_IMG "</CENTER></H1>"
#define IDLABEL     "Your Grex login id is:"
#define STMTLABEL   "To see statements by the candidates, " \
                    "<A HREF=\"webstmts\">select this link</A>."
#define FOOTER      "Voting software copyright &copy; 1997-2005 " \
                    "by John H. Remmers " \
                    "<I>(<A HREF=\"mailto:remmers@cyberspace.org\">" \
                    "remmers@cyberspace.org</A>)</I>"
#define BALHDR_FMT  "There are %d candidates for %d positions. "\
                    "Candidates you've voted for are marked with a (*). " \
                    "Vote for the candidates of your choice " \
                    "by selecting their names on the form below.\n"
#define FORMHDR_FMT "<B>You may vote for at most %d candidates.</B><BR>\n"
#define CTRL_CLICK  "(You may need to control-click or command-click to select multiple candidates.)<BR>\n"
#define FORM_TAG    "<FORM ACTION=\"castvote\" METHOD=\"post\">"
#define SELTAG_FMT  "<SELECT NAME=\"cand\" SIZE=\"%d\" MULTIPLE>\n"
#define OPTTAG_FMT  "<OPTION VALUE=\"%s\">"
#define INP_SUBMIT  "<INPUT TYPE=\"submit\" " \
                    "VALUE=\"Cast my ballot now\">"
#define INP_RESET   "<INPUT TYPE=\"reset\" " \
                    "VALUE=\"Clear my selections\">"
#define STMTS_TITLE "Statements by the Candidates"
#define STMTS_HDR   "<H1><CENTER>" LOGO_IMG "&nbsp;" \
                    STMTS_TITLE "&nbsp;" LOGO_IMG "</CENTER></H1>"
#define RECTITLE    "Vote Recorded"                    

/* ------------------------------------------------------------
 * WEB ERROR REPORTING
 *
 * When an error occurs on the web, we display a page with an
 * error message.
 */
#define ERRTITLE    "Vote Error"
#define ERRWELCOME  "A voting error occurred"
#define ERRHEADING  "<H1><CENTER>" LOGO_IMG "&nbsp;" \
                    ERRWELCOME "&nbsp;" LOGO_IMG "</CENTER></H1>"
#define ADMINLINK   "<I><A HREF=\"mailto:remmers@cyberspace.org\">" \
                    "voting administrator</A></I>"
#define RETURNLINK  "<A HREF=\"voting-booth\">voting booth</A>"

/* ------------------------------------------------------------
 * VOTER
 *
 * How we identify and authenticate the voter depends on whether
 * we are running on the web or on a tty. In the former case we
 * assume server basic authentication and get the user's id from
 * the environment variable REMOTE_USER. In the latter case, the
 * user is logged in and we use getpwnam().
 */
#define NOBODY_UID  67          /* nobody's uid - EDIT THIS! */
extern int nobody;              /* Flag: 1 if running as 'nobody' */
extern char *voter;             /* Voter's login id */
char *validate(void);           /* Validate - set 'nobody' and 'voter' */

/* ------------------------------------------------------------
 * DIRECTORY STRUCTURE
 *
 * The files pertinent to a vote reside in a "voting directory"
 * whose path is the value of the environment variable VOTEDIR,
 * or '/var/spool/vote' if VOTEDIR is not set. Files in this
 * directory:
 *
 *    candlist        - list of candidates. Format:
 *                        1st line: number of open slots
 *                        2nd line: number of candidates
 *                        each remaining line: specs for a
 *                         candidate, in the format
 *                             FULLNAME:LOGIN:STMTPATH
 *    info.txt        - plaintext info message
 *    info.html       - html info message
 *    title           - short title for this vote
 *
 * Votes are stored in subdirectory 'votes'. Each voter gets their
 * own file recording their vote.
 *
 * The directory .log contains a history file for each voter.
 */
#define VDIR_DFLT   "/var/spool/vote"
extern char *vdirpth;           /* Full path of vote directory */

typedef struct candrec {        /* Memory structure for a candidate */
    char *name;                 /*   - full name */
    char *logid;                /*   - login id */
    char *stmt;                 /*   - path to statement */
    int votefor;                /*   - flag: 1 iff user voted for them */
} CandRec;

typedef struct voterec {        /* Memory structure for vote */
    char *voter;                /*   - login id of voter */
    char *title;                /*   - vote title */
    char *info;                 /*   - info (text or html) */
    int numcands;               /*   - number of candidates */
    int numslots;               /*   - number of slots */
    CandRec *clist;             /*   - array of candidate info */
} VoteRec;

void setvdir(void);             /* Set vote directory */
VoteRec *getvdata(void);        /* Read in data for vote and user */
int record(CandRec *, int);     /* Record user's vote */
