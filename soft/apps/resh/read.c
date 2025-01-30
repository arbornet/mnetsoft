/*
 * Read a command using readline.
 */

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

char *
resh_read(char *prompt)
{
	return(readline(prompt));
}
