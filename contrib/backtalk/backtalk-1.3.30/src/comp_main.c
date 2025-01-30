/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "str.h"
#include "sysdict.h"
#include "comp.h"
#include "comp_tok.h"
#include "comp_util.h"

char *progname;		/* Name of compiler */
int  curr_line;		/* Current line number of input file (0 = None) */
int  verbose= 0;	/* Run in verbose mode */
int  curr_file= -2;	/* Index of current source file in file list */
int  initializing;	/* Are we compiling config.bt? */

#ifdef DYNALOAD
#include <dlfcn.h>
#ifndef RTLD_LAZY               /* Linux, Solaris, AIX, SGI */
# ifdef DL_LAZY
#  define RTLD_LAZY DL_LAZY     /* OpenBSD */
# else
#  define RTLD_LAZY 1           /* SunOS */
# endif
#endif
void *prog_handle;	/* dlopen handle for main program */
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* COMPILE(fp) - Compile whatever we read from the given open input file
 * and store the compiled version in memory.  The calling program should
 * first do a start_proc() or init_comp(), and then do an end_proc() after
 * we return.
 */

compile(FILE *ifp)
{
    Token *tk, *dv;
    HashEntry *h;
    int oldline;

    /* Save old line number, but start numbering lines from one */
    oldline= curr_line;
    curr_line= 1;

    /* Process each token in the input file in sequence */
    while ((tk= get_token(ifp)) != NULL)
    {
	/* Figure out what kind of token we have */
	if (tk->flag & TKT_SYMBOL)
	{
	    if (tk->flag & TKT_CONSTANT)
	    {
		/* Literal */
		if ((h= FindHashEntry(&syshash, sval(*tk))) != NULL)
		{
		    /* System Dictionary Literal -- push sysdict offset */
		    add_tok(TK_BOUND_LITERAL, tk->sfid, tk->line,
			(void *)(h-sysdict));
		    free(sval(*tk));
		    continue;
		}
	    }
	    else
	    {
		/* Symbol */
		if (!strcmp(sval(*tk),"{"))				/* } */
		{
		    /* Start Procedure */
		    start_proc(tk->sfid,tk->line);
		    free(sval(*tk));
		    continue;
		}							/* { */
		else if (!strcmp(sval(*tk),"}"))
		{
		    /* End Procedure */
		    if (end_proc())				        /* { */
			    die("unmatched }");
		    free(sval(*tk));
		    continue;
		}
		else if ((h= FindHashEntry(&constdict, sval(*tk))) != NULL)
		{
		    /* Constant symbol -- append its value to the program */
		    free(sval(*tk));
		    dv= &(h->t);
		    if (class(*dv) == TKC_STRING)
			add_tok(dv->flag,tk->sfid,tk->line,
				(void *)strdup(sval(*dv)));
		    else
			add_tok(dv->flag,tk->sfid,tk->line,dv->val);
		    continue;
		}
		else if ((h= FindHashEntry(&syshash, sval(*tk))) != NULL)
		{
		    free(sval(*tk));

		    if ((h->flag&DT_CONSTANT) && class(h->t) != TKC_FUNCTION)
		    {
			/* System Dictionary constant -- replace with value */
			dv= &(h->t);
			if (class(*dv) == TKC_STRING)
			    add_tok(dv->flag,tk->sfid,tk->line,
			    	    (void *)strdup(sval(*dv)));
			else
			    add_tok(dv->flag,tk->sfid,tk->line,dv->val);
		    }
		    else
		    {
			/* System Dictionary variable -- use sysdict offset */
			add_tok(TK_BOUND_SYMBOL,tk->sfid,tk->line,
			    (void *)(h-sysdict));

			/* If it is a function, check if we can collapse it */
			if (type(h->t) == TK_FUNCTION)
			    (*(fval(h->t)))();
#ifdef DYNALOAD
			else if (type(h->t) == TK_DYNAMIC)
			{
			    void (*func)();
			    func= (void (*)())dlsym(prog_handle, sval(h->t));
			    if (func) (*func)();
			}
#endif
		    }

		    continue;
		}
	    }
	}

	/* Everything else:  just add it to the program */
	add_token(tk);
    }

    /* Reset the parent file's line number */
    curr_line= oldline;
}


/* COMPILE_FILE -- open and compile the given filename into memory */

void compile_file(char *filename)
{
    int old_file;
    FILE *fp;

    /* Open the source */
    if ((fp= fopen(filename,"r")) == NULL)
    {
	 fprintf(stderr,"%s: cannot open %s to read\n", progname, filename);
	 exit(1);
    }

    /* Set the current file index */
    old_file= curr_file;
    curr_file= add_file_name(filename);

    /* Compile the selected script */
    init_comp(curr_file);
    compile(fp);

    /* Restore the old file index */
    curr_file= old_file;

    /* End the procedure */
    if (!end_proc())
	die("unmatched { in %s",filename);				/* } */

    fclose(fp);

}


/* IS_NEW -- given a source and binary file name, see if the source is newer.
 */

