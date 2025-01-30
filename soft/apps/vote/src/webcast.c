/*
 * webcast.c ------------------------------------------ Copyright (c) 1997 by
 * John H. Remmers ------------------------------------------ $Id: webcast.c
 * 1311 2013-01-18 15:31:57Z cross $
 * ------------------------------------------ Cast vote via the web.
 * 
 * This is a CGI that reads the vote form submitted by the user and does the
 * right thing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vote.h"
#include "formvars.h"

static void     errnodata(void);/* couldn't get election data */
static void     errnoform(void);/* couldn't get election data */
static void     dspchoices(CandRec *, int);	/* display user's choices */
static void     toomany(int, int);	/* voted for too many candidates */
static void     recorderr(char *);	/* error in recording votes */

/*
 * ------------------------------------------------------------
 */
int 
main()
{
	VoteRec        *vr;	/* vote record */
	int             i;	/* index for candidate list */
	CandRec        *cr;	/* ptr to cand. record array */
	char          **vartab;	/* table of (name,value) pairs */
	int             nc;	/* number of candidates */
	int             ns;	/* number of slots */
	int             nv;	/* number of folks user voted for */


#ifndef NOVAL
	if (getuid() != NOBODY_UID) {
		printf("Why, you can't run this program.\n");
		printf("You're not 'nobody'!\n");
		exit(1);
	}
#endif

	vr = getvdata();	/* Get vote record */
	if (vr == NULL) {
		errnodata();
		exit(1);
	}
	vartab = getcgivars();	/* Decode the form */
	if (vartab == NULL) {
		errnoform();
		exit(1);
	}
	cr = vr->clist;		/* Extract candidate list */
	nc = vr->numcands;	/* Number of candidates */
	ns = vr->numslots;	/* Number of open slots */

	nv = 0;			/* Record votes in CandList array */
	for (i = 0; i < nc; ++i) {
		char           *login;	/* Candidate login id */

		cr[i].votefor = 0;
		for (login = cgival("cand"); login; login = cgival(NULL)) {
			if (!strcmp(login, cr[i].logid)) {
				++nv;
				cr[i].votefor = 1;
				break;
			}
		}
	}

	if (nv > ns) {		/* Did user vote for too many? */
		toomany(nv, ns);
		exit(1);
	}
	if (record(cr, nc)) {	/* Record user's vote on disk */
		switch (err) {
		case ERR_CDVOTES:
			recorderr("Can't chdir to 'votes'");
			break;
		case ERR_WRTVOTE:
			recorderr("Can't open voter file");
			break;
		case ERR_CDHIST:
			recorderr("Can't chdir to history");
			break;
		case ERR_WRTHIST:
			recorderr("Can't append to history");
			break;
		default:
			recorderr("Unknown software error");
			break;
		}
		exit(1);
	}
	dspchoices(cr, nc);	/* Display choices back to user */

	return (EXIT_SUCCESS);
}

static void 
errnodata(void)
{				/* couldn't get election data */
	printf("<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n", ERRTITLE);
	printf("%s\n", BODY_TAG);
	printf("%s\n", ERRHEADING);
	printf("<P>The election data could not be read.\n");
	printf("<P>To report this, please mail the %s.\n", ADMINLINK);
	printf("<HR>\n");
	printf("Return to the %s\n", RETURNLINK);
	printf("</BODY></HTML>\n");
}

static void 
errnoform(void)
{				/* couldn't get election data */
	printf("<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n", ERRTITLE);
	printf("%s\n", BODY_TAG);
	printf("%s\n", ERRHEADING);
	printf("<P>Your vote could not be read.\n");
	printf("<P>To report this, please mail the %s.\n", ADMINLINK);
	printf("<HR>\n");
	printf("Return to the %s\n", RETURNLINK);
	printf("</BODY></HTML>\n");
}

static void 
toomany(int nv, int ns)
{				/* voted for too many candidates */
	printf("<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n", ERRTITLE);
	printf("%s\n", BODY_TAG);
	printf("%s\n", ERRHEADING);
	printf("<P>You voted for %d candidates.\n", nv);
	printf("The maximum number is %d.\n", ns);
	printf("Your vote was not recorded. "
	       "Any previous vote was left unchanged.\n");
	printf("<HR>\n");
	printf("Return to the %s\n", RETURNLINK);
	printf("</BODY></HTML>\n");
}

static void 
recorderr(char *msg)
{				/* vote not recorded */
	printf("<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n", ERRTITLE);
	printf("%s\n", BODY_TAG);
	printf("%s\n", ERRHEADING);
	printf("<P>A software error occurred. Your vote was not recorded.\n");
	printf("<P>%s\n", msg);
	printf("<P>To report this, please mail the %s.\n", ADMINLINK);
	printf("<HR>\n");
	printf("Return to the %s\n", RETURNLINK);
	printf("</BODY></HTML>\n");
}

static void 
dspchoices(CandRec * cr, int nc)
{				/* display vote result */
	int             i;

	printf("<HTML><HEAD><TITLE>%s</TITLE></HEAD>\n", RECTITLE);
	printf("%s\n", BODY_TAG);
	printf("<H1>Your vote was successfully recorded.</H1>\n");
	printf("<P>You voted for the following candidates:<BR>\n");
	printf("<UL>\n");
	for (i = 0; i < nc; ++i)
		if (cr[i].votefor == 1)
			printf("<LI>%s (%s)\n", cr[i].name, cr[i].logid);
	printf("</UL>\n");
	printf("<HR>\n");
	printf("Return to the %s\n", RETURNLINK);
	printf("</BODY></HTML>\n");
}
