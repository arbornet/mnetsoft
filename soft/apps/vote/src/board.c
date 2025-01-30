/*
 * board.c  -  General-purpose program for voting on slates of candidates.
 * 
 * Version 1.0 Latest revision 3/3/92
 * 
 * 11/30/94 - changed record() to pad each vote to 32 chars.
 * 
 * 11/27/95 - added facility to view candidates' statements
 * 
 * 11/30/05 - added automated voting booth opening and closing
 * 
 * Copyright (c) 1992 by John H. Remmers All rights reserved.
 */

#include <sys/time.h>

#include <ctype.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Election file structure */
#define  BOARDDIR	"/var/spool/vote"	/* main directory */
#define  HASHDIR	BOARDDIR "/.vdir"	/* hashed votes */
#define  LONGINFO	"info.txt"	/* detailed info */
#define  CANDLIST	"candlist"	/* candidate list */
#define  VOTEDIR	"votes"	/* vote subdir */
#define  START_FILE     "start"	/* when vote starts */
#define  END_FILE       "end"	/* when vote ends */

/* Voter */
struct passwd  *pass;		/* voter */
struct passwd  *validate();	/* validate voter */

/* Candidates */
#define MAXCAND		50	/* ridiculously high max # of candidates */
#define MAXBALLOT	4096	/* space for candidate info */
#define STMTFILE        "statement"	/* statement file (in home dir) */
#define STMTROWS        23	/* maximum # of rows of statement */
#define STMTCOLS        79	/* maximum # of cols of statement */
int             numcand;	/* number of candidates */
int             numslots;	/* number of slots to be filled */
int             numvotes;	/* number of votes cast */
char            balbuf[MAXBALLOT];	/* strings pointed to in 'candlist' */
struct candtag {		/* list of candidates: */
	char           *descrip;/* candidate's name & id */
	char           *dir;	/* dir holding votes for the candidate */
	/* (subdirectory of CANDDIR) */
	FILE           *stmt;	/* candidate's statement file */
	int             uvote;	/* user's vote (1 if for, 0 if against) */
} candlist[MAXCAND];
void getcands();	/* fill candlist[] from disk */
void listcands();	/* list candidates and user's votes */

/* Voting */
#define VOTEMSG		"votemsg"	/* voting instr */
void vote();		/* gather user's vote */
void record();	/* record user's vote */
void hashrec();	/* record hash value of user's vote */

/* Menu */
void menu();		/* interactive menu for voter actions */

/* Statement menu */
void stmtmenu();	/* menu of candidates who have statements */
void dispstmt();	/* display statement of one candidate */

/* Misc */
#define  LINEMAX	128	/* max input line length */

/* Utility routines */
void display();		/* display contents of file on stdout */
void votepause();	/* pause display */
void rndorder();	/* generate a random permutation of 1..n */
void bye();		/* normal exit */
int polls_open(void);	/* check if polls are open */

int 
main()
{
	pass = validate();	/* validate user */
	printf("\n          *** Welcome to the Grex Voting Booth, %s! ***\n\n",
	       pass->pw_name);
	getcands(candlist);	/* read candidate info into structure array */
	menu();			/* allow user to choose options */
	return (EXIT_SUCCESS);
}

/* User interface */
void 
menu()
{
	int choice;			/* menu selection */
	char linebuf[LINEMAX];		/* current stdin line from fgets() */

	for (;;) {
		printf("Choose an option by number:\n\n");
		printf("  1. Election information\n");
		printf("  2. List of candidates\n");
		printf("  3. View statements by the candidates\n");
		printf("  4. Vote\n");
		printf("  5. Exit the vote program\n");
		printf("\nYour choice (1-5)? ");
		fflush(stdout);

		if (!fgets(linebuf, LINEMAX, stdin)) {
			putchar('\n');
			bye();
		}
		if (sscanf(linebuf, "%d", &choice) != 1 || choice < 1 || 5 < choice) {
			printf("Invalid option.\n");
			continue;
		}
		switch (choice) {
		case 1:
			display(LONGINFO);
			break;
		case 2:
			listcands();
			break;
		case 3:
			stmtmenu();
			break;
		case 4:
			vote();
			break;
		case 5:
			bye();
			break;
		default:
			printf("Weird internal error in menu().  Goodbye!\n");
			exit(1);
		}
	}
}