int is_new(char *source_name, char *binary_name)
{
    struct stat source, binary;
    char bf[4];
    int fd;

    if (stat(source_name, &source))
    {
	 fprintf(stderr,"%s: cannot access %s\n", progname, source);
	 exit(1);
    }

    /* Compare last modification times */
    if (stat(binary_name, &binary) || source.st_mtime > binary.st_mtime)
    	return 1;

    if ((fd= open(binary_name,O_RDONLY)) < 0)
    	return 0;

    /* Check magic number of binary file - recompile if it is wrong */
    if (read(fd,bf,4) != 4)
    	goto eof;
    if (strncmp(bf,MAGIC,4))
    	return 1;

    if (read(fd,bf,4) != 4)
    	goto eof;

    close(fd);

    /* Check version number of binary file - recompile if it is wrong */
    return (bf[0] != VERSION_A || bf[1] != VERSION_B ||
            bf[2] != VERSION_C || bf[3] != VERSION_D);

eof:
    close(fd);
    return 1;
}


main(int argc, char **argv)
{
    char *source= NULL;
    char *binary;
    char *scriptdir= strdup(sdict(VAR_SCRIPTDIR));
    int i,rc;
    char *t;
    FILE *fp;

	/* Set default umask */
	umask(022);

	/* Figure out if this is a big_endian machine */
	check_endian();

	/* Set program name */
	if ((progname= strrchr(argv[0],'/')) == NULL)
		progname= argv[0];
	else
		progname++;

#ifdef DYNALOAD
	prog_handle= dlopen(NULL,RTLD_LAZY);
#endif

	/* Parse command line arguments */
	for (i= 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
			switch(argv[i][1])
			{
			case 'v':
				verbose= 1;
				break;
			case 'c':
				i++;
				if (i >= argc) goto usage;
				free(scriptdir);
				scriptdir= strdup(argv[i]);
				break;
			default:
				goto usage;
			}
		else if (source == NULL)
			source= argv[i];
		else
			goto usage;
	}
	if (source == NULL) goto usage;

	/* Initialize dictionaries */
	init_dict();

	/* cd into the scriptdir directory */
	if (chdir(scriptdir))
	{
		 fprintf(stderr,"%s: script directory \"%s\" not accessible\n",
			 progname, scriptdir);
		 exit(1);
	}

	/* Construct source and object file names */
	if (sb_name(source, &source, &binary))
	{
		 fprintf(stderr,"%s: file name \"%s\" too long\n", progname,
		 	source);
		 exit(1);
	 }
	 /* Copy this into non-static storage */
	 binary= strdup(binary);

	 /* Set the "flavor" system dictionary constant to first component of
	  * our source file's path name, and "scriptname" to the rest.
	  */
	 if ((t= strchr(source,'/')) != NULL)
	 {
	 	char tmp= t[1];
	 	t[1]= '\0';
	 	sysdict[VAR_FLAVOR].t.val= strdup(source);
	 	sysdict[VAR_FLAVOR].t.flag|= TKF_FREE;
	 	t[1]= tmp;
	 	sysdict[VAR_SCRIPTNAME].t.val= strdup(t+1);
	 	sysdict[VAR_SCRIPTNAME].t.flag|= TKF_FREE;
	}

	/* First compile the configuration file */
	initializing= TRUE;
	compile_file("config.bt");
	initializing= FALSE;

	/* Write out the compiled config file only if it is new */
	if (is_new("config.bt", "config.bb"))
	{
	    /* Open the output file */
	    if ((fp= fopen("config.bb","w")) == NULL)
	    {
		 fprintf(stderr,
		     "%s: cannot create/open %s/config.bb to write (euid=%d)\n",
		     progname,scriptdir,geteuid());
		 exit(1);
	    }

	    /* Write out the compiled program */
	    if (rc= write_prog(fileno(fp)))
	    {
	    	/* Probably disk is full - delete any partial binary and die */
	    	unlink("config.bb");
		die("Could not write config.bb: %s",strerror(rc));
	    }
	    fclose(fp);
	}

	/* If we are compiling the config file, don't do it twice */
	if (!strcmp(source,"config.bt"))
	{
	    if (verbose) dump_prog();
	    exit(0);
	}

	/* Throw away the compiled config file - we only wanted side effects */
	drop_program();

	/* "config.bt" may have changed the scriptdir -- cd to new one */
	if (strcmp(sdict(VAR_SCRIPTDIR),scriptdir) &&
	    chdir(sdict(VAR_SCRIPTDIR)))
	{
		fprintf(stderr,"%s: script directory \"%s\" not accessible\n",
			  progname, sdict(VAR_SCRIPTDIR));
		exit(1);
	}
	free(scriptdir);

	/* Put source file name into file list */
	compile_file(source);

	/* Dump the result of the compilation */
	if (verbose) dump_prog();

	/* Open the output file */
	if ((fp= fopen(binary,"w")) == NULL)
	{
	     fprintf(stderr,"%s: cannot open %s\n", progname, binary);
	     exit(1);
	}
	free(binary);

	/* Write out the compiled program */
	if (rc= write_prog(fileno(fp)))
	{
	    /* Probably disk is full - delete any partial binary and die */
	    unlink(binary);
	    die("Could not write %s: %s",binary,strerror(rc));
	}
	/*fclose(fp);*/

	exit(0);

usage:	fprintf(stderr,"Usage: %s [-v] [-c <dir>] <source>\n",progname);
	exit(1);
}


#if __STDC__
void die(const char *fmt,...)
#else
void die(fmt, va_alist)
    const char *fmt;
    va_dcl
#endif
{
    va_list ap;

    VA_START(ap,fmt);

    fprintf(stderr, "%s: Error compiling %s",
    	progname, find_file_name(curr_file));

    if (curr_line) fprintf(stderr," (line %d)", curr_line);

    fputs(": ",stderr);
    vfprintf(stderr, fmt, ap);
    fputs("\n",stderr);

    va_end(ap);

    exit(1);
}
