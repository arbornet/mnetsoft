#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include "sys.h"

/* Configurable Stuff */
#define DICT_DIR "/cyberspace/var/letter"
#define DEFAULT_FILE DICT_DIR"/word-05"
#define BFSZ 1024

struct {
	char *opt;
	char *path;
} optfiles[] = {
	{ "1", DICT_DIR "/word-01" },
	{ "2", DICT_DIR "/word-02" },
	{ "3", DICT_DIR "/word-03" },
	{ "4", DICT_DIR "/word-04" },
	{ "5", DICT_DIR "/word-05" },
	{ "6", DICT_DIR "/word-06" },
	{ "7", DICT_DIR "/word-07" },
	{ "8", DICT_DIR "/word-08" },
	{ "9", DICT_DIR "/word-09" },
	{ "10", DICT_DIR "/word-10" },
	{ "11", DICT_DIR "/word-11" },
	{ "12", DICT_DIR "/word-12" },
	{ "13", DICT_DIR "/word-13" },
	{ "14", DICT_DIR "/word-14" },
	{ "15", DICT_DIR "/word-15" },
	{ "16", DICT_DIR "/word-16" },
	{ "17", DICT_DIR "/word-17" },
	{ "18", DICT_DIR "/word-18" },
	{ "19", DICT_DIR "/word-19" },
	{ "20", DICT_DIR "/word-20" },
	{ NULL, NULL }
};
/* End of configurable stuff */

/* Some globals */

FILE *fp;			/* Data file */
int n;				/* Number of letters in words */
int words;			/* Number of words in data file */
int targets;			/* Number of them that can be used as target
				 * words */
char *desc;			/* Data file description (in malloced memory) */
long line1;			/* Offset to line one in data file */
long hash[26];			/* Where letters of the alphabet begin in
				 * file */
char *progname;			/* argv[0] */

struct hist {
	struct hist *next;	/* Link to next word guessed */
	short score;		/* Score (-1 if not in dictionary) */
	char word[1];		/* Word guessed - size is actually n+1 */
} *firsthist = NULL, *lasthist = NULL;


/* Empty the history */
void 
free_hist(void)
{
	struct hist *p, *q;

	for (p = firsthist; p != NULL; p = q) {
		q = p->next;
		free(p);
	}
	firsthist = lasthist = NULL;
}

/* Display the history */
void 
show_hist(void)
{
	struct hist *p;
	int i;

	for (i = 0; i < n + 4; i++)
		putchar('-');
	putchar('\n');

	for (p = firsthist; p != NULL; p = p->next)
		if (p->score < 0)
			printf(" %s not in dictionary\n", p->word);
		else
			printf(" %s %d\n", p->word, p->score);
}


/* Build a rather stupid hash */

void 
hash_file(void)
{
	char lastchar = 'a' - 1;
	char bf[BFSZ];
	int i;

	fseek(fp, line1, SEEK_SET);

	while (fgets(bf, BFSZ, fp) != NULL) {
		if (strlen(bf) != n + 12) {
			printf("%s: bad line in data file: %s\n", progname, bf);
			done(1);
		}
		if (bf[0] != lastchar) {
			if (!islower(bf[0])) {
				printf("%s: bad character '%c' in data file\n", progname, bf[0]);
				done(1);
			}
			/* Fill in gaps with zeros */
			for (i = lastchar - 'a' + 1; i < bf[0] - 'a'; i++)
				hash[i] = 0;

			/* Save new offset */
			lastchar = bf[0];
			hash[lastchar - 'a'] = ftell(fp) - (n + 12);
		}
	}
}


/* Find a word.  Returns offset if found, zero otherwise */

int 
find_word(char *word)
{
	char bf[BFSZ];

	if (hash[word[0] - 'a'] == 0)
		return 0;
	fseek(fp, hash[word[0] - 'a'], SEEK_SET);

	while (fgets(bf, BFSZ, fp) != NULL) {
		int c = strncmp(word, bf, n);
		if (c == 0)
			return ftell(fp) - (n + 12);
		if (c < 0)
			return 0;
	}
}


int 
check_word(char *guess, char *target)
{
	int i;
	struct hist *new;

	/* Is it the target word? */
	if (!strncmp(guess, target, n)) {
		printf(" %d YES!\n", n);
		return 1;
	}
	/* Save word to history list */
	new = (struct hist *) malloc(sizeof(struct hist) + n);
	strncpy(new->word, guess, n);
	new->word[n] = '\0';
	new->next = NULL;
	if (lasthist == NULL)
		firsthist = lasthist = new;
	else {
		lasthist->next = new;
		lasthist = new;
	}

	/* Check if it is in our dictionary */
	if (find_word(guess) == 0) {
		printf(" not in dictionary.\n");
		new->score = -1;
		return 0;
	}
	/* Score it */
	new->score = 0;
	for (i = 0; i < n; i++)
		if (target[i] == guess[i])
			new->score++;

	printf(" %d\n", new->score);
	return 0;
}


