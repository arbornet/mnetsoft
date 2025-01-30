#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <pwd.h>
#include <kvm.h>
#include <utmp.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>

#ifdef DEBUG
#define killpid(p) printf("Killing %d\n",p)
#else
#define killpid(p) kill(p,SIGKILL)
#endif

#define T_UID		0
#define T_TTY		1


struct targ {
	int type;		/* What kind of thing:  T_UID or T_TTY */
	union {
		uid_t uid;	/* user uid number if type == T_UID */
		dev_t tty;	/* tty device number if type == T_TTY */
	} u;
} *target;

kvm_t *kd;


/*
 * KILL_TARG - Make a pass through the process table, killing all processes
 * owned by the tty or user pointed to by the given target (but don't kill
 * ourselves).  Returns the count of processes killed.
 */

int
kill_targ(struct targ * tg)
{
	struct kinfo_proc *proc, *pr;
	pid_t my_pid = getpid();
	int cnt, i, nkill = 0;

	if (tg->type == T_UID) {
		/* Get list of processes with real uid matching target uid */
		proc = kvm_getprocs(kd, KERN_PROC_RUID, tg->u.uid, &cnt);
		if (proc == NULL) {
			fprintf(stderr, "Failed to get process list for uid %d (%s)\n",
				tg->u.uid, kvm_geterr(kd));
			exit(1);
		}
	} else {
		/*
		 * Get list of processes with control terminal matching
		 * device tty
		 */
		proc = kvm_getprocs(kd, KERN_PROC_TTY, tg->u.tty, &cnt);
		if (proc == NULL) {
			fprintf(stderr, "Failed to get process list for tty %d,d (%s)\n",
			major(tg->u.tty), minor(tg->u.tty), kvm_geterr(kd));
			exit(1);
		}
	}

	/* Loop through the processes */
	for (i = 0, pr = proc; i < cnt; i++, pr++) {
		if (pr->kp_proc.p_pid != my_pid && pr->kp_proc.p_pid > 1) {
			killpid(pr->kp_proc.p_pid);
			nkill++;
		}
	}
	return (nkill);
}


/*
 * DEV_NO - Find the device number of the given the tty name like "ttyXX" or
 * "console".  Return non-zero on failure.
 */

int
dev_no(char *tty, dev_t * dev)
{
	char devpath[UT_LINESIZE + 10];
	struct stat st;

	if (strlen(tty) == 2)
		sprintf(devpath, "/dev/tty%s", tty);
	else
		sprintf(devpath, "/dev/%.*s", UT_LINESIZE, tty);

	if (stat(devpath, &st) || !S_ISCHR(st.st_mode))
		return (1);

	*dev = st.st_rdev;
	return (0);
}


/*
 * SNOOZE - like sleep(), but uses select() instead.
 */

snooze(int secs)
{
	struct timeval tv;

	tv.tv_sec = secs;
	tv.tv_usec = 0;

	select(0, NULL, NULL, NULL, &tv);
}

main(int argc, char **argv)
{
	int i, j, ntarget, n;
	struct passwd *pw;
	char *ttyname;
	int repeat = 0, uid;
	char errbuf[_POSIX2_LINE_MAX + 1];

	target = (struct targ *) malloc(argc * sizeof(struct targ));
	for (i = 1, ntarget = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			for (j = 1; argv[i][j] != '\0'; j++)
				switch (argv[i][j]) {
				case 'r':
					repeat = 1;
					break;
				case 't':
					/* TTY name - get device number */
					if (argv[i][j + 1] != '\0')
						ttyname = argv[i] + j + 1;
					else if (i + 1 < argc)
						ttyname = argv[++i];
					else
						goto usage;

					target[ntarget].type = T_TTY;
					if (dev_no(ttyname, &target[ntarget].u.tty))
						fprintf(stderr, "Bad tty name: %s\n", ttyname);
					else
						ntarget++;

					goto nextarg;

				default:
					goto usage;
				}
		} else if (isascii(argv[i][0]) && isdigit(argv[i][0])) {
			/* UID number - save it */
			target[ntarget].type = T_UID;
			target[ntarget++].u.uid = atoi(argv[i]);
		} else {
			/* User name - look it up */
			if ((pw = getpwnam(argv[i])) == NULL)
				fprintf(stderr, "Bad user name: %s\n", argv[i]);
			else {
				target[ntarget].type = T_UID;
				target[ntarget++].u.uid = pw->pw_uid;
			}
		}
nextarg:	;
	}

	if (ntarget == 0)
		goto usage;

	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf)) == NULL) {
		fprintf(stderr, "FAIL: cannot open process table\n");
		exit(1);
	}
	/* Abandon root status if this is a setuid-root program */
	setuid(uid = getuid());

	for (j = 1; j <= 12; j++) {
		if (repeat)
			fprintf(stderr, "Pass %d...\n", j);

		/* Raise our CPU priority */
		if (uid != 0)
			nice(-20);

		/* Kill the processes of each effected user, in turn */
		n = 0;
		for (i = 0; i < ntarget; i++)
			n += kill_targ(target + i);

		if (!repeat || n == 0)
			break;

		/*
		 * Restore our CPU priority to normal and sleep bit while the
		 * last wave of kills takes effect.
		 */
		if (uid == 0)
			nice(20);
		snooze(2);
	}

	if (repeat)
		fprintf(stderr, "Done\n");
	kvm_close(kd);

	exit(0);

usage:
	fprintf(stderr, "usage: %s [-r] <login>|<uid>|-t <ttyXX>...\n", argv[0]);
	exit(1);
}
