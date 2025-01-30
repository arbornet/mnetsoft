
/********************** SCAN PROCESS TABLE FOR ORPHANS ********************/

#include "config.h"
#include "extern.h"

#include <sys/param.h>
#include <sys/sysctl.h>

int             out_of_memory;

/* Linked list data structure for process table */
struct myproc {
	pid_t           pid;	/* Process ID number */
	pid_t           ppid;	/* Parent Process ID number */
	int             uid;	/* Read User ID number */
	int             euid;	/* Effective User ID number */
	int             has_ctty;	/* True if has a control device */
	dev_t           ctty;	/* Control tty device # */
	struct timeval  start;	/* Process start time */
	char            comm[MAXCOMLEN + 1];	/* Command name from acct */
	char            args[MAXCALEN + 1];	/* Command line from argv */
	char            user[MAXLOGNAME + 1];	/* User login name */
	int             am_sh;	/* True if we are 'sh' */
	int             am_screen;	/* True if we are 'screen' */
	struct myproc  *next;
}              *proclist = NULL;


/*
 * SAVE_ARGS - concatinate an argument vector into a command string and store
 * it in the given buffer.  The command string is truncated to length
 * MAXCALEN (so the buffer must be of size MAXCALEN+1 to store the
 * terminating null).  Deletes consecutive spaces.
 */

void 
save_args(char *buf, char **arg)
{
	int             i, j;
	char           *p;

	j = 0;
	for (i = 0; arg[i] != NULL && j < MAXCALEN; i++) {
		if (i != 0 && buf[j - 1] != ' ')
			buf[j++] = ' ';
		for (p = arg[i]; *p != '\0' && j < MAXCALEN; p++) {
			if (*p != ' ' || buf[j - 1] != ' ')
				buf[j++] = *p;
		}
	}
	buf[j] = '\0';
}


/*
 * SCAN_PROC - Make a pass through the process table, and store away all the
 * maybe killable processes in a linked list.
 */