void 
choose_word(char *target, long *wordno)
{
	char bf[BFSZ];
	long i = random() % targets;

	fseek(fp, line1, SEEK_SET);
	*wordno = 0;
	while (fgets(bf, BFSZ, fp) != NULL) {
		if (isdigit(bf[n]) && i-- == 0) {
			strncpy(target, bf, n);
			target[n] = '\0';
			return;
		}
		(*wordno)++;
	}
}

void 
save_score(char *word, long wordno, int guesses)
{
	long offset = wordno * (n + 12) + line1;
	char bf[BFSZ + 1];
	int oldscore, i;
	char *user, *olduser;

	fseek(fp, offset, SEEK_SET);

	if (fgets(bf, BFSZ, fp) == NULL) {
		printf("Could not read data file line back.  Score not saved.\n");
		return;
	}
	if (strncmp(word, bf, n)) {
		printf("Data file change?  Score not saved.\n");
		return;
	}
	if (!isdigit(bf[n])) {
		printf("Data line lacks score slot.  Score not saved.\n");
		return;
	}
	oldscore = atoi(bf + n);

	if (oldscore > 0) {
		olduser = bf + n + 3;
		for (i = 0; i < 8; i++)
			if (olduser[i] == '*') {
				olduser[i] = '\0';
				break;
			}
		printf("You %sbeat the previous record for this word: %s %d guesses\n",
		 (guesses < oldscore) ? "" : "did not ", olduser, oldscore);

		if (guesses >= oldscore)
			return;
	}
	if ((user = getlogin()) == NULL) {
		printf("Who are you?  Score not saved.\n");
		return;
	}
	snprintf(bf, BFSZ, "%s%03d%s", word, guesses, user);
	bf[n + 11] = '\n';
	bf[n + 12] = '\0';
	for (i = strlen(bf); i < n + 11; i++)
		bf[i] = '*';

	fseek(fp, offset, SEEK_SET);
	fwrite(bf, n + 12, 1, fp);
	fflush(fp);
}


/* Read a word in cbreak mode.  Returns 1 if we should exit.  Buffer must be
 * of size n+1 or more.
 */

int 
readword(char *bf)
{
	int i = 0;
	int ch, j, rc = 1;

	putchar('?');

	while ((ch = getchar()) != EOF) {
		if (isupper(ch))
			ch = tolower(ch);

		if (islower(ch)) {
			if (i < n) {
				bf[i++] = ch;
				putchar(ch);
			} else
				putchar('\a');
		} else if (ch == '\n') {
			if (i == n) {
				bf[i] = '\0';
				rc = 0;
				break;
			} else if (n > 1) {
				if (bf[0] == 'q') {
					putchar('\n');
					return 1;
				} else if (bf[0] == 'r') {
					putchar('\n');
					show_hist();
					putchar('?');
					i = 0;
				} else
					putchar('\a');
			} else
				putchar('\a');
		} else if (ch == EOF_CHAR)
			break;

		else if (ch == ERASE_CHAR) {
			if (i > 0) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				i--;
			} else
				putchar('\a');
		} else if (ch == KILL_CHAR || ch == WERASE_CHAR) {
			while (i > 0) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				i--;
			}
		} else if (ch == '\014' || ch == '\022') {	/* ctrl-L or ctrl-R */
			for (j = 0; j <= i; j++)
				putchar('\b');
			for (j = 0; j <= i; j++)
				putchar(' ');
			for (j = 0; j <= i; j++)
				putchar('\b');
			show_hist();
			putchar('?');
			for (j = 0; j < i; j++)
				putchar(bf[j]);
		}
	}

	/* Erase the question mark */
	for (j = 0; j <= i; j++)
		putchar('\b');
	putchar(' ');
	for (j = 0; j < i; j++)
		putchar(bf[j]);

	return rc;
}


/* Read the answer to a yes/no question.  Return true if yes. */

int 
yes(void)
{
	int ch, got = 0, trc, rc = 0;

	while ((ch = getchar()) != EOF) {
		trc = (ch == 'Y' || ch == 'y');
		if (trc || ch == 'n' || ch == 'N') {
			got = 1;
			rc = trc;
			putchar(ch);
		} else if (ch == '\n') {
			putchar('\n');
			if (got)
				return rc;
			else
				return 0;
		} else if (ch == ERASE_CHAR || ch == KILL_CHAR) {
			if (got) {
				putchar('\b');
				putchar(' ');
				putchar('\b');
				got = 0;
			}
		} else if (ch == EOF_CHAR) {
			putchar('\n');
			return 0;
		} else
			putchar('\a');
	}
	putchar('\n');
	return 0;
}

