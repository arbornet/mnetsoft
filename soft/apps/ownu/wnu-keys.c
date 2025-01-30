#include <stdio.h>
#include <termios.h>
#include "nu.h"
#include "wnu.h"

/* VERSION HISTORY:
 * 12/14/95 - last version before I started collecting version history.
 * 06/24/97 - Added version history. [jdw]
 */

#define cntl(c) ((c)-'@')

#define MERASE 0
#define MINTR  1
#define MKILL  2
#define MEOF   3
#define NKEYS  4

char keyval[NKEYS]= { cntl('H'), cntl('C'), cntl('X'), cntl('D') };
char *kname[NKEYS]= { "erase", "interupt", "kill", "end-of-file" };


/* RESERVED_KEY - Check if a control-key is reserved.  If so, return an
 * explanation completing the sentance "This key is reserved for..."
 *
 * This is a stripped down version of Marcus's chk_reserved() function.
 */

char *reserved_key(char c, int key)
{
	switch(c)
	{
	case '\r':	return "the return character";
	case '\n':	return "the newline character";
	case '\t':	return "the tab character";
	case cntl('Z'):	return "job control (suspend)";
	case cntl('Y'):	return "job control (delayed suspend)";
	case cntl('W'):	return "word erase";
	case cntl('R'):	return "text redisplay";
	case cntl('O'):	return "flushing output";
	case cntl('V'):	return "the literal next character";
	case cntl('Q'):	return "flow control (start)";
	case cntl('S'):	return "flow control (stop)";
	case cntl('['):	return "the VI mode delimiter / function key input";
	case cntl('\\'):return "the quit character";
	case cntl('D'):	if (key == MEOF) break;
			return "the eof character";
	}
	return 0;
}


/* CHECK_KEY - Generic routine to interpret a special key and do some
 * generic tests on it.  Returns non-zero on error.  This is more idiot-
 * proof than it needs to be, since it should be impossible to get a bad
 * character off the form and and the only harm a person does by bypassing
 * the form to get a different character is to himself.  We call any
 * errors here hostile, since they can only come from either a user not using
 * our form, or an error in our form and we want to know about either case.
 */

int check_key(char *val, int key)
{
char *res;

    /* We don't complain if he didn't set the keys.  Just default them. */
    if (val[0] == '\0')
	return;

    if (val[0] == '^' && isascii(val[1]) && val[2] == '\0' &&
	(isalpha(val[1]) || val[1] == '?'))
    {
	if (val[1] == '?')
	    keyval[key]= '\177';
	else
	    keyval[key]= val[1]- ((isupper(val[1])) ? '@' : '`');
    }
    else if (val[1] != '\0' || !isascii(val[0]))
    {
	err(USER|HOSTILE,"Unrecognizable %s-key value (<KBD>%#S</KBD>).\n"
	    "Use <KBD>^C</KBD> notation for control-characters.\n",
	    kname[key],val);
	return;
    }
    else if (isprint(val[0]))
    {
	err(USER|HOSTILE,"Your %s-key should be a control-character.\n",
	    kname[key]);
	return;
    }
    else
	keyval[key]= val[0];

    if ((res= reserved_key(keyval[key],key)) != NULL)
	err(USER|HOSTILE,"You have selected an illegal %s-key value.\n"
	    "The %#C character is reserved for %s.\n",
	    kname[key], keyval[key], res);
}


char *check_backspace(char *val) { check_key(val,MERASE); return val; }
char *check_interupt(char *val)  { check_key(val,MINTR); return val; }
char *check_kill(char *val)      { check_key(val,MKILL); return val; }
char *check_eof(char *val)       { check_key(val,MEOF); return val; }


/* FINAL_KEY_CHECK - Check for duplicate keys and save the key values for
 * Marcus.
 */

void final_key_check()
{
int i,j;

    for (i= 0; i < NKEYS-1; i++)
	for (j= i+1; j < NKEYS; j++)
	    if (keyval[i] == keyval[j])
	    {
		err(USER,"You have set both your %s key and your %s key to "
		    "<KBD>%#C</KBD>.  Set one to something else.\n",
		    kname[i], kname[j], keyval[i]);
		return;
	    }

    SERASE(&norm)= keyval[MERASE];
    SINTR(&norm)=  keyval[MINTR];
    SKILL(&norm)=  keyval[MKILL];
    SEOF(&norm)=   keyval[MEOF];
}
