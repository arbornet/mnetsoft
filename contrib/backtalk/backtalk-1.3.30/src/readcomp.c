/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "backtalk.h"

#include <sys/stat.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include "sysdict.h"
#include "readcomp.h"
#include "byteorder.h"
#include "comp.h"
#include "comp_tok.h"
#include "waittype.h"
#include "str.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

extern int errno;

/* This is the list of source files out of the header of the binary file. */
char **filelist= NULL;
byte filelist_size= 0;
byte sfid_offset;


/* As we read tokens, this is the current file that they are associated with */
byte curr_file;

/* READ_VAL -- read an integer of up to four bytes in length from a binary
 * file where the integer is store in network byte order.  Returns 0 on eof,
 * dies on error.
 */

int read_val(int fd, int size, void *val)
{
    unsigned char tmp;
    int i;

    size= read(fd, (char *)val, size);
    if (size < 0) die("read error: %s", strerror(errno));

    /* On big endian-machines or if value is less than 2 bytes it is OK */
    if (big_endian || size < 2) return size;

    /* little-endian machines - shuffle into big endian */
    for (i= 0; i < size/2; i++)
    {
	tmp= ((char *)val)[i];
	((char *)val)[i]= ((char *)val)[size-i-1];
	((char *)val)[size-i-1]= tmp;
    }

    return size;
}


/* READ_TOKEN -- Read in a token from a compiled script.  The memory for the
 * Token itself should already have been allocated.  This routine will allocate
 * up the memory to store procedure arrays and strings in.
 */

void read_token(int fd, Token *t)
{
    unsigned int len;
    int i, rc, v;
    char *flat;
    Regex *reg;

    /* Read the type field - if it is a file change token, skip it */
    for (;;)
    {
	if (read_val(fd, 2, (void *)&t->flag) != 2) goto eof;

	if (type(*t) != TK_UNDEF) break;

	if (read_val(fd, 1, (void *)&curr_file) != 1) goto eof;
	curr_file+= sfid_offset;
    }

    if (read_val(fd, 2, (void *)&t->line) != 2) goto eof;
    t->sfid= curr_file;

    if (type(*t) == TK_PROCEDURE || type(*t) ==  TK_ARRAY)
    {
	/* Procedures and Arrays */
	/* (actually, we don't compile arrays yet) */

	/* Read array size */
	if (read_val(fd, 4, (void *)&len) != 4) goto eof;

	/* Create an array for procedure tokens */
	t->val= (void *)malloc(sizeof(Array));
	aval(*t)->lk= 1;
	aval(*t)->sz= len;
	aval(*t)->a= (Token *)(len > 0 ? malloc(len * sizeof(Token)) : NULL);

	for (i= 0; i < len; i++)
	    read_token(fd,(aval(*t)->a)+i);
    }
    else if (class(*t) == TKC_INTEGER)
    {
	/* Integers and bound symbols */

	if (read_val(fd, 4, (void *)&v) != 4) goto eof;
	t->val= (void *)(long)v;
    }
    else if (class(*t) == TKC_STRING)
    {
	/* Strings and unbound symbols */
	if (read_val(fd, 4, (void *)&len) != 4) goto eof;
	t->val= (void *)malloc(len+1);
	t->flag|= TKF_FREE;

	if ((rc= read(fd,sval(*t),len)) != len) goto eof;
	sval(*t)[len]= '\0';
    }
    else if (class(*t) == TKC_REGEX)
    {
	/* Compiled regular expressions */
	if (read_val(fd, 4, (void *)&len) != 4) goto eof;
	flat= (void *)malloc(len);
	if ((rc= read(fd,flat,len)) != len) goto eof;
	reg= (Regex *)malloc(sizeof(Regex));
	flattoreg(flat, &(reg->re));
	free(flat);
	reg->lk= 1;
	t->val= (void *)reg;
	t->flag|= TKF_FREE;
    }
    else
	die("bug: strange token type %d",type(*t));

    return;

eof:
    if (rc < 0)
	die("read error in compiled program: %s", strerror(errno));
    else
	die("premature end-of-file in compiled program");
}


/* FREE_FILE_LIST - Deallocate the memory allocated to the file list.  Normally
 * we only do this if we are hunting for memory leaks.
 */

void free_file_list()
{
    int i;

    if (filelist != NULL)
    {
	for (i= 0; i < filelist_size; i++)
	    free(filelist[i]);
	free(filelist);
    }
    filelist_size= 0;
}


/* READ_FILE_LIST - Load the list of source filenames into the filelist array.
 * If a file list has previously been loaded, this
 */

void read_file_list(int fd)
{
    unsigned int len;
    byte sz;
    int i, rc;

    if (read_val(fd, 1, (void *)&sz) != 1) goto eof;
    sfid_offset= filelist_size;
    filelist_size+= sz;

    if (sfid_offset > 0)
	filelist= (char **)realloc(filelist,
	    filelist_size * sizeof(char *));
    else
	filelist= (char **)malloc(filelist_size * sizeof(char *));

    for (i= sfid_offset; i < filelist_size; i++)
    {
	if (read_val(fd, 4, (void *)&len) != 4) goto eof;
	filelist[i]= (char *)malloc(len+1);

	if ((rc= read(fd, filelist[i], len)) != len) goto eof;
	filelist[i][len]= '\0';
    }
    return;

eof:
    if (rc < 0)
	die("read error in program header: %s", strerror(errno));
    else
	die("premature end-of-file in compiled program header");
}


/* GETLISTFILE - Look up a file in the list loaded by read_file_list().
 */