struct sc {
	char login[8];
	int ngames;
	int total;
};

int 
cmp_scores(const void *a, const void *b)
{
	return strncmp(((struct sc *) a)->login, ((struct sc *) b)->login, 8);
}

void 
show_score(void)
{
	int sz, m, scr, i;
	struct sc *scores;
	char bf[BFSZ + 1];

	sz = 50;
	scores = (struct sc *) malloc(sz * sizeof(struct sc));
	m = 0;

	/* Gather info about users into growing array */
	while (fgets(bf, BFSZ, fp) != NULL) {
		if (isdigit(bf[n]) && (scr = atoi(bf + n)) > 0) {
			for (i = 0; i < m; i++)
				if (!strncmp(bf + n + 3, scores[i].login, 8)) {
					scores[i].ngames++;
					scores[i].total += scr;
					goto next;
				}
			if (m >= sz) {
				sz = m + 50;
				scores = (struct sc *) realloc(scores, sz * sizeof(struct sc));
			}
			strncpy(scores[m].login, bf + n + 3, 8);
			scores[m].ngames = 1;
			scores[m].total = scr;
			m++;
		}
next:		;
	}

	/* Sort the array */
	qsort(scores, m, sizeof(struct sc), cmp_scores);

	/* Print it out */
	printf("user     games average\n");
	printf("----------------------\n");
	for (i = 0; i < m; i++) {
		int j;
		for (j = 0; j < 8; j++)
			putchar((scores[i].login[j] == '*') ? ' ' : scores[i].login[j]);
		printf("%5d %7.1f\n", scores[i].ngames,
		       (float) scores[i].total / scores[i].ngames);
	}
}

int 
main(int argc, char **argv)
{
	char *file = DEFAULT_FILE;
	char bf[BFSZ + 1];
	int i, score = 0;
	char *word;
	long wordno;
	int guesses;
	int nosave = 0;
	progname = argv[0];

	/*
	 * Parse arguments - mainly just figure out what data file we are
	 * using
	 */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			int j;
			if (argv[i][1] == 's' && argv[i][2] == '\0')
				score = 1;
			else {
				for (j = 0; optfiles[j].opt != NULL &&
				 strcmp(optfiles[j].opt, argv[i] + 1); j++);
				if (optfiles[j].opt != NULL)
					file = optfiles[j].path;
				else {
					printf("%s: Unknown option -%s\n", progname, argv[i] + 1);
					done(1);
				}
			}
		} else {
			file = argv[i];
		}
	}

	/* Open the data file */
	if ((fp = fopen(file, "r+")) == NULL) {
		if ((fp = fopen(file, "r")) == NULL) {
			printf("%s: Cannot open data file %s to read\n", progname, file);
			done(1);
		} else {
			nosave = 1;
			if (!score)
				printf("Cannot open data file %s to write.\n"
				       "Scores will not be saved.\n", file);
		}
	}
	/* Surrender suid privileges, just for the shear paranoia of it all */
	setuid(getuid());

	/* Read header line */
	if (fgets(bf, BFSZ, fp) == NULL) {
		printf("%s: Cannot read header from data file %s\n", progname, file);
		done(1);
	}
	line1 = ftell(fp);

	if (sscanf(bf, "%d %d %d %n", &n, &words, &targets, &i) < 3) {
		printf("%s: Bad header format in data file %s\n", progname, file);
		done(1);
	}
	desc = strdup(bf + i);

	if (score) {
		show_score();
		exit(0);
	}
	hash_file();

	/* Set up stuff for running in cbreak mode */
	initmodes();
	signal(SIGQUIT, (void (*) ()) intr);
	signal(SIGINT, (void (*) ()) intr);
#ifdef JOBCONTROL
	signal(SIGTSTP, (void (*) ()) susp);
#endif				/* JOBCONTROL */
	set_mode(1);

	printf("%s\n", desc);

	/* Seed random number generator */
	srandom(time(0L) + getpid());

	/* printf("n=%d t=%d w=%d o=%ld\n",n,targets,words,line1); */

	/* Memory for the target word */
	word = malloc(n + 1);

	for (;;) {
		choose_word(word, &wordno);
		free_hist();
		printf("Word chosen...\n");

		/* printf("%s\n",word); */
		guesses = 0;
		for (;;) {
			if (readword(bf)) {
				printf("The word was: %s\n", word);
				goto done;
			}
			guesses++;
			if (check_word(bf, word))
				break;
		}
		printf("Got it in %d guess%s.\n", guesses, (guesses != 1) ? "es" : "");

		if (!nosave)
			save_score(word, wordno, guesses);

		printf("Again? ");
		if (!yes())
			break;
	}
done:	;

	fclose(fp);

	done(0);
}
