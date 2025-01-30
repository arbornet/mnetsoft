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
 * Record user's vote
 */
int 
record(CandRec * cr, int nc)
{
	FILE           *fp;	/* descriptor for user's vote file */
	int             hash;	/* hashed vote */
	int             i;
	time_t          t;
	char           *s;

	if (chdir("votes")) {	/* Cd to directory where votes are stored */
		err = ERR_CDVOTES;
		return 1;
	}
	unlink(voter);
	printf("<P>%s</P>\n", voter);
	if ((fp = fopen(voter, "w")) == NULL) {
		err = ERR_WRTVOTE;
		return 1;
	}
	hash = 0;		/* Write new vote file */
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
	chdir("..");		/* Back to the main vote directory */

	if (chdir(".vdir")) {	/* Cd to history directory */
		err = ERR_CDHIST;
		return 1;
	}
	if ((fp = fopen(voter, "a")) == NULL) {	/* append to history file */
		err = ERR_WRTHIST;
		chdir("..");
		return 1;
	}
	time(&t);
	s = ctime(&t);

	if (getuid() == NOBODY_UID)	/* we're a web voter */
		fprintf(fp, "W%d %s", hash, s);	/* record hash value, date,
						 * time */
	else
		fprintf(fp, "%d %s", hash, s);
	fclose(fp);
	chdir("..");

	err = ERR_NOERR;
	return (0);
}

/*
 * ------------------------------------------------------------ Read in
 * database for this vote.
 */
char           *vdirpth;	/* Full path of vote directory */

VoteRec        *
getvdata(void)
{
	char           *vdirpth;/* vote directory */
	VoteRec        *vr;	/* vote record */

	/* Set full path of vote directory */
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
	/* Allocate a vote record */
	vr = (VoteRec *) malloc(sizeof(VoteRec));
	if (vr == NULL) {
		err = ERR_VOTEREC_ALLOC;
		return NULL;
	}
	vr->voter = validate();	/* Validate voter and set field */
	if (vr->voter == NULL)
		return NULL;

	vr->title = getfile("title");	/* Get title */
	if (vr->title == NULL) {
		err = ERR_NOTITLE;
		return NULL;
	} {
		char           *p = strchr(vr->title, '\n');
		if (p)
			*p = '\0';
	}

	vr->info = getfile((nobody) ? "info.html" : "info.txt");
	/* Get info file */
	if (vr->info == NULL) {
		err = ERR_NOINFO;
		return NULL;
	}
	getcands(&(vr->numcands), &(vr->numslots), &(vr->clist));
	/* Get candidate data */
	if (vr->clist == NULL) {/* (assume err was set by getcands() */
		return NULL;
	}
	err = 0;
	return vr;
}

static void 
getcands(int *ncands, int *nslots, CandRec ** candrec)
{
	char           *cp;	/* ptr to "candlist" buffer */
	char           *bp;	/* current position in buffer */
	int             nc;	/* number of candidates */
	int             ns;	/* number of slots */
	CandRec        *cr;	/* pointer into candrec array */
	int             i;	/* array index for cr */
	int            *p;	/* random permuation */

	if ((cp = getfile("candlist")) == NULL) {
		err = ERR_OPENCANDLIST;
		*candrec = NULL;
		return;
	}
	/* Get number of candidates */
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

	/* Get number of slots */
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

	/* Generate random perm of range 0..(nc-1) */
	p = calloc(nc, sizeof(int));
	if (p == NULL) {
		err = ERR_PERMUTE;
		return;
	}
	rndorder(p, nc);

	/* Get candidate info */
	if ((cr = (CandRec *) calloc(nc, sizeof(CandRec))) == NULL) {
		err = ERR_ALLOCCANDLIST;
		*candrec = NULL;
		return;
	}
	*candrec = cr;

	for (i = 0; i < nc; ++i) {
		cr = *candrec + p[i];
		if ((bp = strchr(cp, ':')) == NULL) {	/* Get full name */
			err = ERR_CANDFMT;
			*candrec = NULL;
			return;
		}
		*bp = '\0';
		cr->name = cp;
		cp = bp + 1;

		if ((bp = strchr(cp, ':')) == NULL) {	/* Get login */
			err = ERR_CANDFMT;
			*candrec = NULL;
			return;
		}
		*bp = '\0';
		cr->logid = cp;
		cp = bp + 1;

		if ((bp = strchr(cp, '\n')) == NULL) {	/* Get statement path */
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

static int 
votedfor(char *login)
{				/* check if voter voted for login */
	FILE           *vf;	/* voter's vote file */
	char            vbuf[80];	/* vote file line buffer */
	int             ret;	/* value to return */
	static char    *alphanum = "abcdefghijklmnopqrstuvwxyz0123456789";

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