char *getlistfile(int index)
{
    if (index < 0 || index >= filelist_size)
	die ("file list index %d out of range", index);
    return filelist[index];
}




/* READ_SCRIPT - Load a script given the filename of either the compiled or
 * uncompiled script.  Check if the script needs recompilation, and do it if
 * necessary.  Return a pointer to the compiled program upon success and
 * a NULL on failure.
 */

Token *read_script(char *fname)
{
    char *sfile, *bfile;
    Token *head;
    char bf[4];
    int i,fd,rc,old_sz;
    int len= strlen(fname);
    struct stat bst, st;
    int recomp;

    /* Generate source and binary names */
    if (sb_name(fname,&sfile,&bfile))
    	die("filename \"%s\" too long\n",fname);

    for (i= 0; ; i++)
    {
	recomp= 0;

	/* Open the binary file */
	if ((fd= open(bfile,0)) < 0)
	{
	    /* Binary does not exist */
	    if (access(sfile,4))
	    {
		/* Check for subdirectory */
		if (access(fname,5) || strlen(fname) > BFSZ-8)
		    die_noscript(fname);
		else
		{
		    /* Try opening 'begin' script inside directory */
		    static char f[BFSZ];
		    strcpy(f,fname);
		    strcat(f,"/begin");
		    return read_script(f);
		}
	    }
	    else if (i > 0)
	    	die("Cannot compile %s",sfile);
	    else
		recomp= 1;
	}
	else
	{
	    /* Read and Check Magic number */
	    if ((rc= read(fd, bf, 4)) != 4)
	    	goto eof;
	    if (strncmp(bf,MAGIC,4))
		die("bad magic number in compiled script file \"%s\"",bfile);

	    /* Read and Check the Version number */
	    if ((rc= read(fd, bf, 4)) != 4)
	    	goto eof;
	    if (bf[0] != VERSION_A || bf[1] != VERSION_B ||
	        bf[2] != VERSION_C || bf[3] != VERSION_D)
	    {
		if (i > 0)
		    /* Error persists after recompiling */
		    die("version mismatch between Backtalk compiler "
		        "(%d.%d.%d.%d) and interpretor (%d.%d.%d.%d)",
			bf[0],bf[1],bf[2],bf[3],
			VERSION_A, VERSION_B, VERSION_C, VERSION_D);
		else
		    recomp= 1;
	    }

	    /* Read the list of source file names that went into this */
	    old_sz= filelist_size;
	    read_file_list(fd);

	    /* Compare the last modification dates to see if source is newer
	     * than the binary and a recompile is needed.
	     */

	    if (idict(VAR_AUTO_RECOMPILE) > 0)
	    {
		/* Check the config.bt file */
		fstat(fd, &bst);
		if (stat("config.bt",&st))
			die("cannot stat config");
		recomp|= (st.st_mtime > bst.st_mtime);

		/* Check the filelist files -- if they are missing we assume
		 * the binary is current.
		 */
		if (idict(VAR_AUTO_RECOMPILE) > 1)
		{
		    int j;

		    for (j= old_sz + 1; !recomp && j < filelist_size; j++)
			recomp= (!stat(filelist[j],&st) &&
				 st.st_mtime > bst.st_mtime);
		}
	    }
        }

	if (recomp)
	{
	    if (fd >= 0) close(fd);

	    if (i > 3) die("repeated attempts to recompile %s failed",sfile);
	    if (i > 0) sleep(10);

	    recompile(sfile);
	}
	else
	{
	    /* Allocate memory for top token */
	    head= (Token *)malloc(sizeof(Token));

	    read_token(fd, head);

	    close(fd);

	    return head;
	}
    }
eof:	if (rc < 0)
		die("read error in \"%s\": %s", bfile, strerror(errno));
	else
		die("premature end of file in \"%s\"",bfile);
}


/* RECOMPILE - Execute the backtalk compiler on the named file.  The file
 * name can be either the source or binary name file name.  Dies on failure.
 * Attempts to read whatever error message the compiler prints and print
 * that.
 */

void recompile(char *file)
{
    pid_t cpid, wpid;
    int pip[2];
    waittype status;

    if (pipe(pip))
    	die("Could not create pipe to compiler");

    switch (cpid= fork())
    {
    case 0: /* Child - exec the command */

	close(pip[0]);

	/* Attach pip[1] to child's stderr */
	if (pip[1] != 2)
	{
	    dup2(pip[1],2);
	    close(pip[1]);
	}

	execl(BT_COMPILER, "btc", file, (char *)NULL);
	/* Exec failed */ exit(96);

    case -1: /* Parent - Fork failed */

	close(pip[0]);
	close(pip[1]);

	die("Fork of '"BT_COMPILER" %s' failed",file);

    default: /* Parent - Fork succeeded */
    
    	close(pip[1]);

	/* Wait for child to exit */
	while ((wpid= wait(&status)) != cpid && wpid != -1)
	    ;

	/* Die if we couldn't execute the compiler */
	if (WIFEXITED(status) && WEXITSTATUS(status) == 96)
	    die("unable to execute "BT_COMPILER" command to recompile %s",file);

	/* If compiler found errors in the file, report those */
	if (!WIFEXITED(status) || WEXITSTATUS(status) == 1)
	{
	    char errmsg[BFSZ*4+1];
	    int n;

	    /* Read the error message from our pipe */
	    n= read(pip[0], errmsg, BFSZ*4);
	    errmsg[n]= '\0';
	    if (n > 0)
		die("%s",errmsg);
	    else
	    	die("Unknown error compiling %s",file);
	}

	close(pip[0]);
    }
}
