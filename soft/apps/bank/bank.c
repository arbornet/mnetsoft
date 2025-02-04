#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define OPENDELAY 1    /* Days to wait before opening an account */
#define INITBALANCE 10 /* How much to give new account openers */

/* Files where bank records are kept */
const char *transfile = "/arbornet/var/bank/trans"; /* Transaction records */
const char *bankfile = "/arbornet/var/bank/bank";   /* Current balances */
const char *bankers = "/arbornet/etc/bankers";      /* List of Bankers */

const char *translock = "/arbornet/var/bank/t.lock";
const char *banklock = "/arbornet/var/bank/b.lock";

/* Names of monetary units and of the bank */
const char *bankname = "the First National Bank of M-Net";
const char *unit = "gribbly";
const char *units = "gribblies";
const char *abbr = "gb";

/* Structure of transaction file entry */
struct trans_ent {
	uid_t to_uid;   /* Uid of person affected */
	char code;      /* See below */
	char fr_id[33]; /* Login id of donor */
	char why[40];   /* Donor's reason why */
	long amount;    /* Number of units */
};

/* Transaction codes */
#define TR_NONE 0 /* Not a real entry */
#define TR_XFER 1 /* From another user */
#define TR_GIFT 2 /* Bestowal from the government */
#define TR_FINE 3 /* Fine from the government */

const char *trname[4] = {"bug", "transfer", "gift", "fine"};

/* Global variables */
int myuid;      /* Customer's uid number */
char myid[33];  /* Customer's login name */
int banker;     /* Is the customer a banker? */
int report;     /* Should I just report new transactions? */
long mybal;     /* Customer's current balance */
char buf[42];   /* Input buffer */
char hisid[42]; /* His login name */
time_t tm;      /* Current time */
int bf;         /* Bank file stream */
int tf;         /* Transaction file stream */

void checkme(void);
void parsetrans(int type, int argc, char **argv);
void dobank(void);
void asktrans(int type, char *did);
void dotrans(int type, int uid, long amt, char *reason);
void askopen(char *did);
void askclose(char *did);
void askreport(char *did);
void transaction(struct trans_ent tr);
int getwho(char *did);
void scantrans(int uid, int silent);
int balance(int uid, long *bal);
void writebalance(int uid, long bal);
void addbalance(int uid, long bal);
void openaccount(int uid);
void closeaccount(int uid);
void wealthy(void);
void listall(void);
void setlock(int which);
void unlock(int which);
void done(int code);
int am_banker(char *id);

int
main(int argc, char *argv[])
{
	struct passwd *pwd;
	int i;

	if ((bf = open(bankfile, O_RDWR | O_CREAT, 0700)) < 0) {
		printf("Error: Cannot open bank file %s\n", bankfile);
		done(1);
	}
	if ((tf = open(transfile, O_RDWR | O_CREAT, 0600)) < 0) {
		printf("Cannot open transaction file %s\n", transfile);
		done(1);
	}

	tm = time((long *)0);

	pwd = getpwuid(myuid = getuid());
	strlcpy(myid, pwd->pw_name, sizeof(myid));
	banker = am_banker(myid);

	if (argc > 1) {
		if (argv[1][0] != '-') {
			for (i = 1; i < argc; i++) {
				setpwent();
				printf("%8s ", argv[i]);
				if ((pwd = getpwnam(argv[i])) == NULL)
					printf("not found\n");
				else if (balance(pwd->pw_uid, &mybal))
					printf("has no account\n");
				else
					printf(
					    "balance = %ld%s\n", mybal, abbr);
			}
			done(0);
		}

		switch (argv[1][1]) {
		case 'w':
			wealthy();
			exit(0);
		case 'a':
			listall();
			exit(0);
		case 'r':
			report = 1;
			banker = 0;
			break;
		case 't':
			parsetrans(TR_XFER, argc, argv);
			done(0);
		case 'b':
			parsetrans(TR_GIFT, argc, argv);
			done(0);
		case 'f':
			parsetrans(TR_FINE, argc, argv);
			done(0);
		case 'o':
		case 'c':
			if (!banker) {
				printf("You're not a banker!\n");
				done(1);
			}
			if (argc != 3) {
				printf("usage: %s -%c userid\n", argv[0],
				    argv[1][1]);
				done(1);
			}
			setpwent();
			if ((pwd = getpwnam(argv[2])) == NULL) {
				printf("Can't find user %s\n", argv[2]);
				done(1);
			}
			if (argv[1][1] == 'o')
				addbalance(pwd->pw_uid, 0L);
			else
				closeaccount(pwd->pw_uid);
			done(0);
		default:
			printf("Legal options are -w -a -t ");
			if (banker)
				printf("-b -f -o -c ");
			printf("and -r\n");
			done(1);
		}
	}

	if (banker) {
		struct tm *tod;
		printf("Good ");
		tod = localtime(&tm);
		if (tod->tm_hour < 12)
			printf("Morning");
		else if (tod->tm_hour < 18)
			printf("Afternoon");
		else
			printf("Evening");
		printf(", Banker %s\n", myid);
	}

	if (!report)
		printf("Welcome to %s\n\n", bankname);

	checkme(); /* Only returns if user has open account */

	printf("Transactions since your last visit:  ");
	scantrans(myuid, 0);

	printf("\nCurrent balance: %ld%s\n", mybal, abbr);

	if (!report) {
		if (banker)
			dobank();
		else if (mybal > 0) {
			printf("\nWould you like to transfer funds to another "
			       "user? ");
			fgets(buf, 40, stdin);
			if (buf[0] != 'n' && buf[0] != 'N')
				asktrans(TR_XFER, NULL);
		}
	}
	done(0);
}

