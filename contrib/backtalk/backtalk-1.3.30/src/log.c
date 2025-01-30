/* (c) 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "backtalk.h"
#include "stack.h"
#include "dict.h"
#include "lock.h"


/* MACRO_LOG:  Runs a Backtalk macro to generate something to append to a log
 * file.  Pass in a filename, and a macroname.  If you have arguments that
 * need to be pushed on the stack for the macro to use, push them before
 * calling this and give the number of input args as nstack.  If create
 * is true, we automatically create the file if it doesn't exist.  Otherwise,
 * nothing is done if the file doesn't exist.
 */

void macro_log(char *filename, char *macro_name, int nstack, int create)
{
    Token *macro;
    char *line;
    int fd,i;

    /* First see if the macro exists */
    macro= get_dict(macro_name);
    if (!macro ||
	type(*macro) != TK_PROCEDURE ||
	(fd= open(filename,O_WRONLY|O_APPEND|(create ? O_CREAT : 0))) < 0)
    {
	/* Could not run - discard the arguments */
	for (i= 0; i < nstack; i++)
	    func_pop();
	return;
    }
    lock_exclusive(fd,macro_name);

    /* Run the macro */
    uniquify_token(macro); /* rerun() consumes it's token so uniquify first */
    rerun(aval(*macro));

    /* get the result */
    line= pop_string();

    /* Append to the file */
    if (line[0] != '\0')
	write(fd,line,strlen(line));

    close(fd);
    unlock(macro_name);
    free(line);
}