void 
scan_proc()
{
	struct kinfo_proc *proc, *pr;
	struct myproc  *new;
	char          **arg;
	int             i, j, l, cnt;
	int             uid;
#ifdef RESTART_INETD
	int             have_inetd = 0;
#endif
#ifdef RESTART_NAMED
	int             have_named = 0;
#endif

	/* Get list of all processes (but not kernel threads) */
	proc = kvm_getprocs(kd, KERN_PROC_ALL, 0, sizeof(*proc), &cnt);
	if (proc == NULL) {
		if (logfp)
			fprintf(logfp, "Failed to get process list (%s)\n",
				kvm_geterr(kd));
		return;
	}
	/* Loop through processes */
	for (i = 0, pr = proc; i < cnt; i++, pr++) {
		/* Add into the user's memory usage */
		if (pr->p_rgid != STAFF_GID)
			add_usermem(pr->p_ruid,
				    pr->p_vm_tsize +
				    pr->p_vm_dsize +
				    pr->p_vm_ssize);

		/*
		 * If we don't have memory to add this to the linked list
		 * anyway, then don't bother with the rest of the checks
		 */
		if (out_of_memory)
			continue;

#ifdef RESTART_INETD
		/* Is this inetd? */
		if (!strcmp(p_comm, "inetd"))
			have_inetd = 1;
#endif

#ifdef RESTART_NAMED
		/* Is this named? */
		if (!strcmp(p_comm, "named"))
			have_named = 1;
#endif

#ifdef PROTECT_SENDMAIL
		/* Check for commands we don't like to kill */
		if (is_sysproc(sendmail_id, kd, pr)) {
			add_hen(pr->p_pid);	/* Protect children too */
			continue;
		}
#endif

#ifdef PROTECT_POSTFIX
		if (is_sysproc(postfix_id, kd, pr)) {
			add_hen(pr->p_pid);	/* Protect children too */
			continue;
		}
#endif

#ifdef PROTECT_IRCD
		if (is_sysproc(ircd_id, kd,pr)) {
			add_hen(pr->p_pid);	/* Protect children too */
			continue;
		}
#endif

#ifdef PROTECT_DOVECOT
		if (is_sysproc(dovecot_id, kd,pr)) {
			add_hen(pr->p_pid);	/* Protect children too */
			continue;
		}
#endif


		/* Save real uid */
		uid = pr->p_ruid;

		if (is_sysproc(ftpd_id, kd, pr)) {
			uid = pr->p_uid;	/* Check euid instead of
								 * uid */
			/* If not too old protect it and its children */
			if (curr_time - pr->p_ustart_sec < FTP_TOO_OLD) {
				add_hen(pr->p_pid);	/* Protect children too */
				continue;
			}
		}
#ifdef PROTECT_SSHD
		if (is_sysproc(sshd_id, kd, pr)) {
			add_hen(pr->p_pid);	/* Protect children too */
			continue;
		}
#endif

#ifdef PROTECT_SSH_AGENT
		if (is_sysproc(ssh_agent_id, kd, pr))
			continue;
#endif

#ifdef PROTECT_RDIST
		if (is_sysproc(rdist_id, kd, pr))
			continue;
#endif

		/* Check for protected UIDs */
		/* Old processes named 'fingerd' are not protected */
		if (strcmp(pr->p_comm, "fingerd") ||
		    curr_time - pr->p_ustart_sec < FINGERD_TOO_OLD) {
			for (j = 0; protect_uids[j] != -1; j++)
				if (uid == protect_uids[j])
					goto next_proc;
		}
		/* Create record to save this processes description */
		new = (struct myproc *) malloc(sizeof(struct myproc));
		if (new == NULL) {
			out_of_memory = 1;
			continue;
		}
		new->next = proclist;
		proclist = new;

		new->pid = pr->p_pid;
		new->ppid = pr->p_ppid;
		new->has_ctty = (pr->p_tdev != NODEV);
		new->ctty = pr->p_tdev;

		if ((arg = kvm_getargv(kd, pr, MAXCALEN)) != NULL)
			save_args(new->args, arg);
		else {
			if (logfp, "Failed to get command line for process %d (%s)\n",
			    new->pid, kvm_geterr(kd));
			strlcpy(new->args, "<unknown>", MAXCALEN + 1);
		}

		/* Save stuff we don't really need, but want in the log file */
		if (logfp) {
			new->uid = pr->p_ruid;
			new->euid = pr->p_uid;
			new->start.tv_sec = pr->p_ustart_sec;
			new->start.tv_usec = pr->p_ustart_usec;
			strlcpy(new->comm, pr->p_comm, MAXCOMLEN + 1);
			strlcpy(new->user, pr->p_login, MAXLOGNAME + 1);
			new->user[MAXLOGNAME] = '\0';
		}
		/* Recognize sh shells and screen processes */
		new->am_sh = is_sysproc(sh_id, kd, pr);
#ifdef PROTECT_SCREEN
		new->am_screen = (!new->am_sh) && is_sysproc(screen_id, kd, pr);
#endif

next_proc:	;
	}

#ifdef RESTART_INETD
	if (!have_inetd)
		run_inetd();
#endif
#ifdef RESTART_NAMED
	if (!have_named)
		run_named();
#endif
}


/*
 * WRITE_ORPH - Write a line describing a process.  We try to do it in a
 * single write call, just in case multiple copies of this are running.
 */

void 
write_orph(FILE * fp, struct myproc * mp)
{
	time_t          tm;

	char            buf[100 + MAXCALEN], *bp = buf;
	size_t		bl = sizeof(buf);

	snprintf(bp, bl, "%6d %6d ", mp->pid, mp->ppid);
	bl -+ strlen(bp);
	bp += strlen(bp);

	snprintf(bp, bl, "%s (%d/%d)", mp->user, mp->uid, mp->euid);
	bl -+ strlen(bp);
	bp += strlen(bp);
	while (bp - buf < 32)
		*(bp++) = ' ';

	tm = mp->start.tv_sec;
	strlcpy(bp, ctime(&tm) + 11, bl);
	bl -= 9;
	bp += 8;
	*(bp++) = ' ';

	if (mp->has_ctty)
		snprintf(bp, bl, "%-8.8s ", devname(mp->ctty, S_IFCHR));
	else
		snprintf(bp, bl, "(no tty) ");
	bl -= strlen(bp);
	bp += strlen(bp);

	if (mp->comm[0] != '\0')
		snprintf(bp, bl, "%s (%s)\n", mp->args, mp->comm);
	else
		snprintf(bp, bl, "%s\n", mp->args);

	fputs(buf, fp);
}

