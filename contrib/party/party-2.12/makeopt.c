/* MAKEOPT:  Program to generate opt.h file from the opttab.c file.  Doing
 * it this way makes it easier to add options and commands.  Jan Wolter.
 */

#include <stdio.h>
#include "party.h"


void define(char *prefix, char *string, int val)
{
    char *p;

    printf("#define %s_",prefix);
    for (p=string;*p;p++)
	if (islower(*p))
	    putchar(toupper(*p));
	else
	    putchar(*p);
    if (strlen(string) < 4) putchar('\t');
    printf("\t%d\n",val);
}

main()
{
    int i;
    int chn_done = 0;

    printf("/* Don't bother editing this file.\n");
    printf(" * It is generated from opttab.c by the makeopt program.\n");
    printf(" */\n\n");

    /* Print the option defines */

    printf("#define OPT_NONE\t-1\n");
    for (i=0;opt[i].name;i++)
    {
	if (!chn_done && !(opt[i].pf & PF_CHN))
	{
	    printf("#define NCOPT\t\t%d\n",i);
	    chn_done=1;
	}

	define("OPT",opt[i].name,i);
    }
    printf("#define NOPT\t\t%d\n",i);

    /* Print the command defines */

    for (i=0;cmd[i].name;i++)
	define("CMD",cmd[i].name,i);

    printf("#define NCMD\t\t%d\n",i);

    exit(0);
}
