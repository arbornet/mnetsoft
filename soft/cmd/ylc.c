/*
 * lc [directory ...]
 *
 * Copyright University of Waterloo, 1978,1985,1987
 */

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum
{
	DEFNCOLS = 5,
	COLUMNWIDTH = 15,
	INDENT = 4,
};

typedef struct filelist {
	char **names;	// Array of names
	size_t end;	// Next element to use
	size_t size;	// Array size
} Filelist;

Filelist files;
Filelist directories;
Filelist blockdevs;
Filelist chardevs;
Filelist symlinks;
Filelist sockets;
Filelist pipes;
Filelist others;

int fflag;		// Show regular files
int dflag;		// Show directories
int bflag;		// Show block devices
int cflag;		// Show character devices
int lflag;		// Show symbolic links
int sflag;		// Show Unix domain sockets
int pflag;		// Show named pipes
int aflag;		// Show '.' and '..' in output

int noflag;		// No output - just want exit status
int ncolumns;		// Number of output file-name columns
char *dirprefix;
int manyflag;
int anyfound;

char pathbuf[MAXPATHLEN];
char *filename = pathbuf;
size_t maxfnlen = sizeof(pathbuf);

void
verrorf(const char *fmt, va_list args)
{
	vfprintf(stderr, fmt, args);
	fprintf(stderr, ": %s\n", strerror(errno));
}

void
warn(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verrorf(fmt, args);
	va_end(args);
}

void
error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	verrorf(fmt, args);
	va_end(args);
	exit(EXIT_FAILURE);
}

void *
erealloc(void *m, const size_t len)
{
	void *p;

	p = realloc(m, len);
	if (p == NULL) {
		error("realloc failed (%zu)", len);
	}

	return p;
}

char *
estrdup(const char *s)
{
	char *r;

	r = strdup(s);
	if (r == NULL) {
		error("stdup failed ('%s')", s);
	}

	return r;
}

void
extend(Filelist *list)
{
	void *m, *n;
	size_t resize;

	resize = (list->size == 0) ? 8 : 2*list->size;
	m = erealloc(list->names, resize*sizeof(char *));
	n = (char *)m + list->size*sizeof(char *);
	memset(n, 0, (resize - list->size)*sizeof(char *));
	list->names = (char **)m;
	list->size = resize;
}

void
addto(Filelist *list, const char *name)
{
	anyfound++;
	if (noflag) { return; }
	if (list->end >= list->size) { extend(list); }
	list->names[list->end++] = estrdup(name);
}

void
clear(Filelist *list)
{
	for (int k = 0; k < list->end; k++)
		free(list->names[k]);
	list->end = 0;
}

void
pindent()
{
	printf("%-*s", INDENT, "");
}

int
pstrcmp(const void *pa, const void *pb)
{
	const char *a = *(const char * const *)pa;
	const char *b = *(const char * const *)pb;

	return strcmp(a, b);
}

int
getcolumns(int offset)
{
	int cols, width;
	struct winsize win;

	if (!isatty(STDOUT_FILENO)) {
		return DEFNCOLS;
	}
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) < 0) {
		return DEFNCOLS;
	}
	cols = 0;
	width = win.ws_col - offset;
	if (width > 0) {
		cols = width / COLUMNWIDTH;
	}
	if (cols <= 0) {
		return 1;
	}

	return cols;
}

void
print(char *title, Filelist *list)
{
	int position, maxposition, start;

	if (list->names == NULL || list->end == 0)
		return;

	printf("%s", dirprefix);
	dirprefix = "\n";

	qsort((void *)list->names, list->end, sizeof(char *), pstrcmp);
	start = 0;
	if (ncolumns > 1) {
		if (manyflag) {
			pindent();
		}
		printf("%s:\n", title);
		pindent();
		start = INDENT;
		if (manyflag) {
			pindent();
			start += INDENT;
		}
	}
	position = start;
	maxposition = ncolumns*COLUMNWIDTH + start;
	for (int k = 0; k < list->end; k++) {
		char *name = list->names[k];
		size_t namelen = strlen(name);
		int used = (int)namelen;	// For alignment

		if (position > start) {
			used += COLUMNWIDTH - (position - start)%COLUMNWIDTH;
		}
		if ((position + used) > maxposition) {
			printf("\n");
			if (ncolumns > 1) {
				pindent();
				if (manyflag) {
					pindent();
				}
				position = start;
			}
			used = namelen;
		}
		printf("%*s", used, name);
		position += used;
	}
	if (position != 0) {
		printf("\n");
	}
}