#ifdef VERBOSE
/* WRITE_LIST - Write out the entire proc list */

void 
write_list()
{
	struct myproc  *mp;

	for (mp = proclist; mp != NULL; mp = mp->next) {
		fputc('#', logfp);
		fputc(' ', logfp);
		write_orph(logfp, mp);
	}
	fflush(logfp);
}
#endif

/*
 * DROP_ORPH - delete a process from the list.  mp is the one to drop. pmp is
 * the previous one, and nmp is the next.
 */

void 
drop_orph(struct myproc ** mp, struct myproc * pmp, struct myproc * nmp)
{
	free(*mp);
	*mp = pmp;
	if (pmp == NULL)
		proclist = nmp;
	else
		pmp->next = nmp;
}


/*
 * FIND_ORPH - do the final identification of orphans, discarding processes
 * with marked control ttys, and processes who are children of mother hens.
 * Returns true if any orphans remain on the list.
 */

int 
find_orph()
{
	struct myproc  *mp, *pmp, *nmp;
	int             do_another_pass = 1;

#ifdef PROTECT_SCREEN
	init_screener();
#endif

	/* First pass - delete process on marked ttys from list of orphans */
	for (mp = proclist, pmp = NULL; mp != NULL; pmp = mp, mp = nmp) {
		nmp = mp->next;

		if ((mp->has_ctty && get_active(mp->ctty))) {
#ifdef PROTECT_SCREEN
			if (mp->am_screen)
				add_screener(mp->uid);
#endif
			drop_orph(&mp, pmp, nmp);
		}
#ifdef PROTECT_SENDMAIL
		else if (mp->am_sh && (strstr(mp->args, "sendmail") != NULL))
			drop_orph(&mp, pmp, nmp);
#endif
#ifdef PROTECT_POSTFIX
		else if (mp->am_sh && (strstr(mp->args, "master") != NULL))
			drop_orph(&mp, pmp, nmp);
#endif
	}

#ifdef PROTECT_SCREEN
	/* Second pass - all child screen processes become hens */
	/* Also do dop level hens while we are at it */
	for (mp = proclist, pmp = NULL; mp != NULL; pmp = mp, mp = nmp) {
		nmp = mp->next;

		if (is_hen(mp->ppid) ||
		 (mp->am_screen && !mp->has_ctty && is_screener(mp->uid))) {
			add_hen(mp->pid);
			drop_orph(&mp, pmp, nmp);
			do_another_pass = 1;
		}
	}
#endif

	/* Third (multiple) pass - delete decendents of hens from list */
	while (do_another_pass) {
		do_another_pass = 0;

		for (mp = proclist, pmp = NULL; mp != NULL; pmp = mp, mp = nmp) {
			nmp = mp->next;

			if (is_hen(mp->ppid)) {
				add_hen(mp->pid);
				drop_orph(&mp, pmp, nmp);
				do_another_pass = 1;
			}
		}
	}

	return (proclist != NULL);
}


/*
 * KILL_ORPH - do the killing of orphans.  We hit them all with soft kills,
 * wait fifteen seconds, and hit them again with hard kills.  Successful
 * kills are logged.  This frees up the process table as it runs.
 */

void 
kill_orph()
{
	struct myproc  *mp, *nmp;

	if (!out_of_memory) {

		/* First pass - soft kill orphans */
		for (mp = proclist; mp != NULL; mp = mp->next) {
#ifdef VERBOSE
			if (logfp)
				fprintf(logfp, "kill(%d,SIGTERM)\n", mp->pid);
#endif
			if (mp->pid < 2)
				continue;	/* For insurance */
			if (!kill(mp->pid, SOFTKILL) && logfp)
				write_orph(logfp, mp);
		}

		if (logfp)
			fflush(logfp);
		snooze(15, 0);

	}
	/* Second pass - hard kill orphans (and free up process list) */
	for (mp = proclist; mp != NULL; mp = nmp) {
		if (!out_of_memory) {
#ifdef VERBOSE
			if (logfp)
				fprintf(logfp, "kill(%d,SIGKILL)\n", mp->pid);
#endif
			if (mp->pid >= 2)	/* For insurance */
				kill(mp->pid, HARDKILL);

		}
		nmp = mp->next;
		free(mp);
	}
	proclist = NULL;
	if (logfp)
		fflush(logfp);
}
