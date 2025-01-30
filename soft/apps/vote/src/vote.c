/*
 * vote.c
 * ------------------------------------------
 * Copyright (c) 1997 by John H. Remmers
 * ------------------------------------------
 * $Id: vote.c 1626 2017-09-18 19:17:23Z cross $
 * ------------------------------------------
 * Library of routines for vote program(s).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include    "vote.h"


static void     getcands(int *, int *, CandRec **);	/* get candidate info */
static int      polls_open(void);	/* check if polls are open */
static int      votedfor(char *);	/* check if voter voted for person */
static void     rndorder(int *, int);	/* generate random perm */

int             err;		/* error code */

/*
 * ------------------------------------------------------------
 * Some useful file & path routines.
 * 
 * getfile() read a whole file into a memory buffer and returns
 * a pointer to the buffer.
 */
static char *
getfile(char *file)
{
	struct stat     buf;	/* buffer for stat() call */
	long int        fsize;	/* size of file */
	long int        fgot;	/* returned by fread() */
	char           *bp;	/* pointer to file buffer */
	FILE           *fp;	/* file pointer */

	/* Get file size */
	if (stat(file, &buf)) {
		err = ERR_STATFILE;
		return NULL;
	}
	fsize = buf.st_size;

	if ((bp = malloc(fsize + 1)) == NULL) {
		err = ERR_ALLOCFILE;
		return NULL;
	}
	/* Read the file */
	if ((fp = fopen(file, "r")) == NULL) {
		err = ERR_OPENFILE;
		free(bp);
		return NULL;
	}
	fgot = fread(bp, 1, fsize, fp);
	if (fgot != fsize) {
		err = ERR_FREADFILE;
		free(bp);
		return NULL;
	}
	err = 0;
	return bp;
}

/*
 * ------------------------------------------------------------
 * Validate the user. Sets global 'nobody' flag and 'voter' id.
 */
int nobody;		// Flag: 1 if running as 'nobody'
char *voter;		// Voter's login id

char *
validate(void)
{
	if (getuid() == NOBODY_UID) {	// We are running on the web
		voter = getenv("REMOTE_USER");
		nobody = 1;
	} else {			// We are running on a tty
		int uid;
		struct passwd *pass;

		nobody = 0;
		uid = getuid();
		pass = getpwuid(uid);
		if (pass && uid)
			voter = pass->pw_name;
		else
			voter = NULL;
	}
	err = (voter) ? 0 : ERR_VALID;
	return voter;
}

/*
 * ------------------------------------------------------------
 * Set vote directory path.
 */
char *vdirpth;

void 
setvdir(void)
{
	if ((vdirpth = getenv("VOTEDIR")) == NULL)
		vdirpth = VDIR_DFLT;
}

/*
 * ------------------------------------------------------------
 * Record user's vote
 */
int 
record(CandRec *cr, int nc)
{
	FILE *fp;		// descriptor for user's vote file
	int hash;		// hashed vote
	int i;
	time_t t;
	char *s;

	if (chdir("votes")) {	// Cd to directory where votes are stored
		err = ERR_CDVOTES;
		return 1;
	}
	unlink(voter);
	printf("<P>%s</P>\n", voter);
	if ((fp = fopen(voter, "w")) == NULL) {
		err = ERR_WRTVOTE;
		return 1;
	}
	hash = 0;		// Write new vote file
	for (i = 0; i < nc; ++i)
		if (cr[i].votefor) {
			char *cd = cr[i].logid;
			while (*cd) {
				hash += (*cd);
				++cd;
			}
			fprintf(fp, "%-32s\n", cr[i].logid);
		}
	fclose(fp);
	chdir("..");		// Back to the main vote directory

	if (chdir(".vdir")) {	// Cd to history directory
		err = ERR_CDHIST;
		return 1;
	}
	if ((fp = fopen(voter, "a")) == NULL) {	// append to history file
		err = ERR_WRTHIST;
		chdir("..");
		return 1;
	}
	time(&t);
	s = ctime(&t);

	if (getuid() == NOBODY_UID)		// we're a web voter record
		fprintf(fp, "W%d %s", hash, s);	// hash value, date, time
	else
		fprintf(fp, "%d %s", hash, s);
	fclose(fp);
	chdir("..");

	err = ERR_NOERR;
	return 0;
}

/*
 * ------------------------------------------------------------
 * Read in database for this vote.
 */
char *vdirpth;			// Full path of vote directory

