/* GATE -- GAther TExt for Yapp or Picospan.
 *
 * (c) June 1995, Jan Wolter
 *
 * This is a word wrapping text gatherer designed for use with Marcus Watt's
 * PicoSpan conferencing program, or Dave Thaler's Yapp clone of the same.
 *
 * This program may be freely used and distributed.
 *
 * janc@arbornet.org, janc@cyberspace.org, wolter@cs.tamu.edu
 */

#include "gate.h"

char *version_number="2.06";

FILE *tfp;              /* The file we are writing */
char *filename= NULL;   /* The file we are writing */
jmp_buf jenv;           /* Jump buffer for intrupt and suspend handling */
int use_jump= 0;        /* Should we use jenv or just resume? */
int subtask= 0;		/* Am I doing an interuptable subtask? */
int in_cbreak= 0;       /* Is the user's tty in cbreak mode? */
char *progname;         /* The name of this program */
int resume= 0;		/* Have we been suspended? */
int lines_above= 0;	/* # Displayed lines above we could backwarp into */
int junk_above= 0;	/* lines of discardable junk prompts above */
int base_col= 0;	/* Logical base column to tab from */


int main(int argc,char **argv)
{
    char bf[2][BUFSIZE+18];	/* extra storage for ~ expansion and such */
    int wcol;
    int i;
    int sw=0;

    progname= argv[0];

#ifdef NOVICE
    novice= (getenv("NOVICE") != NULL);		/* used on M-Net */
#endif /*NOVICE*/
    parse_opts(getenv("GATEOPTS"),"GATEOPTS");

    for (i= 1; i < argc; i++)
    {
	if (argv[i][0] == '-')
	{
	    if (argv[i][1] == '-')
		parse_opts(argv[i]+2,"option");
	    else
		goto usage;
	}
	else if (filename == NULL)
	    filename= argv[i];
	else
	    goto usage;
    }
    if (filename == NULL) goto usage;

    initterm();
    if (maxcol > mycols - 1) maxcol= mycols - 1;
    if (hotcol > mycols - 1) hotcol= mycols - 1;

    initmodes();

    puts(HELP_MESG);

    if (openfile(filename,0)) exit(RET_ABORT);

    /* Put up the signal handlers */
#ifdef HAVE_SIGACTION
    sigact(SIGHUP, hup, NULL);
    sigact(SIGQUIT, intr, NULL);
    sigact(SIGINT, intr, NULL);
    sigact(SIGTSTP, susp, NULL);
#else
    signal(SIGHUP,(void (*)())hup);
    signal(SIGQUIT,(void (*)())intr);
    signal(SIGINT,(void (*)())intr);
#ifdef SIGTSTP
    signal(SIGTSTP,(void (*)())susp);
#endif
#endif

    set_mode(1);

    wcol= 0;
    for (;;)
    {
	if (ggetline(bf[sw], bf[!sw], &wcol, prompt))
	{
	    putchar('\n');
	    done(RET_ENTER);
	    /* in case exit was cancelled */
	    clearerr(stdin);
	    continue;
	}
	if (bf[sw][0] == '.' && bf[sw][1] == '\n')
	{
	    done(RET_ENTER);
	    /* in case exit was cancelled */
	    continue;
	}
	if (bf[sw][0] == '!')
	{
	    dosystem(bf[sw]+1);
	    resume= 0;
	}
	else if (bf[sw][0] == *cmdchar)
	{
	    docolon(bf[sw]+1);
	    resume= 0;
	}
	else
	{
	    if (++lines_above > mylines) lines_above= mylines-1;
	    if (novice) novice_error(bf[sw]);
	    fputs(bf[sw],tfp);
	    fflush(tfp);
	}
	sw= !sw;
    }

usage:
    version();
    fprintf(stderr,"usage: %s [--<option>...] <filename>\n",progname);
    exit(RET_ABORT);
}

#ifdef YAPP
#ifdef RIVER
#define CF "River yapp"
#else
#define CF "yapp"
#endif
#else
#define CF "picospan"
#endif

#ifdef NO_SPELL
#define SP "no spell"
#endif
#ifdef ASPELL
#define SP "new aspell"
#endif
#ifdef OLDASPELL
#define SP "old aspell"
#endif
#ifdef IISPELL
#define SP "int'l ispell"
#endif
#ifdef GISPELL
#define SP "ispell-4"
#endif
#ifdef SIMPLE_SPELL
#define SP "unix spell"
#endif
#ifdef PLUS_SPELL
#define SP "unix spell+"
#endif

void version()
{
    fprintf(stderr,"Gate %s (%s; %s) - (c) 1995, Jan Wolter\n",
	version_number,CF,SP);
}