/* Validate voter & return pointer to their passwd structure */
struct passwd  *
validate()
{
	struct passwd  *pass;	/* point to user's passwd file info */
	int             uid;	/* user's UID */

	uid = getuid();		/* identify user */
	pass = getpwuid(uid);
	if (!pass || !uid) {
		printf("We don't know who you are!\n");
		exit(1);
	}
	return pass;
}

/* Get and record user's vote */
void 
vote()
{
	int i;
	char linebuf[LINEMAX], *vp;
	int savevote[MAXCAND];

	display(VOTEMSG);
	printf("Vote for at most %d:\n", numslots);
	numvotes = 0;
	for (i = 0; i < numcand; ++i)
		savevote[i] = candlist[i].uvote;
	for (i = 0; i < numcand; ++i) {
		printf("%-30s (y/n): ", candlist[i].descrip);
		if (!fgets(linebuf, LINEMAX, stdin)) {
			putchar('\n');
			printf("Unexpected EOF during voting.\n");
			printf("Previous vote (if any) unchanged.\n");
			bye();
		}
		vp = linebuf;
		while (isspace(*vp) && (*vp != '\n'))
			++vp;
		if (*vp == 'y' || *vp == 'Y') {
			++numvotes;
			candlist[i].uvote = 1;
		} else
			candlist[i].uvote = 0;
	}

	if (numvotes > numslots) {
		printf("\nYou voted for %d candidates.\n", numvotes);
		printf("There are only %d positions.\n", numslots);
		printf("Vote canceled; previous vote (if any) unchanged.\n");
		printf("Please select the vote option from the menu if you wish to vote.\n\n");
		for (i = 0; i < numcand; ++i)
			candlist[i].uvote = savevote[i];
		return;
	}
	printf("\nYou voted for the following candidates:\n\n");
	for (i = 0; i < numcand; ++i)
		if (candlist[i].uvote)
			printf("%s\n", candlist[i].descrip);
	printf("\nIs this correct (y/n)? ");
	if (!fgets(linebuf, LINEMAX, stdin)) {
		putchar('\n');
		printf("Unexpected EOF during voting.\n");
		printf("Previous vote (if any) unchanged.\n");
		bye();
	}
	vp = linebuf;
	while (isspace(*vp) && (*vp != '\n'))
		++vp;
	if (*vp == 'y' || *vp == 'Y') {
		printf("Recording vote...\n");
		record();
	} else {
		printf("Previous vote (if any) unchanged.\n");
		for (i = 0; i < numcand; ++i)
			candlist[i].uvote = savevote[i];
	}
}

/* Record user's vote on disk */
void 
record()
{
	FILE           *fp;	/* descriptor for user's vote file */
	int             hash;	/* hash value of user's vote */
	int             i;
	if (chdir(VOTEDIR)) {
		printf("Can't access vote directory in record()\n");
		exit(11);
	}
	unlink(pass->pw_name);
	fp = fopen(pass->pw_name, "w");
	hash = 0;
	for (i = 0; i < numcand; ++i)
		if (candlist[i].uvote) {
			{
				char           *cd = candlist[i].dir;
				while (*cd) {
					hash += (*cd);
					++cd;
				}
			}
			fprintf(fp, "%-32s\n", candlist[i].dir);
		}
	fclose(fp);
	hashrec(pass->pw_name, hash);
	chdir("..");
}

/* Record hash value of user's vote */
void 
hashrec(user, hash)
	char           *user;
	int             hash;
{
	FILE           *fp;
	time_t          t;
	char           *s;

	if (chdir(HASHDIR)) {
		printf("Can't sloop\n");
		exit(1);
	}
	if ((fp = fopen(user, "a")) == NULL) {
		printf("Can't gloop\n");
		exit(1);
	}
	time(&t);
	s = ctime(&t);

	fprintf(fp, "%d %s", hash, s);	/* record hash value, date, time */
	fclose(fp);

	chdir(BOARDDIR);
	chdir(VOTEDIR);
}

