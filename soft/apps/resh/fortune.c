/*
 *  Read a fortune.
 */

#include <sys/types.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

const char *idx = "/cyberspace/grex/resh/lib/fortunes.idx";
const char *fortunes = "/cyberspace/grex/resh/lib/fortunes";

int
main(int argc, char *argv[])
{
	FILE	*fp;
	long	pos;
	int	ch, ch0, ch1, n;

	fp = fopen(idx, "r");
	if (fp == NULL) {
		perror(argv[1]);
		return(EXIT_FAILURE);
	}
	fseek(fp, 0, SEEK_END);
	n = arc4random() % (ftell(fp) / sizeof(long));
	fseek(fp, sizeof(long) * n, SEEK_SET);
	fread(&pos, 1, sizeof pos, fp);
	fclose(fp);
	fp = fopen(fortunes, "r");
	if (fp == NULL) {
		perror(argv[2]);
		return(EXIT_FAILURE);
	}
	fseek(fp, pos, SEEK_SET);
	ch1 = 0;
	ch0 = fgetc(fp);
	ch = fgetc(fp);
	while (ch != EOF) {
		if ((ch1 == 0 || ch1 == '\n') && ch0 == '%' && ch == '\n')
			break;
		fputc(ch0, stdout);
		ch1 = ch0;
		ch0 = ch;
		ch = fgetc(fp);
	}
	fclose(fp);

	return(EXIT_SUCCESS);
}
