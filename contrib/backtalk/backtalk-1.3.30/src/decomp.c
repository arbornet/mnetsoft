/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "printtoken.h"
#include "comp.h"
#include "str.h"
#include "byteorder.h"
#include "readcomp.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *progname, *source;
int ttymode= 1;

extern unsigned long filelist_size;

void print_prog(Token *p);


main(int argc, char **argv)
{
    int i,j, flags= 1, vflag= 0;
    int fd;
    Token *head;
    char bf[4];

    progname= argv[0];

    check_endian();

    for (i= 1; i < argc; i++)
    {
	if (flags && argv[i][0] == '-')
	{
	    switch (argv[i][1])
	    {
	    case 'v':
		vflag= 1;
		break;
	    default:
		goto usage;
	    }
	}
	else
	{
	    flags= 0;

	    if ((fd= open(source= argv[i],O_RDONLY)) < 0)
		printf("%s: cannot open %s\n", progname, argv[i]);
	    else
	    {
		/* Check magic */
		read(fd,bf,4);
		if (strncmp(bf,MAGIC,4))
			die("bad magic number in executable file %s",
				source);

		/* Check version */
		read(fd,bf,4);
		printf("Compiled for Backtalk version %d.%d.%d.%d\n",
			bf[0], bf[1], bf[2], bf[3]);
		if (bf[0] != VERSION_A || bf[1] != VERSION_B ||
		    bf[2] != VERSION_C || bf[3] != VERSION_D)
			printf("WARNING:  This decompiler is version"
			       "%d.%d.%d.%d.\n          Output is "
			       "probably garbage!\n\n",VERSION_A,
			       VERSION_B,VERSION_C,VERSION_D);

		/* Read and print the list of source file names */
		read_file_list(fd);
		printf("Source File List:\n");
		for (j= 0; j < filelist_size; j++)
			printf("%2d) %s\n",j,getlistfile(j));
		printf("=============================\n");

		/* Load program */
		head= (Token *)malloc(sizeof(Token));
		read_token(fd,head);

		/* Write program */
		if (vflag)
		    print_prog(head);
		else
		{
		    print_token(stdout,head);
		    putchar('\n');
		}
	    }
	}
    }
    exit(0);

usage:
    fprintf(stderr,"usage: %s [-v] file.bo [...]\n", argv[0]);
    exit(1);
}


#if __STDC__
void die(char const *fmt,...)
#else
void die(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
va_list ap;

    VA_START(ap,fmt);
    fprintf(stderr,"%s: Error in %s: ",progname,source);
    vfprintf(stderr, fmt, ap);
    fputs("\n",stderr);
    va_end(ap);
    exit(1);
}


void die_noscript(char *file)
{
    fprintf(stderr,"%s: Error in %s: Script %s not found",progname,source,file);
    exit(1);
}


/* PRINT_PROG - Print out the the given TK_PROCEDURE token.  Debugging use
 * only.
 */

int ppl_sfid;

void ppl(int level, Token *t)
{
    Token *p;
    int i,j;
    char *c;

    for (i= 0; i < aval(*t)->sz; i++)
    {
	p= &(aval(*t)->a[i]);
	if (p->sfid != ppl_sfid)
	    printf("%s:\n",getlistfile(ppl_sfid= p->sfid));

	printf("%3d: ",p->line);

	for (j= 0; j < level; j++)
	    fputs("  ",stdout);

	switch (type(*p))
	{
	case TK_PROCEDURE:
	    fputs("{\n",stdout);
	    ppl(level+1,p);
	    for (j= 0; j < level; j++)
		fputs("  ",stdout);
	    fputs("     }\n",stdout);
	    break;
	case TK_INTEGER:
	    printf("%ld\n",ival(*p));
	    break;
	case TK_BOUND_SYMBOL:
	    printf("%-32s  <bound>\n",sysdict[ival(*p)].key);
	    break;
	case TK_BOUND_LITERAL:
	    printf("/%-32s <bound>\n",sysdict[ival(*p)].key);
	    break;
	case TK_MARK:
	    printf("mark <%ld>\n",ival(*p));
	    break;
	case TK_STRING:
	    putchar('(');
	    for (c= sval(*p); *c != '\0'; c++)
	    {
		if (*c == '\n')
		    fputs("\\n",stdout);
		else
		    putchar(*c);
	    }
	    fputs(")\n",stdout);
	    break;
	case TK_UNBOUND_SYMBOL:
	    printf("%-32s  <unbound>\n",sval(*p));
	    break;
	case TK_UNBOUND_LITERAL:
	    printf("/%-32s <unbound>\n",sval(*p));
	    break;
	case TK_ARRAY:
	    printf("[...]\n");
	    break;
	case TK_FUNCTION:
	    printf("<func>\n");
	    break;
	case TK_DYNAMIC:
	    printf("<dyna:%s>\n",sval(*p));
	    break;
	}
    }
}

void print_prog(Token *p)
{
    ppl_sfid= -1;
    ppl(0,p);
}
