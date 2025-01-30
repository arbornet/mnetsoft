/* Copyright 2003, Jan D. Wolter, All Rights Reserved. */

#include "common.h"
#include "sysdict.h"
#include "waittype.h"
#include "email.h"
#include "str.h"

#ifdef SENDMAIL_PATH

/* EMAIL_RECIP_COUNT: Count total number of recipients in "To", "Cc" and "Bcc"
 * lists.
 */

int email_recip_count(Email *email)
{
    int i, n= 0;
    struct emaddr *r;

    for (i= 0; i < N_REC; i++)
	for (r= email->to[i]; r != NULL; r= r->next)
	    n++;

    return n;
}


/* EMAIL_RECIP_SAVE: Save all recipient addresses to the given array.  Array
 * must be big enough.  We do not save email addresses that start with a -,
 * as sendmail might mistake these for options.
 */

void email_recip_save(Email *email, char **b)
{
    int i, j= 0;
    struct emaddr *r;

    for (i= 0; i < N_REC; i++)
	for (r= email->to[i]; r != NULL; r= r->next)
	    if (r->addr[0] != '-')
		b[j++]= r->addr;

    b[j]= NULL;
}


/* EMAIL_SEND - Send an email.  Returns 0 on success, miscellaneous non-
 * zero values otherwise.
 */

int email_send(Email *email)
{
    pid_t child_pid, wait_pid;
    FILE *fp;
    waittype wrc;
    static char sendmail_path[]= SENDMAIL_PATH;
    int pip[2];
    char **arg;
    int i;

    if (pipe(pip))
	return 3;

    /* Make a few attempts at forking - should never iterate unless system
     * is seriously short of memory.
     */
    for (i= 0; i < 5; i++)
    {
	if ((child_pid = fork()) >= 0) break;
	sleep(5);
    }

    switch(child_pid)
    {
    case 0: /* Child - exec sendmail */

	close(pip[1]);

	/* Attach pip[0] to child's stdin */
	if (pip[0] != 0)
	{
	    dup2(pip[0],0);
	    close(pip[0]);
	}

	/* Construct sendmail argument array.  Doing "sendmail -t" would be
	 * nice, but there are many pseudo-sendmail wrappers for other mailers
	 * that implement very few of the actual sendmail options.  I think
	 * the "-f" and "-F" options usually exist, but not much else can be
	 * counted on.
	 */
	arg= (char **)malloc(email_recip_count(email) + 7);
	if ((arg[0]= strrchr(sendmail_path,'/')) != NULL)
	    arg[0]++;
	else
	    arg[0]= sendmail_path;

	/* A redundant measure against options being slipped into email
	 * addresses.  Ignored by some really old sendmails, so we also
	 * have a check in email_recip_save().
	 */
	arg[1]= "--";

	email_recip_save(email, arg + 2);

	execv(sendmail_path, arg);
	/* exec failed */
	exit(-1);

    case -1: /* Parent - fork failed */

	close(pip[0]);
	close(pip[1]);
	return 2;
    
    default: /* Parent - Fork succeeded */

	close(pip[0]);
	if ((fp= fdopen(pip[1],"w")) == NULL)
	    return 4;

	/* Send the message */
	email_write(email, EFG_DOTSPACE, fp);
	fputs(".\n",fp);
	fclose(fp);

	do {
	    if ((wait_pid= wait(&wrc)) == -1)
		return 1;
	} while (wait_pid != child_pid);

	/* If child was killed ... */
	if (!WIFEXITED(wrc))
	    return 5;

	/* Child exited cleanly */
	return 0;
    }
}
#endif /* SENDMAIL_PATH */
