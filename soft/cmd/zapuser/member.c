#include <grp.h>
#include <stdio.h>
#include <stdlib.h>

#include "zapuser.h"

/* List of protected groups */

char **grouplist = NULL;
int *gidlist = NULL;
int groupn = 0;
int groupsz = 0;


/* List of members of protected groups */

char **memlist = NULL;
int memn = 0;
int memsz = 0;


/*
 * PROTECT_GROUP Add the given group name to the list of protected groups.
 */

void
protect_group(char *gname)
{
	if (groupn >= groupsz) {
		groupsz = groupn + 10;
		grouplist = (char **) Realloc(grouplist, groupsz * sizeof(char *));
		if (grouplist == NULL) {
			fprintf(stderr, "out of memory for group list\n");
			exit(1);
		}
	}
	grouplist[groupn++] = strdup(gname);
	if (verbose > 1)
		printf("protect group %s\n", gname);
}


/*
 * SAVE_USERS Copy all the users in the given list to the list of members of
 * protected groups.
 */

void
save_users(char **list)
{
	char **u;

	for (u = list; *u != NULL; u++) {
		if (memn >= memsz) {
			memsz = memn + 200;
			memlist = (char **) Realloc(memlist, memsz * sizeof(char *));
			if (memlist == NULL) {
				fprintf(stderr, "out of memory for member list\n");
				exit(1);
			}
		}
		memlist[memn++] = strdup(*u);
	}
}


/*
 * FIND_MEMBERS Save information about who is protected groups for later use
 * by is_member().
 */

void
find_members()
{
	struct group *gr;
	int i;

	if (groupn == 0)
		return;

	gidlist = (int *) malloc(groupn * sizeof(int));
	if (gidlist == NULL) {
		fprintf(stderr, "out of memory for gid list\n");
		exit(1);
	}
	for (i = 0; i < groupn; i++) {
		if ((gr = getgrnam(grouplist[i])) == NULL) {
			error("group %s protected in " CONFIG_FILE " not found",
			      grouplist[i]);
		} else {
			gidlist[i] = gr->gr_gid;
			save_users(gr->gr_mem);
		}
	}

	/* Don't need group names anymore */
	free(grouplist);
}


/*
 * IS_MEMBER Given a user name and gid number, is that person a member or a
 * staffer? find_members() must have been called before this.
 */

int
is_member(char *user, gid_t gid)
{
	int i;

	/* Check if principle gid is a protected group */
	for (i = 0; i < groupn; i++)
		if (gid == gidlist[i])
			return 1;

	/* Check if assigned to a protected group via /etc/group file */
	for (i = 0; i < memn; i++) {
		if (!strcmp(user, memlist[i]))
			return 1;
	}

	return 0;
}