/* Read info on candidates from disk */
void 
getcands(candlist)
	struct candtag *candlist;
{
	FILE *candfile;		/* file of candidate info */
	FILE *userfile;		/* user's vote file */
	char candid[LINEMAX];	/* candidate id */
	int permute[MAXCAND];	/* random permutation of candidates */
	int nc, j;
	char *bbufpt;
	char linebuf[LINEMAX];
	int linelen;
	size_t bbuflen, itemlen;
	char *descripfield, *dirfield, *stmtfield;

	if (chdir(BOARDDIR)) {	/* go to election directory */
		printf("Can't access voting directory!\n");
		exit(2);
	}
	if (!polls_open()) {	/* check if polls are open */
		printf("We're sorry, the polls are closed.\n");
		exit(0);
	}
	/* report number of positions */
	if ((candfile = fopen(CANDLIST, "r")) == NULL) {
		printf("Can't open %s\n", CANDLIST);
		exit(3);
	}
	if (fscanf(candfile, "%d", &numcand) != 1) {
		printf("Can't read # of candidates from %s\n", CANDLIST);
		exit(5);
	}
	while (getc(candfile) != '\n');
	if (fscanf(candfile, "%d", &numslots) != 1) {
		printf("Can't read # of vacancies from %s\n", CANDLIST);
		exit(4);
	}
	while (getc(candfile) != '\n');

	/* read candidate info */
	rndorder(permute, numcand);
	bbufpt = balbuf;
	bbuflen = sizeof(balbuf);
	for (nc = 0; nc < numcand; ++nc) {
		if (fgets(linebuf, LINEMAX, candfile) == NULL) {
			printf("Unexpected EOF in %s\n", CANDLIST);
			exit(6);
		}
		linelen = strlen(linebuf);
		if (linebuf[linelen - 1] != '\n') {
			printf("Line too long in file %s\n", CANDLIST);
			exit(7);
		}
		descripfield = strtok(linebuf, ":");
		dirfield = strtok(NULL, ":");
		stmtfield = strtok(NULL, "\n");
		if (!(descripfield && dirfield && stmtfield)) {
			printf("Improper format in %s\n", CANDLIST);
			exit(8);
		}
		j = permute[nc];
		strlcpy(bbufpt, descripfield, bbuflen);
		candlist[j].descrip = bbufpt;
		itemlen = strlen(bbufpt) + 1;
		bbufpt += itemlen;
		bbuflen -= itemlen;
		strlcpy(bbufpt, dirfield, bbuflen);
		candlist[j].dir = bbufpt;
		itemlen = strlen(bbufpt) + 1;
		bbufpt += itemlen;
		bbuflen -= itemlen;
		candlist[j].stmt = fopen(stmtfield, "r");
		candlist[j].uvote = 0;
	}
	fclose(candfile);
	/* get user's votes and fill them in */
	if (chdir(VOTEDIR)) {
		printf("Can't access vote directory\n");
		exit(9);
	}
	numvotes = 0;
	for (nc = 0; nc < numcand; ++nc)
		candlist[nc].uvote = 0;
	if ((userfile = fopen(pass->pw_name, "r")) != NULL) {
		while (fscanf(userfile, "%s", candid) == 1) {
			int             t, j;
			t = numcand;
			j = 0;
			while (j != t)
				if (strcmp(candlist[j].dir, candid))
					++j;
				else
					t = j;
			if (t != numcand) {
				++numvotes;
				candlist[t].uvote = 1;
			}
		}
		fclose(userfile);
	}
	chdir("..");
}

/* List candidates and user's vote */
void 
listcands()
{
	int nc;

	printf("There are %d candidates for %d open positions.\n",
	       numcand, numslots);
	printf("The candidates are listed in random order.\n");
	printf("Candidates you've voted for are marked with a *.\n\n");
	for (nc = 0; nc < numcand; ++nc) {
		printf(candlist[nc].uvote ? "*" : " ");
		printf("%s (%s)\n", candlist[nc].descrip, candlist[nc].dir);
	}
	printf("\nYou have voted for %d candidates.\n\n", numvotes);
	votepause();
}

/* Display contents of file on stdout */
void 
display(char *filename)
{
	FILE *filept;
	int ch;

	if ((filept = fopen(filename, "r")) == NULL) {
		printf("Can't access file %s\n", filename);
		exit(3);
	}
	while ((ch = getc(filept)) != EOF)
		putchar(ch);
	votepause();
}

