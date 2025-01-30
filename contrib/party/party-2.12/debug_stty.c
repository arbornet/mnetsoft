/* PARTY PROGRAM -- STTY DEBUGGING ROUTINES -- Jan Wolter
 *
 * This file is for debugging support only.  If you don't have DEBUG_STTY set,
 * you don't need it.
 */

#ifdef DEBUG_STTY

#include <stdio.h>
#include <ctype.h>
#include "party.h"

struct ct {
        unsigned char   cc;
        char            *name;
        };

struct ft {
        unsigned long   flag;
        char            *name;
        };

#ifdef F_STTY
struct ft flagtab[] = {
	{TANDEM,	"tandem"},
	{CBREAK,	"cbreak"},
	{LCASE,		"lcase"},
	{ECHO,		"echo"},
	{CRMOD,		"crmod"},
	{RAW,		"raw"},
	{ODDP,		"oddp"},
	{EVENP,		"evenp"},
	{NL1,		"nl1"},
	{NL2,		"nl2"},
	{NL3,		"nl3"},
	{TAB1,		"tab1"},
	{TAB2,		"tab2"},
	{CR1,		"cr1"},
	{CR2,		"cr2"},
	{FF1,		"ff1"},
	{BS1,		"bs1"},
	{CRTBS,		"crtbs"},
	{PRTERA,	"prtera"},
	{CRTERA,	"crtera"},
#ifdef EUCBKSP
	{EUCBKSP,	"eucbksp"},
#endif
#ifdef TILDE
	{TILDE,		"tilde"},
#endif
	{MDMBUF,	"mdmbuf"},
	{LITOUT,	"litout"},
	{TOSTOP,	"tostop"},
	{FLUSHO,	"flusho"},
	{NOHANG,	"nohang"},
	{PASS8OUT,	"pass8out"},
	{CRTKIL,	"crtkil"},
	{PASS8,		"pass8"},
	{CTLECH,	"ctlech"},
	{PENDIN,	"pendin"},
	{DECCTQ,	"decctq"},
	{NOFLSH,	"noflsh"},
        {0,             NULL}};
#endif /*F_STTY*/

#ifdef F_TERMIOS
struct ct cchartab[] = {
	{VEOF,		"eof"},
	{VEOL,		"eol"},
#ifdef VEOL2
	{VEOL2,		"eol2"},
#endif
	{VERASE,	"erase"},
#ifdef VWERASE
	{VWERASE,	"werase"},
#endif
	{VKILL,		"kill"},
#ifdef VREPRINT
	{VREPRINT,	"reprint"},
#endif
	{VINTR,		"intr"},
	{VQUIT,		"quit"},
	{VSUSP,		"susp"},
#ifdef VDSUSP
	{VDSUSP,	"dsusp"},
#endif
	{VSTART,	"start"},
	{VSTOP,		"stop"},
#ifdef VLNEXT
	{VLNEXT,	"lnext"},
#endif
#ifdef VDISCARD
	{VDISCARD,	"discard"},
#endif
	{VMIN,		"min"},
	{VTIME,		"time"},
#ifdef VSTATUS
	{VSTATUS,	"status"},
#endif
        {0,             NULL}};

        
struct ft iflagtab[] = {
        {IGNBRK,        "ignbrk"},
        {BRKINT,        "brkint"},
        {IGNPAR,        "ignpar"},
        {PARMRK,        "parmrk"},
        {INPCK,         "inpck"},
        {ISTRIP,        "istrip"},
        {INLCR,         "inlcr"},
        {IGNCR,         "igncr"},
        {IXON,          "ixon"},
        {IXOFF,         "ixoff"},
#ifdef IXANY
        {IXANY,         "ixany"},
#endif
#ifdef IXANY
        {IMAXBEL,       "imaxbel"},
#endif
        {0,             NULL}};

struct ft oflagtab[] = {
        {OPOST,         "opost"},
#ifdef ONLCR
        {ONLCR,         "onlcr"},
#endif
#ifdef OXTABS
        {OXTABS,        "oxtabs"},
#endif
#ifdef ONOEOT
        {ONOEOT,        "onoeot"},
#endif
        {0,             NULL}};

