#define LOCKFILE
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Files where bank records are kept */
char *transfile = "/arbornet/var/bank/trans"; /* Transaction records */

char *translock = "/arbornet/var/bank/t.lock";

/* Names of monetary units and of the bank */
char *bankname = "the First National Bank of M-Net";
char *unit = "gribbly";
char *units = "gribblies";
char *abbr = "gb";

/* Structure of transaction file entry */
struct trans_ent {
	int to_uid;    /* Uid of person affected */
	char code;     /* See below */
	char fr_id[8]; /* Login id of donor */
	char why[40];  /* Donor's reason why */
	long amount;   /* Number of units */
};

/* Transaction codes */
#define TR_NONE 0 /* Not a real entry */
#define TR_XFER 1 /* From another user */
#define TR_GIFT 2 /* Bestowal from the government */
#define TR_FINE 3 /* Fine from the government */

char *trname[4] = {"old", "transfer", "gift", "fine"};

/* Global variables */
int myuid;      /* Customer's uid number */
char myid[9];   /* Customer's login name */
int report;     /* Should I just report new transactions? */
char buf[42];   /* Input buffer */
char hisid[42]; /* His login name */
long tm;        /* Current time */
int tf;         /* Transaction file stream */

/*
struct tm *localtime();
long time(),atol(),lseek();
char *ctime();
*/

main(argc, argv) int argc;
char **argv;
{
	struct passwd *pwd;
	int i;

	if ((tf = open(transfile, O_RDWR | O_CREAT, 0600)) < 0) {
		printf("Cannot open transaction file %s\n", transfile);
		exit(1);
	}

	printf("Transactions since your last visit:  ");
	scantrans(myuid, 0);

	exit(0);
}

/*
 * Register the user
 */

/*
 * Report all tranactions involving uid which have not been previously
 * reported.  Transaction records are set to null after they have been
 * printed.  If silent is set, nothing is printed.
 */

scantrans(uid, silent) int uid;
int silent;
{
	struct trans_ent tr;
	struct passwd *psw;
	long offset;
	int someflag;

	lseek(tf, offset = 0L, 0);
	while (read(tf, &tr, sizeof(tr)) == sizeof(tr)) {
		someflag = tr.to_uid;
		psw = getpwuid(someflag);
		printf("\n%7ld%s %s from %0.8s to %0.8s", tr.amount, abbr,
		    trname[tr.code], tr.fr_id, psw->pw_name);
		if (tr.why[0] != '\0')
			printf(" for %0.40s", tr.why);
		offset += sizeof(tr);
	}
	printf("\n");
}
