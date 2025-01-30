/* PARTY PROGRAM -- OPTION AND COMMAND TABLE - Jan Wolter */

/* The "opt.h" file is generated from this by the "makeopt" program.
 */

#include "party.h"

/* This is the table of options available in the party program.
 * Note that all channel options must be first, followed by all
 * system options, and then all user options.
 */

struct optent opt[] = {

	/* All PF_CHN options must be first.  These are always reset to their
	 * default values before entering a new channel.  They are normally
	 * the only ones you would be setting in the chantab.
	 */

    { "askname",      PF_CHN|PF_NO,                  0, NULL },
    { "chanintro",    PF_CHN|PF_NO|PF_STR,           0, "" },
    { "keeplog",      PF_CHN|PF_NO|PF_SHOW,          0, NULL},
    { "idleout",      PF_CHN|PF_NO|PF_STR|PF_NUM,    0, DFLT_IDLEOUT },
    { "makenoise",    PF_CHN|PF_NO|PF_STR|PF_SHOW,   1, DFLT_MAKENOISE },
    { "mapname",      PF_CHN|PF_NO|PF_STR,           0, "" },
#ifndef NOCLOSE
    { "mayclose",     PF_CHN|PF_NO|PF_SHOW,          0, NULL},
#endif
    { "readlim",      PF_CHN|PF_STR|PF_NUM|PF_SHOW,  0, DFLT_READLIM },
    { "rename",       PF_CHN|PF_NO|PF_SHOW,          0, NULL },
    { "randname",     PF_CHN|PF_NO|PF_STR,           0, "" },
    { "uidname",      PF_CHN|PF_NO,                  1, NULL },
    { "uniquename",   PF_CHN|PF_NO,                  1, NULL },

	/* The options below are set only in the partytab */

    { "chantab",      PF_SYS|PF_STR,                1, DFLT_CHANTAB },
    { "dir",          PF_SYS|PF_STR,                1, DFLT_DIR },
    { "userlist",     PF_SYS|PF_NO,                 0, NULL },
    { "whofile",      PF_SYS|PF_STR,                1, DFLT_WHOFILE },
    { "capacity",     PF_SYS|PF_NO|PF_STR|PF_NUM,   0, DFLT_CAPACITY },

	/* The options below are set by the user or in the partytab */

    { "alias",        PF_STR|PF_SHOW,               1, NULL },
    { "arg",          PF_NO,                        1, NULL },
    { "bs",           PF_NO|PF_SEE|PF_SHOW,         1, NULL },
    { "back",         PF_NUM|PF_STR,                1, DFLT_BACK },
    { "cols",         PF_NUM|PF_STR|PF_SHOW,        1, DFLT_COLS },
    { "colon",        PF_NO,                        1, NULL },
    { "control",      PF_NO|PF_SEE|PF_SHOW,         1, NULL },
    { "debug",        PF_NO|PF_STR,                 0, DFLT_DEBUG },
    { "env",          PF_NO,                        1, NULL },
    { "fastshell",    PF_NO|PF_SHOW,                1, NULL },
    { "filter",       PF_STR|PF_NO|PF_SHOW,         1, DFLT_FILTER },
    { "firstchar",    PF_NO|PF_SHOW,                0, NULL },
    { "fullmesg",     PF_STR,                       1, DFLT_FULLMESG },
    { "help",         PF_NO|PF_STR,                 1, DFLT_HELP },
    { "intro",        PF_STR,                       1, DFLT_INTRO },
#ifndef NOCLOSE
    { "knockwait",    PF_NUM|PF_STR,		    1, DFLT_KNOCKWAIT },
#endif
    { "maildir",      PF_STR|PF_SHOW,               1, DFLT_MAILDIR },
    { "prompt",       PF_STR,                       1, DFLT_PROMPT },
    { "raw",          PF_NO,                        0, NULL },
    { "repeat",       PF_NO|PF_SHOW,                1, NULL },
    { "shell",        PF_NO|PF_STR|PF_SHOW,         1, DFLT_SHELL },
    { "shownoise",    PF_NO|PF_SHOW,                1, NULL },
    { "showread",     PF_NO|PF_SHOW,                1, NULL },
    { "showevent",    PF_NO|PF_SHOW,                1, NULL },
    { "spaceonly",    PF_NO|PF_STR,                 0, DFLT_SPACEONLY },
    { "start",        PF_STR,                       1, DFLT_START },
    { "tpmorp",       PF_STR,                       1, DFLT_TPMORP },
    { "wrap",         PF_NO|PF_SHOW|PF_STR|PF_NUM,  0, "10" },
	
	/* This marks the end of the option list */

    { NULL,           0,                            0, NULL } };


/* This is the table of commands recognized at the colon prompt.  They need
 * be in no partitular order.  The numbers are the minimum length that is
 * recognized as an abbreviation of the command.
 */

struct cmdent cmd[] = {
	{"back",	1},
	{"chantab",	1},
#ifndef NOCLOSE
	{"close",	2},
#endif
	{"help",	1},
#ifndef NOIGNORE
	{"ignore",	2},
#endif
#ifndef NOCLOSE
	{"invite",	2},
#endif
	{"join",	1},
	{"list",	1},
	{"name",	1},
	{"noises",	3},
#ifndef NOIGNORE
	{"notice",	3},
#endif
#ifndef NOCLOSE
	{"open",	1},
#endif
	{"print",	1},
	{"pwho",	2},
	{"quit",	1},
	{"read",	1},
	{"save",	2},
	{"set",		1},
	{"shell",	2},
	{"version",	1},
	{"who",		1},
	/* This marks the end of the command list */
	{ NULL,		0}};