/*
 * Register the user
 */

void
checkme(void)
{
	int ch;

	switch (balance(myuid, &mybal)) {
	case 1:
		if (report) {
			printf("You have no account\n");
			done(0);
		}
		if (banker) {
			addbalance(myuid, 0L);
			mybal = INITBALANCE;
			return;
		}
		printf("Hello %s, would you like to open an account? ", myid);
		if ((ch = getchar()) == 'n' || ch == 'N') {
			printf("Goodbye then...\n");
			done(0);
		}
		if (ch != '\n')
			while (getchar() != '\n');
		openaccount(myuid);
		printf(
		    "Very well...\nThere will be a %d day delay ", OPENDELAY);
		printf("before your account can be opened.\n    Thank you!\n");
		done(0);
	case 2: /* Applicant awaiting approval */
		if (mybal > tm) {
			if (report) {
				printf("Your account will be open after %s",
				    ctime(&mybal));
				done(0);
			} else if (!banker) {
				printf("Sorry, the paperwork on your new "
				       "account has not ");
				printf(
				    "been completed\nPlease come back after %s",
				    ctime(&mybal));
				printf("    Thank you!\n");
				done(0);
			}
		}
		/* If time is up, initialize the account */
		addbalance(myuid, 0L);
		mybal = INITBALANCE;
		break;
	}
}

void
parsetrans(int type, int argc, char **argv)
{
	long amt;
	struct passwd *pwd;

	if (type != TR_XFER && !banker) {
		printf("You're not a banker!\n");
		return;
	}

	if (argc < 4 || argc > 5) {
		printf("usage: %s [-t", argv[0]);
		if (banker)
			printf(" -b -f");
		printf("] who amount [reason]\n");
		return;
	}

	setpwent();
	if ((pwd = getpwnam(argv[2])) == NULL) {
		printf("No user %s\n", argv[2]);
		return;
	}

	if ((amt = atol(argv[3])) <= 0) {
		printf("Bad amount %s\n", argv[3]);
		return;
	}

	checkme();

	if (type == TR_XFER && amt > mybal) {
		printf("You don't have that much money\n");
		return;
	}

	if (myuid == pwd->pw_uid) {
		printf("Can't transfer money to yourself\n");
		return;
	}

	if (argc == 4) {
		buf[0] = '\0';
		dotrans(type, pwd->pw_uid, amt, buf);
	} else
		dotrans(type, pwd->pw_uid, amt, argv[4]);
}

/*
 *  The banker's secretary.
 */

