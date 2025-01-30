/*
 * READITEM -- Display item number item_no of the conference whose directory
 * is dir, starting with response first_resp, through response last_resp.  If
 * last_resp is -1, then display through the last response in the item.  The
 * number of the last response read is returned, or -1 if nothing is read. If
 * show_header is 0, no header is printed.  If it is 2, only the header is
 * printed.
 *
 * Mar 15, 1993 - Jan Wolter:  Original version Jun  4, 1993 - Jan Wolter:  Skip
 * scribbled text Dec  2, 1995 - Jan Wolter:  Ansification
 */

#include <sys/types.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bbsread.h"

void copyto(char *, char *, char, int);
void item_head(FILE *, int, char *, time_t, char *, char *, bool);
void resp_head(FILE *, int, time_t, char *, char *, bool);
void new_responses(FILE *, int, int, bool);
void weird_item(void);


int
dispitem(FILE * ofp,		/* output device */
	 char *dir,		/* name of conference directory */
	 int item_no,		/* item number to print */
	 int first_resp,	/* First response to display */
	 int last_resp,		/* Last response to display */
	 int max_resp,		/* Largest number response (unreliable) */
	 int show_header,	/* Should we print the header ? */
	 int prefer_html)
{				/* Should we prefer HTML version ? */
	FILE *ifp;
	char buf[BFSZ];
	char title[BFSZ];
	char fullname[BFSZ];
	char login[9];
	int uid;
	long date;
	char *a;
	int resp;
	bool printing = false, scribbled = false;
	bool thisversion = false, gotversion = false;
	bool didE = true;

	if (last_resp == -1)
		last_resp = INT_MAX;

	snprintf(buf, sizeof buf, "%s/_%d", dir, item_no);

	if ((ifp = fopen(buf, "r")) == NULL) {
		fprintf(stderr, "%s: could not open %s\n", progname, buf);
		exit(1);
	}
	/* Read and check magic code number */
	if (fgets(buf, BFSZ, ifp) == NULL || strcmp(buf, "!<ps02>\n"))
		weird_item();

	title[0] = '\0';
	fullname[0] = '\0';
	login[0] = '\0';
	uid = -1;
	date = 0L;

	resp = 0;
	printing = true;

	while (fgets(buf, BFSZ, ifp) != NULL)
		if (buf[0] == ',')
			switch (buf[1]) {
			case 'H':	/* Item Title */
				if (!printing)
					break;
				copyto(title, buf + 2, '\n', BFSZ);
				break;
			case 'A':	/* Item or Response Alias */
				if (!printing)
					break;
				copyto(fullname, buf + 2, '\n', BFSZ);
				break;
			case 'U':	/* Item or Response Login/Uid */
				if (!printing)
					break;
				uid = atoi(buf + 2);
				if ((a = index(buf + 2, ',')) == NULL)
					weird_item();
				copyto(login, a + 1, '\n', 9);
				break;
			case 'D':	/* Item or Response Date */
				if (!printing)
					break;
				date = atolh(buf + 2);

				/* Print response/item headers */
				if (resp == 0) {
					if (show_header)
						item_head(ofp, item_no, title,
						      date, fullname, login,
							  scribbled);
					if (show_header == 2) {
						fclose(ifp);
						return (-1);
					}
					if (first_resp > 0)
						printing = false;
				} else if (printing)
					resp_head(ofp, resp, date, fullname, login,
						  scribbled);
				thisversion = prefer_html;
				gotversion = false;
				break;
			case 'T':
				thisversion = !gotversion;
				didE = false;
				break;
			case 'R':
				/* Handle flags later maybe */
				/* R0001 censored */
				/* R0003 scribbled */
				scribbled = (buf[5] == '3');
				if (didE)
					break;
			case 'E':
				didE = true;
				if (resp == 0 && last_resp == INT_MAX)
					new_responses(ofp, first_resp, max_resp,
						   printing || show_header);
				if (uid != -1) {
					fullname[0] = '\0';
					login[0] = '\0';
					uid = -1;
					date = -1L;
				}
				resp++;
				printing = (resp >= first_resp &&
					    resp <= last_resp);
				break;
			case ':':
				if (printing && !scribbled && thisversion) {
					fputc(' ', ofp);
					fputs(buf + 2, ofp);
					gotversion = true;
				}
				break;
			case ',':
				if (printing && !scribbled && thisversion) {
					fputc(' ', ofp);
					fputs(buf + 1, ofp);
					gotversion = true;
				}
				break;
			default:
				weird_item();
			}
		else {
			if (printing && !scribbled && thisversion) {
				fputc(' ', ofp);
				fputs(buf, ofp);
				gotversion = true;
			}
		}
	fclose(ifp);
	return (resp - 1);
}

void
weird_item()
{
	fprintf(stderr, "%s: item file in weird format\n", progname);
	exit(1);
}

/*
 * COPYTO -- Copy b to a up to but not including the first occurance of ch in
 * b.  Don't copy more than n-1 characters.
 */

void
copyto(char *a, char *b, char ch, int n)
{
	while ((*b != ch) && (n-- > 0))
		*a++ = *b++;
	*a = '\0';
}


void
item_head(FILE * ofp,
	  int item_no,
	  char *title,
	  time_t date,
	  char *fullname,
	  char *login,
	  bool scribbled)
{

	fprintf(ofp, "Item #%d entered by %s(%s) on %s",
		item_no, fullname, login, ctime(&date));
	if (scribbled)
		fprintf(ofp, " %s\n <scribbled>\n", title);
	else
		fprintf(ofp, " %s\n\n", title);
}


void
resp_head(FILE * ofp,
	  int resp_no,
	  time_t date,
	  char *fullname,
	  char *login,
	  bool scribbled)
{
	fprintf(ofp, "#%d %s(%s) on %24.24s:\n",
		resp_no, fullname, login, ctime(&date));
	if (scribbled)
		fprintf(ofp, " <scribbled>\n");
}

void
new_responses(FILE * ofp, int first, int max, bool printing)
{

	if (max == 0)
		fprintf(ofp, "No responses.\n");
	else {
		if (first == 0)
			first = 1;
		if (printing)
			fprintf(ofp, "\n%d response%s total.\n\n", max,
				max == 1 ? "" : "s");
		else
			fprintf(ofp, "%d new of %d response%s total.\n\n",
				max - (first == 0 ? 0 : first - 1), max,
				max == 1 ? "" : "s");
	}
}
