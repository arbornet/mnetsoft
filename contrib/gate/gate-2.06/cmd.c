#include "gate.h"

struct ct {
	char *name;
	void (*func)();
	} cmdtab[]= {

	{ "?",		cmd_help    },
	{ "cl_ear",	cmd_empty   },
	{ "_edit",	do_edit     },
	{ "em_pty",	cmd_empty   },
	{ "ex_it",	cmd_quit    },
	{ "h_elp",	cmd_help    },
	{ "o_k",	cmd_ok      },
	{ "p_rint",	cmd_print   },
	{ "q_uit",	cmd_quit    },
	{ "r_ead",	cmd_read    },
	{ "se_t",	cmd_set     },
#ifndef NO_SPELL
	{ "sp_ell",	cmd_spell   },
#endif
	{ "s_ubstitute",cmd_subs    },
	{ "v_isual",	do_edit     },
	{ "ve_rsion",	version     },
	{ "w_rite",	cmd_write   },

	{ NULL,		NULL }};

/* DOCOLON -- Process a colon escape command. */

void docolon(char *cmd)
{
    char *cmdend;
    char *arg;
    struct ct *ctp;

    cmd= firstout(cmd," \t");
    lines_above= 0;

    /* Single Character Commands: */
    switch(cmd[0])
    {
    case '!':
	dosystem(cmd+1);
	return;

    case '/':
	cmd_subs(cmd);
	return;

    case '|':
	dopipe(cmd+1);
	return;
    }

    cmdend= firstin(cmd," /\t\n");
    arg= firstout(cmdend," \t\n");
    *cmdend= '\0';

    if (*arg == '\0' && *(--cmdend) == '!')
    {
	*cmdend= '\0';
	arg= "!";
    }

    /* Command words */
    for (ctp= cmdtab; ctp->name != NULL; ctp++)
	if (isprefix(cmd,ctp->name))
	{
	    (*ctp->func)(arg);
	    return;
	}

    printf("Unknown colon command '%s' - ignored.\n",cmd);
    return;
}

void cmd_print(char *arg)
{
    fseek(tfp,0L,0);
    printf("---\n");
    typetoend(outcol(prompt,BUFSIZE,0));
}

void cmd_help(char *arg)
{
    typefile(HELP_FILE);
    puts("(Continue your text entry)");
}

void cmd_empty(char *arg)
{
    puts("Enter your text:");
    puts(HELP_MESG);
    emptyfile();
}


void cmd_read(char *arg)
{
    int flg= 1;

    if (arg[0] == '-' && arg[1] == 's' &&
	    (arg[2] == '\0' || isspace(arg[2])) )
    {
	flg= 0;
	arg= firstout(arg+2," \t");
    }
    *firstin(arg," \t\n")= '\0';
    if (*arg == '\0')
	printf("No filename given on command\n");
    else
	read_file(arg,flg);
}


void cmd_write(char *arg)
{
    *firstin(arg," \t\n")= '\0';
    if (*arg == '\0')
	printf("No filename given on command\n");
    else
	write_file(arg);
}


void cmd_set(char *arg)
{
    if (*arg == '\0')
	print_opts();
    else
    {
	int was_secure= secure;
	parse_opts(arg,":set");
	if (secure != was_secure)
	    fchmod(fileno(tfp),secure ? 0600 : 0644);
    }
}


void cmd_quit(char *arg)
{
    if (*arg == '!' || readyes("OK to abandon text? "))
	done(RET_ABORT);
}


void cmd_ok(char *arg)
{
    done(RET_ASKOK);
}

void cmd_subs(char *cmd)
{
    char *arg;
    int n;

    if (cmd[0] != '/' && cmd[0] != '\0')
    {
	printf("incorrect substitute syntax\n");
	return;
    }
    arg= cmd++;
    for(;;)
    {
	if ((arg= strchr(arg+1,'/')) == NULL)
	{
	    printf("missing replacement text on substitution command\n");
	    return;
	}
	if (arg[-1] != '\\')
	    break;
    }
    *(arg++)= '\0';
    if (*cmd == '\0')
    {
	printf("zero-length pattern not allowed on substitution command\n");
	return;
    }

    /* Ignore final newline and / if they are there */
    n= strlen(arg);
    if (arg[n-1] == '\n') arg[--n]= '\0';
    if (arg[n-1] == '/')
    {
	    arg[--n]= '\0';
	    if (arg[n-1] == '\\') arg[n-1]= '/';
    }

    junk_above= 0;
    do_substitute(cmd,arg);
}
