/*
 * This forks off a child process to run inetd.  It first empties the
 * waitlist file.  Or not.
 */

#ifdef RESTART_INETD
void 
run_inetd()
{
	pid_t           child, wpid;
	time_t          now = time(0);

	/* Empty waitlist file before restarting inetd */
	/* truncate(PATH_WAITLIST, (off_t)0); */

	if (logfp) {
		fprintf(logfp, "%20.20s: restarting inetd\n", ctime(&now) + 4);
		fflush(logfp);
	}
	switch (child = fork()) {
	case 0:
		/* Child process - Run inetd */
		setpriority(PRIO_PROCESS, 0, 0);	/* ...at normal priority */
		execl(PATH_INETD, "inetd", "-r", "60", "60", (char *) NULL);
		if (logfp)
			fprintf(logfp, "could not exec " PATH_INETD "\n");

	case -1:
		/* Fork failed */
		fprintf(stderr, "inetdd fork failed\n");
		if (logfp)
			fprintf(logfp, "could not fork to run inetd\n");

	default:
		/* Parent process - do a wait */
		/*
		 * Note that inetd immediately forks a child process, which
		 * becomes the new inetd process, and then exits, so this
		 * won't be a long wait.  We only do it so that we won't
		 * leave zombies.
		 */
		while ((wpid = wait((int *) 0)) != child && wpid != -1);
	}
}
#endif


/*
 * This forks off a child process to run named, attempting to preserve any
 * core dump.
 */

#ifdef RESTART_NAMED
void 
run_named()
{
#define NAMED_CORE_PATH "/usr/local/etc/domain/core"

	pid_t           child, wpid;
	time_t          now = time(0);
	FILE           *fp;

	if (logfp) {
		fprintf(logfp, "%20.20s: restarting named\n", ctime(&now) + 4);
		fflush(logfp);
	}
	/* If a core file exists, renamed it */
	if (!access(NAMED_CORE_PATH, R_OK)) {
		/* Get pid number to use as suffix */
		pid_t           opid = 0;
		static pid_t    cpid = 0;
		char            newname[1024];
		if ((fp = fopen("/var/run/named.pid", "r")) != NULL) {
			if (fscanf(fp, "%d", &opid) > 0) {
				/* kill(oid,HARDKILL); */
			}
			fclose(fp);
		}
		if (opid == 0)
			opid = getpid() + (cpid++);
		snprintf(newname, sizeof(newname), NAMED_CORE_PATH ".%d", opid);

		/* Move the file */
		if (!link(NAMED_CORE_PATH, newname))
			unlink(NAMED_CORE_PATH);
	}
	switch (child = fork()) {
	case 0:
		/* Child process - Run inetd */
		setpriority(PRIO_PROCESS, 0, 0);	/* ...at normal priority */
		execl(PATH_NAMED, "named", "-b", "/etc/named.conf", (char *) NULL);
		if (logfp)
			fprintf(logfp, "could not exec " PATH_NAMED "\n");

	case -1:
		/* Fork failed */
		fprintf(stderr, "inetdd fork failed\n");
		if (logfp)
			fprintf(logfp, "could not fork to run inetd\n");

	default:
		/* Parent process - do a wait */
		/*
		 * Note that inetd immediately forks a child process, which
		 * becomes the new inetd process, and then exits, so this
		 * won't be a long wait.  We only do it so that we won't
		 * leave zombies.
		 */
		while ((wpid = wait((int *) 0)) != child && wpid != -1);
	}
}
#endif