/* Generate a random permutation p of 1..n */
void 
rndorder(int *p, int n)	/* p = n-elt array to hold perm */
{
	int i, j, itmp;
	long tmp, *rnd = (long *) calloc(n, sizeof(long int));

	/*
	 * initialize p to identity perm and rnd to random sequence
	 */
	for (i = 0; i < n; ++i) {
		p[i] = i;
		rnd[i] = arc4random();
	}
	/* sort rnd & re-arrange p in sync */
	for (i = n - 1; i > 0; --i) {
		for (j = 1; j <= i; ++j) {
			if ((tmp = rnd[j]) < rnd[j - 1]) {
				rnd[j] = rnd[j - 1];
				rnd[j - 1] = tmp;
				itmp = p[j];
				p[j] = p[j - 1];
				p[j - 1] = itmp;
			}
		}
	}
	free(rnd);
}

/* Display menu of candidates who have statements */
void 
stmtmenu()
{
	int index[MAXCAND + 1];	/* index into candlist[] */
	int numstmts;		/* number of statements */
	int nc;			/* index into candlist */
	int i;			/* index into index */
	int select;		/* user-selected value of i */
	char linebuf[LINEMAX + 2];	/* current stdin line from fgets() */

	/* scan candlist[] for statments */
	numstmts = 0;
	for (nc = 0; nc < numcand; ++nc) {
		if (candlist[nc].stmt) {
			index[numstmts] = nc;
			++numstmts;
		}
	}

	/*
	 * now index[] contains indices of candidates who have statements
	 */
	printf("The following candidates have prepared statements that\n");
	printf("you can view:\n\n");
	for (;;) {
		for (i = 0; i < numstmts; ++i)
			printf("   %2d. %s\n", i + 1, candlist[index[i]].descrip);
		printf("\nTo view a statement, type the corresponding number,\n");
		printf("or type 0 to return to the main menu.\n");
		printf("Your choice? ");
		if (!fgets(linebuf, LINEMAX, stdin)) {
			putchar('\n');
			bye();
		}
		select = 0;
		sscanf(linebuf, "%d", &select);
		if (select <= 0 || select > numstmts)
			break;
		dispstmt(index[select - 1]);
		votepause();
		putchar('\n');
	}
}


void 
dispstmt(int nc)		/* Display statment of candidate nc */
{
	int r, c;	/* current row, column */
	char ch;	/* current char */
	FILE *fp;	/* ptr to statement file */
	char lbuf[STMTCOLS + 3];	/* input line buffer */

	printf("Statement by %s:\n", candlist[nc].descrip);
	printf("----------------------------------------------------------------------\n");
	fp = candlist[nc].stmt;
	rewind(fp);
	for (r = 0; r < STMTROWS; ++r) {
		if (fgets(lbuf, STMTCOLS + 1, fp) == NULL)
			break;
		for (c = 0; (c < STMTCOLS) && ((ch = lbuf[c]) != '\n') && (ch != '\0'); ++c)
			if (isprint(ch) || ch == '\t')
				putchar(ch);
		putchar('\n');
	}
	printf("----------------------------------------------------------------------\n");
}

/* Normal program exit */
void 
bye()
{
	printf("\nThank you for visiting the Grex Voting Booth.\n");
	exit(0);
}

/* Pause display */
void 
votepause()
{
	printf("Type <return> to continue...");
	getchar();
}

/* Check if the polls are open */
int 
polls_open(void)
{
	struct timeval t;
	struct timezone tz;
	FILE *start, *end;
	long s, e;

	if ((start = fopen("start", "r")) == NULL) {
		perror("start");
		return 0;
	}
	if ((end = fopen("end", "r")) == NULL) {
		perror("end");
		return 0;
	}
	if (fscanf(start, "%ld", &s) == 0) {
		fprintf(stderr, "file start: bad format\n");
		return 0;
	}
	if (fscanf(end, "%ld", &e) == 0) {
		fprintf(stderr, "file end: bad format\n");
		return 0;
	}
	gettimeofday(&t, &tz);
	if ((s <= t.tv_sec) && (t.tv_sec <= e))
		return 1;
	else
		return 0;
}
