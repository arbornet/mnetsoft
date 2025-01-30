/************************* ROBOCOP MAIN PROGRAM ***************************/
/* (c) 2004 - Jan Wolter - Open Source - GNU License             */

#include "config.h"
#include "extern.h"

#include <signal.h>
#include <fcntl.h>
#include <limits.h>

/* Global variables */
time_t          curr_time;
kvm_t          *kd;
int             mypid;
FILE           *logfp;
extern long     kperpage, pagesize;

void 
sigterm()
{
	curr_time = time(0);

	unlink(PIDFILE);
	if (logfp)
		fprintf(logfp, "%20.20s: robocop %d killed\n",
			ctime(&curr_time) + 4, mypid);
	exit(1);
}


int 
main(int argc, char **argv)
{
	time_t          sleep_time;
	char            errbuf[_POSIX2_LINE_MAX + 1];

	/* First, become a daemon. */
	daemon(0, 0);

	/* Initialization */
	mypid = getpid();
	curr_time = time(0);

	/* Save pid */
	umask(022);
	if ((logfp = fopen(PIDFILE, "w")) != NULL) {
		fprintf(logfp, "%d\n", mypid);
		fclose(logfp);
	}
	signal(SIGTERM, sigterm);

	/* Open log file - if it doesn't exist, run without logging */
	logfp = fopen(LOGFILE, "a");
	if (logfp) {
		fprintf(logfp, "%20.20s: robocop %d starting up\n",
			ctime(&curr_time) + 4, mypid);
		fprintf(logfp, " "
#ifdef DEBUG
			"debug "
#endif
#ifdef MDEBUG
			"memhog_debug "
#endif
#ifdef PROTECT_SSHD
			"protect_sshd "
#endif
#ifdef PROTECT_SCREEN
			"protect_screen "
#endif
#ifdef PROTECT_SSH_AGENT
			"protect_ssh_agent "
#endif
#ifdef PROTECT_POSTFIX
			"protect_postfix "
#endif
#ifdef PROTECT_SENDMAIL
			"protect_sendmail "
#endif
#ifdef PROTECT_RDIST
			"protect_rdist "
#endif
#ifdef PROTECT_IRCD
			"protect_ircd "
#endif
#ifdef PROTECT_DOVECOT
			"protect_dovecot "
#endif
			"interval=%dsec log_memhog=%dM kill_memhog=%dM "
			"log_nproc=%d kill_nproc=%d\n",
		   INTERVAL, LOG_MEMORY / MEGABYTES, MAX_MEMORY / MEGABYTES,
			LOG_NPROC, MAX_NPROC);
	}
	getrobokey();
	kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
	if (kd == NULL) {
		if (logfp)
			fprintf(logfp,
			     "Cannot open kernel (%s) - exiting\n", errbuf);
		fprintf(stderr,
			"Cannot open kernel (%s) - exiting\n", errbuf);
		exit(1);
	}
	/* Get page size */
	pagesize = sysconf(_SC_PAGESIZE);
	kperpage = pagesize / 1024;	/* this would be bad if pagesize were
					 * < 1K */

	/* Init terminal hash */
	init_ttynumber();

	/* Fingerprint all our favorite system processes */
	init_sysproc();

	/* Main loop: work a little, sleep a little */
	for (;;) {
		awakened = 0;
		out_of_memory = 0;
		curr_time = time(0);
		init_usermem();
		init_hen();

		/* Log the start-up time */
		if (logfp) {
			fprintf(logfp, "%20.20s: scanning\n", ctime(&curr_time) + 4);
			fflush(logfp);
		}
		/* We always protect processes on the console */
		init_active();
		set_active(makedev(0, 0));	/* console */

		/*
		 * Note:  Though it might be more efficient to read the utmp
		 * file first, that could result in killing processes of
		 * users who logged in between the two scans, so better read
		 * the process-table first.
		 */

		/* Scan process table */
		scan_proc();

		/* Look for memory hogs and kill them off */
		check_mem(kd);

		/* Scan utmp file */
		scan_utmp();

#ifdef VERBOSE
		if (logfp)
			write_list();
#endif

		if (find_orph()) {
			if (logfp) {
				fprintf(logfp, "killing orphans...\n");
				fflush(logfp);
			}
#ifndef DEBUG
			/*
			 * A lot of the "orphans" are actually in the process
			 * of exiting immediately after a logoff.  We wait a
			 * bit to give them a bit more time to exit
			 * naturally.
			 */
			snooze(3 * MINUTES, 0);
#endif
			/* Kill Orphans */
			kill_orph();
		}
		/* Pass complete - sleep until next pass */
		sleep_time = curr_time + INTERVAL - time(0);
		if (sleep_time < 0)
			sleep_time = 1 * MINUTES;
		snooze(sleep_time, 0);

		/* Close and reopen the logfile, in case it has been rolled */
		if (logfp)
			fclose(logfp);
		logfp = fopen(LOGFILE, "a");
	}

	if (logfp)
		fclose(logfp);
	kvm_close(kd);

	exit(0);
}
