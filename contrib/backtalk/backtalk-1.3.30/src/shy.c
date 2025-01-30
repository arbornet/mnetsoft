/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* SHY.C -- This program is used on Grex to allow users to put themselves on
 * the list of people whose responses will not be displayed to unregistered
 * backtalk readers.  This file is just a list of login names, in no
 * particular order, one per line.
 *
 * This program is part of a fix for a political problem on Grex.  Better
 * versions will be developed in the future.
 *
 * This program should be installed suid to whomever owns the shylist file.
 *
 *   Jan Wolter - 04/27/97
 */

#include "common.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define SHYFILE "/home/janc/src/backtalk/shylist"

struct pern {
	char name[9];
	struct pern * next;
	} *head= NULL;
FILE *fp;


/* LOADFILE - read the list of users into memory.   Return true if the named
 * user is in the list.
 */

int loadfile(char *user)
{
    char bf[1024];
    struct pern *new;
    struct pern *last;
    int infile= 0;
    int i;

    if ((fp= fopen(SHYFILE,"r+")) == NULL &&
	(fp= fopen(SHYFILE,"w+")) == NULL)
    {
	fprintf(stderr,"Cannot open "SHYFILE"\n");
	exit(1);
    }

    flock(fileno(fp), LOCK_EX);

    while (fgets(bf,1024,fp) != NULL)
    {
	if (bf[0] == '#') continue;

	new= (struct pern *)malloc(sizeof(struct pern));

	for (i= 0; i < 8 && bf[i] != '\n' && bf[i] != '\0'; i++)
	    new->name[i]= bf[i];
	new->name[i]= '\0';
	new->next= NULL;

	if (!strcmp(new->name, user)) infile= 1;

	if (head == NULL)
	    head= new;
	else
	    last->next= new;
	last=new;
    }

    return infile;
}


/* SAVEFILE - write the file back from memory, deleting the user "without".
 * If "without" is NULL, write whole file.
 */

void savefile(char *without)
{
    struct pern *curr;

    rewind(fp);
    ftruncate(fileno(fp),0L);

    for (curr= head; curr != NULL; curr= curr->next)
	if (without == NULL || strcmp(without, curr->name))
	    fprintf(fp,"%s\n",curr->name);
}


void del_user(char *name)
{
    if (!loadfile(name))
    {
	printf("%s is not on the shy list.\n",name);
	exit(1);
    }
    savefile(name);
    fclose(fp);
}


add_user(char *name)
{
    if (loadfile(name))
    {
	printf("%s is already on the shy list.\n",name);
	exit(1);
    }
    savefile(NULL);
    fprintf(fp,"%s\n",name);
    fclose(fp);
}


void usage(char *progname)
{
    fprintf(stderr,"Usage:\n"
		   "  %s y\n"
		   "     Add yourself to the list of people whose conference responses\n"
		   "     will not be shown to unregistered users by backtalk.\n", progname);
    fprintf(stderr,"  %s n\n"
		   "     Remove yourself to the list of people whose conference responses\n"
		   "     will not be shown to unregistered users by backtalk.\n", progname);
    exit(1);
}


main (int argc, char **argv)
{
    struct passwd *pw;

    if (argc != 2 || argv[1][1] != '\0')
	usage(argv[0]);

    pw= getpwuid(getuid());

    switch (argv[1][0])
    {
    case 'y':
    case 'Y':
    case '1':
	add_user(pw->pw_name);
	break;
    case 'n':
    case 'N':
    case '0':
	del_user(pw->pw_name);
	break;
    default:
	usage(argv[0]);
	break;
    }
    exit(0);
}
