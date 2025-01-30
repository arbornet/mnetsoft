/*
 * webballot.c ------------------------------------------ Copyright (c) 1997
 * by John H. Remmers ------------------------------------------ $Id:
 * webballot.c 1311 2013-01-18 15:31:57Z cross $
 * ------------------------------------------ Produce a ballot form for
 * election
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include    "vote.h"

static void     errpage(void);	/* produce error page */

/*
 * ------------------------------------------------------------ Print the
 * HTML page to stdout.. This would normally be invoked by a script that sets
 * the VOTEDIR environment variable and produces the HTTP headers.
 */
int 
main()
{
	VoteRec        *vr;	/* vote record */
	int             i;	/* index for candidate list */
	CandRec        *cr;	/* ptr to cand. record array */

#ifndef NOVAL
	if (getuid() != NOBODY_UID) {
		printf("Why, you can't run this program.\n");
		printf("You're not 'nobody'!\n");
		exit(1);
	}
#endif

	vr = getvdata();
	if (vr == NULL) {
		errpage();
		exit(0);
	}
	printf("<HTML>\n<HEAD><TITLE>%s</TITLE>\n</HEAD>\n", TITLE);
	printf("%s\n", BODY_TAG);
	printf("%s\n", HEADING);
	printf("<H2><CENTER>%s</CENTER></H2>\n", vr->title);
	printf("<H3><CENTER>%s <EM>%s</EM></CENTER></H3>\n", IDLABEL, vr->voter);

	printf("<P>%s", vr->info);
	printf("<HR>\n");

	printf(BALHDR_FMT, vr->numcands, vr->numslots);
	printf("<HR>\n");

	printf("<CENTER>\n");
	printf("%s\n", FORM_TAG);
	printf(FORMHDR_FMT, vr->numslots);
	printf(CTRL_CLICK);
	printf(SELTAG_FMT, vr->numcands);
	for (i = 0; i < vr->numcands; ++i) {
		cr = vr->clist + i;
		printf(OPTTAG_FMT, cr->logid);
		printf("%s (%s)", cr->name, cr->logid);
		if (cr->votefor)
			printf(" *");
		putchar('\n');
	}
	printf("</SELECT><BR>\n");
	printf("%s\n", INP_SUBMIT);
	printf("%s\n", INP_RESET);
	printf("</FORM>\n");
	printf("</CENTER>\n");
	printf("<HR>");

	printf("<H1><CENTER>%s</CENTER></H1>\n", STMTS_TITLE);
	for (i = 0; i < vr->numcands; ++i) {
		int             c;
		FILE           *fp;

		cr = vr->clist + i;
		printf("<H2>%s:</H2>\n", cr->name);
		printf("<BLOCKQUOTE><PRE>\n");
		if ((fp = fopen(cr->stmt, "r")) == NULL)
			printf("No statement available.\n");
		else {
			while ((c = getc(fp)) != EOF)
				putchar(c);
			fclose(fp);
		}
		printf("</PRE></BLOCKQUOTE>\n");
	}
	printf("<HR><FONT SIZE=-1>%s</FONT>\n", FOOTER);
	printf("</BODY></HTML>\n");

	return (EXIT_SUCCESS);
}

#define PE(m) {printf("%d: %s\n", err, m);}
static void 
errpage(void)
{
	extern int      err;	/* error code */

	printf("<html><head><title>%s</title></head>\n", ERRTITLE);
	printf("<body>\n");
	printf("%s\n", ERRHEADING);
	printf("<p>\n");
	switch (err) {
	case ERR_NOERR:
		PE("No error occurred");
		break;
	case ERR_VALID:
		PE("Couldn't validate user");
		break;
	case ERR_VOTEREC_ALLOC:
		PE("Couldn't allocate voting record");
		break;
	case ERR_NOTITLE:
		PE("Couldn't get vote title");
		break;
	case ERR_NOINFO:
		PE("Couldn't get info file");
		break;
	case ERR_CDVOTEDIR:
		PE("Couldn't cd to vote directory");
		break;
	case ERR_STATFILE:
		PE("Couldn't stat file");
		break;
	case ERR_ALLOCFILE:
		PE("Couldn't allocate file buffer");
		break;
	case ERR_OPENFILE:
		PE("Couldn't open file");
		break;
	case ERR_FREADFILE:
		PE("Error fread-ing file");
		break;
	case ERR_NUMCANDS:
		PE("Error in number of candidates");
		break;
	case ERR_FGETS:
		PE("Error in fgets");
		break;
	case ERR_NUMSLOTS:
		PE("Error in number of slots");
		break;
	case ERR_OPENCANDLIST:
		PE("Couldn't open candlist file");
		break;
	case ERR_ALLOCCANDLIST:
		PE("Couldn't allocate memory for candlist");
		break;
	case ERR_CANDFMT:
		PE("Error in candlist file format");
		break;
	case ERR_CDVOTES:
		PE("Couldn't cd to 'votes' directory");
		break;
	case ERR_PERMUTE:
		PE("Couldn't alloc permutation vector");
		break;
	case ERR_WRTVOTE:
		PE("Can't open or write user's vote file");
		break;
	case ERR_CDHIST:
		PE("Can't cd to history directory");
		break;
	case ERR_WRTHIST:
		PE("Can't write to voter's history file");
		break;
	case ERR_POLLS_CLOSED:
		PE("The polls are closed");
		break;
	default:
		PE("An unknown error occurred");
		break;
	}
	printf("</p>\n");

}