struct ft cflagtab[] = {
#ifdef CIGNORE
        {CIGNORE,       "cignore"},
#endif
        {CS6,           "cs6"},
        {CS7,           "cs7"},
        {CS8,           "cs8"},
        {CSTOPB,        "cstopb"},
        {CREAD,         "cread"},
        {PARENB,        "parenb"},
        {PARODD,        "parodd"},
        {HUPCL,         "hupcl"},
        {CLOCAL,        "clocal"},
#ifdef CRTS_OFLOW
        {CRTS_OFLOW,    "crts_oflow"},
#endif
#ifdef CRTS_IFLOW
        {CRTS_IFLOW,    "crts_iflow"},
#endif
#ifdef MDMBUG
        {MDMBUG,        "mdmbuf"},
#endif
        {0,             NULL}};

struct ft lflagtab[] = {
#ifdef ECHOKE
        {ECHOKE,        "echoke"},
#endif
        {ECHOE,		"echoe"},
        {ECHOK,		"echok"},
        {ECHO,		"echo"},
        {ECHONL,	"echonl"},
#ifdef ECHOPRT
        {ECHOPRT,	"echoprt"},
#endif
#ifdef ECHOCTL
        {ECHOCTL,	"echoctl"},
#endif
        {ISIG,		"isig"},
        {ICANON,	"icanon"},
#ifdef ALTWERASE
        {ALTWERASE,	"altwerase"},
#endif
        {IEXTEN,	"iexten"},
        {EXTPROC,	"extproc"},
        {TOSTOP,	"tostop"},
#ifdef FLUSHO
        {FLUSHO,	"flusho"},
#endif
#ifdef NOKERNINFO
        {NOKERNINFO,	"nokerninfo"},
#endif
#ifdef PENDIN
        {PENDIN,	"pendin"},
#endif
        {NOFLSH,	"noflsh"},
        {0,             NULL}};
#endif /*F_TERMIOS*/

int pscnt;

#ifdef F_STTY
printstty(fp,t)
FILE *fp;
struct sgttyb *t;
{
int i;

	pscnt= 0;

	for (i= 0; flagtab[i].name != NULL; i++)
		psflg(fp,flagtab[i].name, t->sg_flags & flagtab[i].flag);

	fprintf(fp,"\nerase %d kill %d ", t->sg_erase,t->sg_kill);

	fprintf(fp,"ispeed %d ospeed %d\n", t->sg_ispeed,t->sg_ospeed);
}
#endif /* F_STTY */


#ifdef F_TERMIOS
printstty(fp,t)
FILE *fp;
struct termios *t;
{
int i;

	pscnt= 0;

	for (i= 0; iflagtab[i].name != NULL; i++)
		psflg(fp,iflagtab[i].name, t->c_iflag & iflagtab[i].flag);

	for (i= 0; oflagtab[i].name != NULL; i++)
		psflg(fp,oflagtab[i].name, t->c_oflag & oflagtab[i].flag);

	for (i= 0; cflagtab[i].name != NULL; i++)
		psflg(fp,cflagtab[i].name, t->c_cflag & cflagtab[i].flag);

	for (i= 0; lflagtab[i].name != NULL; i++)
		psflg(fp,lflagtab[i].name, t->c_lflag & lflagtab[i].flag);

	for (i= 0; cchartab[i].name != NULL; i++)
		pskey(fp,cchartab[i].name, t->c_cc[cchartab[i].cc]);
	
	fprintf(fp,"\nispeed %d ospeed %d\n", t->c_ispeed,t->c_ospeed);
}
#endif /* F_TERMIOS */

psflg(fp,name,flag)
FILE *fp;
char *name;
unsigned int flag;
{
	if (pscnt > 8)
	{
		putc('\n',fp);
		pscnt= 0;
	}
	if (pscnt != 0) putc(' ',fp);
	if (!flag) putc('-',fp);
	fputs(name,fp);
	pscnt++;
}

pskey(fp,name,key)
FILE *fp;
char *name;
unsigned char key;
{
	if (pscnt > 8)
	{
		putc('\n',fp);
		pscnt= 0;
	}
	if (pscnt != 0) putc(' ',fp);
	fprintf(fp,"%s %d",name,key);
	pscnt++;
}
#endif /*DEBUG_STTY*/
