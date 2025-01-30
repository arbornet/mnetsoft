#include "config.h"
#include "extern.h"

#include <sys/param.h>
#include <sys/sysctl.h>

/*
 * KILLUSER - murder a user who has violated the memory or process limits.
 * This is designed to kill fork-bombs and such, so it does multiple passes
 * through the process-table, using only hard kills.
 */

void 
killuser(kvm_t * kd, uid_t target_uid, FILE * logfp)
{
	struct kinfo_proc *proc, *pr;
	char          **arg;
	int             pass, cnt, i;
	int             nkill, cleanpass = 0;
	int             amnice = !nice(-10);

	/* Don't kill system processes. */
	if (target_uid < MIN_KILL_UID) {
		if (logfp)
			fprintf(logfp, "Attempt to kill processes for system uid %d\n",
				target_uid);
		return;
	}
	/* Do up to thirty passes through the process table */
	for (pass = 0; pass < MAX_KILL_PASS; pass++) {
		nkill = 0;

		/* Get list of processes with real uid matching target uid */
		proc = kvm_getprocs(kd, KERN_PROC_RUID, target_uid, sizeof (*proc), &cnt);
		if (proc == NULL) {
			if (logfp)
				fprintf(logfp, "Failed to get process list for uid %d "
					"while killing (%s)\n", target_uid, kvm_geterr(kd));
			return;
		}
		/* Loop through processes */
		for (i = 0, pr = proc; i < cnt; i++, pr++) {
			/*
			 * Just for extra insurance that we don't kill
			 * process 1
			 */
			if ((pid_t) pr->p_pid < 2)
				continue;

			/* Don't kill processes that are already dead */
			if (P_ZOMBIE(pr))
				continue;

			nkill++;

			/* Log the process (on first pass only) */
			if (pass == 0 && logfp != NULL)
				logproc(pr, logfp);

#ifndef MDEBUG
			kill(pr->p_pid, HARDKILL);
#ifdef VERBOSE
			if (logfp)
				fprintf(logfp, "kill(%d,SIGKILL)\n", pr->p_pid);
#endif
#endif
		}

#ifdef MDEBUG
		/* If we are debugging, don't make multiple passes */
		break;
#else
		/*
		 * Stop making passes if we found nothing to kill for three
		 * consecutive passes
		 */
		if (nkill > 0)
			cleanpass = 0;
		else if (++cleanpass == 3)
			break;
#endif

		/*
		 * Every three passes, we restore our CPU priority to normal
		 * and sleep a bit while the last waves of kills take effect.
		 */
		if (pass % 3 == 2) {
			if (amnice)
				nice(10);
			snooze(1, 1);
			amnice = !nice(-10);
		}
	}

	if (logfp) {
		fprintf(logfp, "user kill complete after %d passes.\n", pass + 1);
		fflush(logfp);
	}
	/* Restore to original priority */
	if (amnice)
		nice(10);
}
