/*
 * Robocop challenge/response.
 */

#include <sys/types.h>

#include <openssl/sha.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
	MAXSTRLEN = 2 * 32 +1 //SHA256_BLOCK_LENGTH + 1
};

extern void getresponse(char k[MAXSTRLEN], char c[MAXSTRLEN], char r[MAXSTRLEN]);

FILE *logfp;

static void
getstring(int quiet, const char *prompt, char str[MAXSTRLEN])
{
	char *p, buf[256 + 1];

	if (!quiet) {
		printf("%s: ", prompt);
		fflush(stdout);
	}
	memset(str, '\0', MAXSTRLEN);
	fgets(buf, sizeof(buf), stdin);
	buf[256] = '\0';
	p = strchr(buf, '\n');
	if (p != NULL)
		*p = '\0';
	strlcpy(str, buf, MAXSTRLEN);
}

int
main(int argc, char *argv[])
{
	int ch, quiet;
	char robokey[MAXSTRLEN];
	char challenge[MAXSTRLEN];
	char response[MAXSTRLEN];

	logfp = stderr;
	quiet = 0;
	if ((ch = getopt(argc, argv, "q")) != -1)
		switch (ch) {
		case 'q':
			quiet = 1;
			break;
		}

	getstring(quiet, "Enter robokey", robokey);
	getstring(quiet, "Enter challenge", challenge);
	getresponse(robokey, challenge, response);

	if (!quiet)
		printf("Your response: ");
	printf("%s\n", response);

	return(EXIT_SUCCESS);
}