VoteRec *
getvdata(void)
{
	char *vdirpth;		// vote directory
	VoteRec *vr;		// vote record

	// Set full path of vote directory
	if ((vdirpth = getenv("VOTEDIR")) == NULL)
		vdirpth = VDIR_DFLT;
	if (chdir(vdirpth)) {
		err = ERR_CDVOTEDIR;
		return NULL;
	}
	if (!polls_open()) {
		err = ERR_POLLS_CLOSED;
		return NULL;
	}
	// Allocate a vote record
	vr = (VoteRec *)malloc(sizeof(VoteRec));
	if (vr == NULL) {
		err = ERR_VOTEREC_ALLOC;
		return NULL;
	}
	vr->voter = validate();	// Validate voter and set field
	if (vr->voter == NULL)
		return NULL;

	vr->title = getfile("title");	// Get title
	if (vr->title == NULL) {
		err = ERR_NOTITLE;
		return NULL;
	} else {
		char *p = strchr(vr->title, '\n');
		if (p)
			*p = '\0';
	}

	vr->info = getfile((nobody) ? "info.html" : "info.txt");
	// Get info file
	if (vr->info == NULL) {
		err = ERR_NOINFO;
		return NULL;
	}
	getcands(&(vr->numcands), &(vr->numslots), &(vr->clist));
	// Get candidate data
	if (vr->clist == NULL) {	// (assume err was set by getcands()
		return NULL;
	}
	err = 0;

	return vr;
}

static void 
getcands(int *ncands, int *nslots, CandRec ** candrec)
{
	char *cp;	// ptr to "candlist" buffer
	char *bp;	// current position in buffer
	int nc;		// number of candidates
	int ns;		// number of slots
	CandRec *cr;	// pointer into candrec array
	int i;		// array index for cr
	int *p;		// random permuation

	if ((cp = getfile("candlist")) == NULL) {
		err = ERR_OPENCANDLIST;
		*candrec = NULL;
		return;
	}
	// Get number of candidates
	if ((bp = strchr(cp, '\n')) == NULL) {
		err = ERR_CANDFMT;
		*candrec = NULL;
		return;
	}
	*bp = '\0';
	if (sscanf(cp, "%d", &nc) != 1) {
		err = ERR_CANDFMT;
		*candrec = NULL;
		return;
	}
	*ncands = nc;
	cp = bp + 1;

	// Get number of slots
	if ((bp = strchr(cp, '\n')) == NULL) {
		err = ERR_CANDFMT;
		*candrec = NULL;
		return;
	}
	*bp = '\0';
	if (sscanf(cp, "%d", &ns) != 1) {
		err = ERR_CANDFMT;
		*candrec = NULL;
		return;
	}
	*nslots = ns;
	cp = bp + 1;

	// Generate random perm of range 0..(nc-1)
	p = calloc(nc, sizeof(int));
	if (p == NULL) {
		err = ERR_PERMUTE;
		return;
	}
	rndorder(p, nc);

	// Get candidate info
	if ((cr = (CandRec *)calloc(nc, sizeof(CandRec))) == NULL) {
		err = ERR_ALLOCCANDLIST;
		*candrec = NULL;
		return;
	}
	*candrec = cr;

	for (i = 0; i < nc; ++i) {
		cr = *candrec + p[i];
		if ((bp = strchr(cp, ':')) == NULL) {	// Get full name
			err = ERR_CANDFMT;
			*candrec = NULL;
			return;
		}
		*bp = '\0';
		cr->name = cp;
		cp = bp + 1;

		if ((bp = strchr(cp, ':')) == NULL) {	// Get login
			err = ERR_CANDFMT;
			*candrec = NULL;
			return;
		}
		*bp = '\0';
		cr->logid = cp;
		cp = bp + 1;

		if ((bp = strchr(cp, '\n')) == NULL) {	// Get statement path
			err = ERR_CANDFMT;
			*candrec = NULL;
			return;
		}
		*bp = '\0';
		cr->stmt = cp;
		cp = bp + 1;

		switch (votedfor(cr->logid)) {
		case 1:
			cr->votefor = 1;
			break;
		case 0:
			cr->votefor = 0;
			break;
		default:
			*candrec = NULL;
			return;
		}
	}
}

/*
 * ------------------------------------------------------------
 * check if voter voted for login
 */
static int 
votedfor(char *login)
{
	FILE *vf;	// voter's vote file
	char vbuf[80];	// vote file line buffer
	int ret;	// value to return
	static char *alphanum = "abcdefghijklmnopqrstuvwxyz0123456789";

	if (chdir("votes")) {
		err = ERR_CDVOTES;
		return -1;
	}
	ret = 0;
	if ((vf = fopen(voter, "r")) != NULL)
		while ((!ret) && fgets(vbuf, 80, vf)) {
			*(vbuf + strspn(vbuf, alphanum)) = '\0';
			ret = !strcmp(vbuf, login);
		}

	chdir("..");
	return ret;
}

/*
 * ------------------------------------------------------------
 * Generate a random permutation p of 1..n
 */
static void 
rndorder(int *p, int n)		// p = n-elt array to hold perm
{
	int i, j, itmp;
	long tmp, *rnd = (long *)calloc(n, sizeof(long int));

	//
	// initialize p to identity perm and rnd to random sequence
	//
	for (i = 0; i < n; ++i) {
		p[i] = i;
		rnd[i] = arc4random();
	}
	// sort rnd & re-arrange p in sync
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

/*
 * ------------------------------------------------------------
 * Check if the polls are open
 */
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
