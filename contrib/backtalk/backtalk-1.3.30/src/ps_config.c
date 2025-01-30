/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Read conference Configuration files.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version
 * May 29, 1993 - Jan Wolter:  Revised to read conference modes.
 * Dec  2, 1995 - Jan Wolter:  Ansification
 * Jan  2, 1996 - Jan Wolter:  BackTalk version
 */

#include "backtalk.h"
#include <ctype.h>

#include "ps_config.h"
#include "ps_conflist.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define CONFIG		"/config"

/* Text modes - These are a Yapp-ism.  We support them for Yapp compatibility.
 * Mode can have on primary keyword and any number of non-primary keywords.
 * Numeric mode is just the sum of the text modes in this table.
 */

struct textmodes {
    char *name;		    /* keyword */
    int prime;		    /* is it a primary keyword? */
    int value;		    /* value */
} textmode[]= {
    {"public",     1,   0},    /* Public conference */
    {"ulist",      1,   4},    /* Ulisted conference */
    {"password",   1,   5},    /* Password protected conference */
    {"paranoid",   1,   6},    /* Password and ulist conference */
    {"protected",  1,   8},    /* Public, but item files depermitted */
    {"readonly",   0,  16},    /* Fishbowl */
    {"fishbowl",   0,  16},    /* Fishbowl */
    {"maillist",   0,  64},    /* mailing list - not supported */
    {"registered", 0, 128},    /* registered mailing list - not supported */
    {"noenter",    0, 256},    /* only fw may enter new items */
    {"acl",        1, -999999},/* Force getting mode from acl file */
    {NULL,         0,   0} };


/* PARSE_TEXTMODE -- Parse a line of text modes.  The passed in pointer should
 * point to the first non-blank character of the line.  The line will be
 * mutilated during processing.  Returns an integer mode value.
 */

int parse_textmode(char *b)
{
    int i, mode, primemode= -1;
    char *e;

    mode= 0;
    while (*b != '\0')
    {
	*(e= firstin(b," \t\n\r"))= '\0';
	for (i= 0; textmode[i].name != NULL; i++)
	    if (!strcmp(b,textmode[i].name))
	    {
		if (textmode[i].prime)
		{
		    if (primemode == -1)
			primemode= textmode[i].value;
		    else if ((primemode == 4 && textmode[i].value == 5) ||
			     (primemode == 5 && textmode[i].value == 4))
			/* We tolerate 'ulist password' for 'paranoid' */
			primemode= 6;
		    else
			die("Illegal conference mode - "
			    "multiple primary keywords\n");
		    mode= primemode;
		}
		else
		    mode+= textmode[i].value;
		break;
	    }

	if (textmode[i].name == NULL)
	    die("Illegal conference mode - bad keyword %s\n",b);

	b= firstout(e+1," \t\n\r");
    }
    return mode;
}


/* READCONFIG -- Given the directory path for a conference, return the name
 * of the participation file, the list of fairwitnesses, the  mode of the
 * conference and the conference title.  Modes 4, 5 and 6 indicate closed
 * conferences of various kinds.  Pfname and fw point to storage areas of at
 * least BFSZ bytes.  Title set to null if no conference title is set or is
 * set to point to a conference title in static storage.  If pfname, fw, mode,
 * or title are passed in as NULL, they aren't set.
 *
 * Missing fairwitness, mode and title lines can be tolerated.
 */


void readconfig(char *dir, char *pfname, char *fw, int *mode, char **title)
{
    FILE *fp;
    static char buf[BFSZ];
    char *b;

    /* Set some defaults */
    if (fw != NULL) fw[0]= '\0';
    if (mode != NULL) *mode= 0;
    if (title != NULL) *title= NULL;

    strcpy(buf,dir);
    strcat(buf,CONFIG);

    if ((fp= fopen(buf,"r")) == NULL)
	die("could not open %s",buf);

    /* Read and check magic code number */
    if (fgets(buf,BFSZ,fp) == NULL || strcmp(buf,"!<pc02>\n"))
	die("Bad config file version number: %s", buf);

    /* Read participation file name */
    if (fgets(buf,BFSZ,fp) == NULL)
	die("conference configuration file is missing name of "
	    "participation file");

    if ((b= strchr(buf,'\n')) == NULL)
	die("conference configuration file has overly long "
	    "participation file name");
    *b = '\0';
    if (pfname != NULL) strcpy(pfname,buf);

    /* Read the time limit line - this is always zero, so who cares? */
    fgets(buf,BFSZ,fp);

    /* Read the list of fairwitnesses */
    if (fgets(buf,BFSZ,fp) != NULL && fw != NULL)
    {
	if ((b= strchr(buf,'\n')) == NULL)
	    die("conference configuration file has overly long "
		"list of fairwitnesses\n");
	*b = '\0';
	strcpy(fw,buf);
    }

    /* Read the mode line, if it exists */
    if (fgets(buf,BFSZ,fp) != NULL && mode != NULL)
    {
	b= firstout(buf," \t\n\r");
	if (isdigit(*b) || *b == '-')
	    *mode= atoi(b);
	else
	    *mode= parse_textmode(b);
    }

#ifdef YAPP_COMPAT
    /* Read the email addresses for Yapp maillink conferences - always
     * ignored by Backtalk */

    fgets(buf,BFSZ,fp);
#endif /* YAPP_COMPAT */

    /* Read the conference title */
    if (title != NULL)
    {
	if (fgets(buf,BFSZ,fp) != NULL)
	{
	    if ((b= strchr(buf,'\n')) == NULL)
		die("conference configuration file has overly long "
		    "conference title line\n");
	    *b = '\0';
	    *title= firstout(buf," \t");
	    if (**title == '\0') *title= NULL;
	}
    }

    fclose(fp);
    return;
}