void
addtodir(mode_t mode, const char *name)
{
	if (fflag && S_ISREG(mode))       addto(&files, name);
	else if (dflag && S_ISDIR(mode))  addto(&directories, name);
	else if (bflag && S_ISBLK(mode))  addto(&blockdevs, name);
	else if (cflag && S_ISCHR(mode))  addto(&chardevs, name);
	else if (lflag && S_ISLNK(mode))  addto(&symlinks, name);
	else if (sflag && S_ISSOCK(mode)) addto(&sockets, name);
	else if (pflag && S_ISFIFO(mode)) addto(&pipes, name);
	else                              addto(&others, name);
}

void
preambledir(const char *path)
{
	dirprefix = "";
	if (manyflag) {
		static char *prefix = "";
		printf("%s%s:\n", prefix, path);
		prefix = "\n";
	}
}

void
printdir()
{
	if (dflag) print("Directories", &directories);
	if (fflag) print("Files", &files);
	if (pflag) print("Pipes", &pipes);
	if (bflag) print("Block Special Files", &blockdevs);
	if (cflag) print("Char Special Files", &chardevs);
	if (lflag) print("Symbolic Links", &symlinks);
	if (sflag) print("Sockets", &sockets);
	print("Others", &others);
}

void
cleardir()
{
	clear(&directories);
	clear(&files);
	clear(&pipes);
	clear(&blockdevs);
	clear(&chardevs);
	clear(&symlinks);
	clear(&sockets);
	clear(&others);
}

int
checkdir(const char *path)
{
	struct stat s;

	if (stat(path, &s) < 0) {
		warn("Failed to stat dir %s", path);
		return 0;
	}
	if (!S_ISDIR(s.st_mode)) {
		if (!manyflag) {
			errno = ENOTDIR;
			error("Bad starting path %s", path);
		}
		return 0;
	}

	return 1;
}

void
filldir(const char *path)
{
	DIR *dir = opendir(path);
	struct dirent *d;

	if (dir == NULL) {
		warn("Failed to open directory %s", path);
		return;
	}
	while ((d = readdir(dir)) != NULL) {
		const char *name = d->d_name;
		struct stat s;

		if (!aflag && (!strcmp(name, ".") || !strcmp(name, ".."))) {
			continue;
		}
		if (strlcpy(filename, name, maxfnlen) >= maxfnlen) {
			errno = ENAMETOOLONG;
			warn("Truncated path '%s' ('%s')", name, pathbuf);
			continue;
		}
		if (lstat(pathbuf, &s) < 0) {
			warn("Failed to lstat %s", pathbuf);
			continue;
		}
		addtodir(s.st_mode, name);
	}
	closedir(dir);
}

void
listdir(char *path)
{
	if (!checkdir(path)) {
		return;
	}
	preambledir(path);
	filldir(path);
	printdir();
	cleardir();
}

int
main(int argc, char *argv[])
{
	char *path;
	int all = 1, ch;

	while ((ch = getopt(argc, argv, "an1fdblcsSp")) != -1) {
		switch (ch) {
		case 'a':
			aflag++;
			break;
		case 'n':
			noflag++;
			break;
		case '1':
			ncolumns = 1;
			break;
		case 'f':
			fflag++;
			all = 0;
			break;
		case 'd':
			dflag++;
			all = 0;
			break;
		case 'b':
			bflag++;
			all = 0;
			break;
		case 'c':
			cflag++;
			all = 0;
			break;
		case 's':
			bflag++;
			cflag++;
			all = 0;
			break;
		case 'l':
			lflag++;
			all = 0;
			break;
		case 'S':
			sflag++;
			all = 0;
			break;
		case 'p':
			pflag++;
			all = 0;
			break;
		default:
			fprintf(stderr, "Unknown flag: %c\n", ch);
			continue;
		}
	}
	argc -= optind;
	argv += optind;
	if (all) {
		fflag = dflag = bflag = cflag = lflag = sflag = pflag = 1;
	}
	manyflag = (argc > 1);
	if (ncolumns == 0) {
		ncolumns = getcolumns(manyflag ? 2*INDENT : INDENT);
	}

	if (argc == 0) {
		listdir(".");
	} else while ((path = *(argv++)) != NULL) {
		size_t plen = 0;
		if ((strlen(path) + 2) > MAXPATHLEN) {
			errno = ENAMETOOLONG;
			warn("Pathname overflow for %s", path);
			continue;
		}
		strlcpy(pathbuf, path, sizeof(pathbuf));
		strlcat(pathbuf, "/", sizeof(pathbuf));
		plen = strlen(pathbuf);
		filename = pathbuf + plen;
		maxfnlen = sizeof(pathbuf) - plen;
		listdir(path);
	}

	return anyfound == 0;
}
