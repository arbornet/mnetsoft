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

/* Pause display */
void 
votepause()
{
	printf("Type <return> to continue...");
	getchar();
}