void
dobank(void)
{
	char *w;
	printf("\nYou may either:\n");
	printf("   Transfer some of your own money to another user\n");
	printf("   Fine another user some money\n");
	printf("   Bestow some money on another user\n");
	printf("   Open an account for another user\n");
	printf("   Close a users account\n");
	printf("   Report on an account's balance\n");
	printf("   Quit\n");

	for (;;) {
		printf(": ");
		buf[0] = '\0';
		fgets(buf, 40, stdin);
		for (w = buf; *w != '\0'; w++)
			if (*w == ' ') {
				w++;
				w[strlen(w) - 1] = '\0';
				break;
			}
		switch (buf[0]) {
		case 'T':
		case 't':
			asktrans(TR_XFER, w);
			break;
		case 'F':
		case 'f':
			asktrans(TR_FINE, w);
			break;
		case 'B':
		case 'b':
			asktrans(TR_GIFT, w);
			break;
		case 'O':
		case 'o':
			askopen(w);
			break;
		case 'C':
		case 'c':
			askclose(w);
			break;
		case 'R':
		case 'r':
			askreport(w);
		case '\n':
			break;
		case '\0':
			putchar('\n');
		case 'Q':
		case 'q':
			return;
		default:
			printf("Pardon me?\n");
			break;
		}
	}
}

/*
 * Ask user about a transaction of type type.
 */

void
asktrans(int type, char *did)
{
	int hisuid;
	long hisbal, amt;
	int n;

	/* Find out who to pay */
	if (did == NULL || *did == '\0') {
		switch (type) {
		case TR_XFER:
			printf("Transfer to");
			break;
		case TR_GIFT:
			printf("Bestow upon");
			break;
		case TR_FINE:
			printf("Fine");
			break;
		}

		printf(" whom? ");
	}
	if ((hisuid = getwho(did)) < 0)
		return;

	if (hisuid == myuid) {
		printf("Why, that's you!\n");
		return;
	}

	/* Get his account status and balance */
	switch (balance(hisuid, &hisbal)) {
	case 1:
		if (!banker) {
			printf("%s has not applied for an account\n", hisid);
			printf("However, we will open one in his name in %d "
			       "days\n",
			    OPENDELAY);
			openaccount(hisuid);
			return;
		}
		addbalance(hisuid, 0L);
		break;
	case 2:
		if (!banker && hisbal < tm) {
			printf("%s's account will be open on %s", hisid,
			    ctime(&hisbal));
			return;
		}
		addbalance(hisuid, 0L);
		break;
	}

	/* Find out how much to pay him */
	printf("How many %s? ", units);
	fgets(buf, 40, stdin);
	amt = atol(buf);

	if (amt <= 0)
		return;

	if (type == TR_XFER && amt > mybal) {
		printf("You only have %ld %s\n", mybal,
		    (mybal == 1) ? unit : units);
		return;
	}

	/* Get a short explaination message */
	for (;;) {
		printf("What for? ");
		fgets(buf, 40, stdin);
		n = strlen(buf);
		if (buf[n - 1] == '\n')
			break;
		printf("Only 40 characters, please.\n");
		while (getchar() != '\n');
	}
	buf[n - 1] = '\0';

	dotrans(type, hisuid, amt, buf);
}

/*
 * Record a transaction of type type to user uid of amount amt for reason
 * reason.
 */

