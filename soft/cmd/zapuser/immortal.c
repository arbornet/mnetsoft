#include "zapuser.h"

char *immortal_file = NULL;	/* File containing immortals list */

char (*immort)[MAX_LOGIN + 1] = NULL;
int immortsz = 0;
int nimmort = 0;

/*
 * ADD_IMMORT add a name to the copy of the immortals list in memory.  This
 * insertion sorts it into the correct place.  Since the immortals file is
 * normally kept sorted, insertion sort is faster than qsort().
 */

void
add_immort(char *name)
{
	int i;

	if (nimmort >= immortsz) {
		immortsz += 300;
		immort = Realloc(immort, immortsz * (MAX_LOGIN + 1));
	}
	immort[nimmort][MAX_LOGIN] = '\0';

	for (i = nimmort - 1; i > 0; i--) {
		if (strncmp(immort[i], name, MAX_LOGIN) > 0)
			strncpy(immort[i + 1], immort[i], MAX_LOGIN);
		else
			break;
	}
	strncpy(immort[i + 1], name, MAX_LOGIN);
	nimmort++;
}

/*
 * LOAD_IMMORTALS Load the immortals file into memory.
 */

void
load_immortals()
{
	char bf[BFSZ];
	FILE *fp;
	char *p;

	if (immortal_file == NULL)
		return;

	if (verbose)
		printf("reading immortal file %s\n", immortal_file);

	if ((fp = fopen(immortal_file, "r")) == NULL) {
		error("could not open immortal file '%s'", immortal_file);
		return;
	}
	while (fgets(bf, BFSZ, fp) != NULL) {
		if ((p = strchr(bf, '\n')) != NULL)
			*p = '\0';
		if (p == NULL || p - bf > MAX_LOGIN)
			error("line too long in %s: %s", immortal_file, bf);
		else
			add_immort(bf);
	}
	fclose(fp);
}


/*
 * IS_IMMORTAL Check if the named user is immortal.
 */

int
is_immortal(char *name)
{
	int c, i;
	int lo = 0;
	int hi = nimmort - 1;

	if (immort == NULL)
		return 0;

	while (lo <= hi) {
		i = (lo + hi) / 2;
		if ((c = strcmp(name, immort[i])) == 0)
			return 1;
		else if (c < 0)
			hi = i - 1;
		else
			lo = i + 1;
	}
	return 0;
}

/*
 * FREE_IMMORTALS Free memory in which immortals are stored.
 */

void
free_immortals()
{
	if (immort == NULL)
		return;
	free(immort);
	immortsz = nimmort = 0;
	immort = NULL;
}
