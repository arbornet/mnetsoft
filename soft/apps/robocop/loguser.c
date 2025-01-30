#include <sys/param.h>
#include <sys/sysctl.h>

#include "config.h"
#include "extern.h"

/*
 * LOGUSER - List all of a user's processes to the log file.
 */

void 
loguser(kvm_t * kd, uid_t target_uid, FILE * logfp)
{
	struct kinfo_proc *proc, *pr;
	int             i, cnt;

	if (!logfp)
		return;

	/* Get list of processes with real uid matching target uid */
	proc = kvm_getprocs(kd, KERN_PROC_RUID, target_uid, sizeof(*proc), &cnt);

	if (proc == NULL) {
		if (logfp)
			fprintf(logfp, "Failed to get process list for uid %s "
			"while logging (%s)\n", target_uid, kvm_geterr(kd));
		return;
	}
	/* Loop through processes */
	for (i = 0, pr = proc; i < cnt; i++, pr++)
		logproc(pr, logfp);
	fflush(logfp);
}


/*
 * LOGPROC - Write a line describing the process pointed to by the given
 * pointer.
 */

void 
logproc(struct kinfo_proc * pr, FILE * logfp)
{
	char          **arg;

	fprintf(logfp, " PID=%d UID=%d(%s) MEM=%ldK (%.*s) ",
		pr->p_pid,
		pr->p_ruid,
		pr->p_login,
		(long) (pr->p_vm_tsize +
			pr->p_vm_dsize +
			pr->p_vm_ssize) * kperpage,
		KI_MAXCOMLEN, pr->p_comm);

	if ((arg = kvm_getargv(kd, pr, 60)) == NULL) {
		if (logfp)
			fprintf(logfp,
				"Logproc failed to get command line for process %d (%s)\n",
				pr->p_pid, kvm_geterr(kd));
	} else {
		int             j;
		for (j = 0; arg[j] != NULL; j++) {
			if (j != 0)
				fputc(' ', logfp);
			fputs(arg[j], logfp);
		}
	}
	fputc('\n', logfp);
}