void
dotrans(int type, int uid, long amt, char *reason)
{
	struct trans_ent tr;

	/* Assemble the transaction record */
	memset(&tr, 0, sizeof(tr));
	tr.code = type;
	tr.to_uid = uid;
	tr.amount = amt;
	strlcpy(tr.fr_id, myid, sizeof(tr.fr_id));
	strlcpy(tr.why, reason, sizeof(tr.why));

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	if (type == TR_XFER)
		addbalance(myuid, -amt);
	addbalance(uid, (type == TR_FINE) ? -amt : amt);

	transaction(tr);

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

/*
 * Instantly open an account for someone, speeding up the paper work.
 */

void
askopen(char *did)
{
	int hisuid;

	/* Find out whose account to open */
	if (did == NULL || *did == '\0')
		printf("Open for whom? ");
	if ((hisuid = getwho(did)) < 0)
		return;

	addbalance(hisuid, 0L);
}

void
askclose(char *did)
{
	int hisuid;

	/* Find out whose account to open */
	if (did == NULL || *did == '\0')
		printf("Close whom? ");
	if ((hisuid = getwho(did)) < 0)
		return;

	closeaccount(hisuid);
}

/*
 * Report on another user
 */

void
askreport(char *did)
{
	int hisuid;
	long amt;

	/* Find out whose account to report */
	if (did == NULL || *did == '\0')
		printf("Report on whom? ");
	if ((hisuid = getwho(did)) < 0)
		return;

	switch (balance(hisuid, &amt)) {
	case 1:
		printf("no account\n");
		break;
	case 2:
		printf("account opens on %s", ctime(&amt));
		break;
	case 0:
		printf("balance =%3ld%s\n", amt, abbr);
		break;
	}
}

/*
 * Find a free spot in the transaction file and put a new transaction in it.
 * take care to handle files that have been truncated to odd lengths by disk
 * full errors.
 */

void
transaction(struct trans_ent tr)
{
	int n;
	struct trans_ent otr;

	lseek(tf, 0L, 0);
	setlock(1);

	/* Skip to first null entry, or EOF */
	while ((n = read(tf, &otr, sizeof(otr))) == sizeof(otr) &&
	       otr.code != TR_NONE);

	/* If not at EOF, rewind to beginning of last read */
	if (n != 0)
		lseek(tf, (long)-n, 1);

	write(tf, &tr, sizeof(tr));

	unlock(1);
}

/*
 * Read in someone's name, and return his uid.  Return -1 if a bad name is
 * given.
 */

int
getwho(char *did)
{
	struct passwd *pwd;

	if (did == NULL || *did == '\0') {
		fgets(hisid, 40, stdin);
		hisid[strlen(hisid) - 1] = '\0';
	} else
		strlcpy(hisid, did, sizeof(hisid));

	setpwent();
	if ((pwd = getpwnam(hisid)) == NULL) {
		printf("No such user (%s)\n", hisid);
		return (-1);
	}
	return (pwd->pw_uid);
}

/*
 * Report all tranactions involving uid which have not been previously
 * reported.  Transaction records are set to null after they have been
 * printed.  If silent is set, nothing is printed.
 */

void
scantrans(int uid, int silent)
{
	struct trans_ent tr;
	long offset;
	int someflag = 0;

	lseek(tf, offset = 0L, 0);
	while (read(tf, &tr, sizeof(tr)) == sizeof(tr)) {
		if (tr.code != TR_NONE && tr.to_uid == uid) {
			if (!silent) {
				someflag = 1;
				printf("\n%7ld%s %s from %.8s", tr.amount,
				    abbr, trname[tr.code], tr.fr_id);
				if (tr.why[0] != '\0')
					printf(" for %.40s", tr.why);
			}

			tr.code = TR_NONE;
			lseek(tf, offset, 0);
			write(tf, &tr, sizeof(tr));
		}
		offset += sizeof(tr);
	}

	if (!someflag && !silent)
		printf("None");
}

/*
 * Place a user's bank balance in *bal.  Function returns 0 if successful,
 * 1 if that user hasn't requested an account, and 2 if the account has been
 * requested, but not opened.  In the latter case, *bal contains the date on
 * which it will be opened.
 */

int
balance(int uid, long *bal)
{

	if (lseek(bf, ((long)uid) * sizeof(long), 0) < 0)
		return (1);

	if (read(bf, bal, sizeof(long)) != sizeof(long))
		return (1);

	if ((*bal % 2) == 0)
		return ((*bal == 0) ? 1 : 2);

	*bal /= 2;
	return (0);
}

/*
 * Write a new value into a user's account.
 */

void
writebalance(int uid, long bal)
{
	if (lseek(bf, ((long)uid) * sizeof(long), 0) >= 0)
		write(bf, &bal, sizeof(long));
}

/*
 * Increase the balance of uid's account by bal.  Open it and put in the
 * initial deposit if it was not open.
 */

void
addbalance(int uid, long bal)
{
	long oldbal;
	if (lseek(bf, ((long)uid) * sizeof(long), 0) < 0)
		return;

	setlock(0);

	if (read(bf, &oldbal, sizeof(long)) != sizeof(long) ||
	    (oldbal % 2) == 0)
		oldbal = (long)INITBALANCE;
	else
		oldbal /= 2;

	writebalance(uid, (bal + oldbal) * 2 + 1);

	unlock(0);
}

/*
 * Create an account with the usual time lag.
 */

void
openaccount(int uid)
{
#define SECSPERDAY 86400L;
	time_t tm;

	tm = time((time_t *)NULL) + OPENDELAY * SECSPERDAY;
	if ((tm % 2) == 1)
		tm--;
	writebalance(uid, tm);
}

void
closeaccount(int uid)
{
	writebalance(uid, 0L);
	scantrans(uid, 1);
}

/*
 * List the 10 wealthiest users.
 */
void
wealthy(void)
{
	struct passwd *pwd;
	struct {
		int uid;
		long bucks;
	} rich[10];
	int n = 0, i;
	long new;
	int cuid;

	lseek(bf, 0L, 0);

	if (read(bf, &new, sizeof(new)) != sizeof(new))
		return;

	rich[0].uid = 0;
	rich[0].bucks = new;

	for (cuid = 1; read(bf, &new, sizeof(new)) == sizeof(new); cuid++) {
		if ((new % 2) == 0)
			continue;
		new = (new - 1) / 2;

		for (i = n; i > 0 && new > rich[i - 1].bucks; i--) {
			if (i < 10) {
				rich[i].uid = rich[i - 1].uid;
				rich[i].bucks = rich[i - 1].bucks;
			}
		}
		if (i < 10) {
			rich[i].uid = cuid;
			rich[i].bucks = new;
			if (n < 10)
				n++;
		}
	}

	for (i = 0; i < n; i++) {
		setpwent();
		if ((pwd = getpwuid(rich[i].uid)) == NULL)
			closeaccount(rich[i].uid);
		else
			printf("%2d: %8.8s%12ld%s\n", i + 1, pwd->pw_name,
			    rich[i].bucks, abbr);
	}
}

void
listall(void)
{
	struct passwd *pwd;
	long bal;
	int cuid;

	lseek(bf, 0L, 0);

	for (cuid = 0; read(bf, &bal, sizeof(bal)) == sizeof(bal); cuid++) {
		if ((bal % 2) == 0)
			continue;
		bal = (bal - 1) / 2;
		setpwent();
		if ((pwd = getpwuid(cuid)) == NULL)
			closeaccount(cuid);
		else
			printf("%8.8s%12ld%s\n", pwd->pw_name, bal, abbr);
	}
}

/*
 * Create a lockfile.  If which is 0, this is the bank lock file.  If which
 * is 1, it is the transaction lockfile.
 */

long fclock[2] = {0L, 0L};

void
setlock(int which)
{
	int lf;
	register int trys = 0;
	struct stat sb;
	const char *lockfile = which ? translock : banklock;

	/* Already locked? */
	if (fclock[which])
		return;

	/* Do exclusive open on file */
	while ((lf = open(lockfile, O_RDONLY | O_CREAT | O_EXCL, 0700)) < 0) {
		/* Examine existing lock file */
		if (stat(lockfile, &sb)) {
			printf("Can't create lockfile %s\n", lockfile);
			done(1);
		}
		/* If more than 5 minutes old, kill it */
		if (sb.st_ctime + 300 < time((long *)0)) {
			printf("Blasting old %s lockfile\n",
			    which ? "transaction" : "bank");
			if (unlink(lockfile)) {
				printf("blast failed\n");
				done(1);
			}
			continue;
		}
		/* Wait up to 15 seconds for old lockfile go away */
		else if (trys++ < 5)
			sleep(3);
		else {
			/* Give up waiting */
			printf("The bank is very busy right now.\n");
			printf("Please try later.\n");
			done(1);
		}
	}

	/* Record the time at which I create the file */
	fstat(lf, &sb);
	fclock[which] = sb.st_ctime;
	close(lf);
}

void
unlock(int which)
{
	const char *lockfile = which ? translock : banklock;
	struct stat sb;

	if (!fclock[which])
		return;

	/* Check if it is my lock file */
	if (stat(lockfile, &sb) || sb.st_ctime != fclock[which])
		printf("Warning: %s lock file has been blasted.\n",
		    which ? "transaction" : "bank");
	/* If so, blast it */
	else if (unlink(lockfile))
		printf("Warning: Can't remove lockfile %s\n", lockfile);

	fclock[which] = 0L;
}

void
done(int code)
{
	if (fclock[0])
		unlock(0);
	if (fclock[1])
		unlock(1);

	exit(code);
}

/* Determine if the user "id" is a banker */

int
am_banker(char *id)
{
	FILE *fp;
	int idlen;
	int buflen;
	char buf[21];

	if ((fp = fopen(bankers, "r")) == NULL) {
		printf("cannot read %s\n", bankers);
		return (0);
	}

	while (fgets(buf, 20, fp) != NULL) {
		buflen = strlen(buf);
		idlen = strlen(id);
		buflen -= 1;
		/*		printf("buf='%s', id='%s', %d, %d
		 * \n",buf,id,buflen,idlen); */
		if (buflen > idlen) {
			idlen = buflen;
		}
		if (!strncmp(id, buf, idlen))
			return (1);
	}
	return (0);
}
