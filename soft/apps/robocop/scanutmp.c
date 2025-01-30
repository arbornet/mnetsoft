/**************************** SCAN UTMP FILE *****************************/

/*
 * GETDEVNUM - Return the device number for a "ttyXX" or "console" style
 * device name.  Device name does not have to be null terminated, so stuff
 * from the utmp file can be passwd right in.  Dies on failure.
 */

#include "config.h"
#include "extern.h"

#include <utmp.h>

/*
 * SCAN_UTMP - Zip through the utmp file, marking all logged in terminals.
 */

void 
scan_utmp()
{
	struct utmp     ut;
	int             i, fd;

	if ((fd = open(UTMP, 0)) < 0) {
		if (logfp)
			fprintf(logfp, "FAIL: cannot open %s\n", UTMP);
		exit(1);
	}
	while (read(fd, &ut, sizeof(struct utmp)) == sizeof(struct utmp)) {
		if (ut.ut_name[0] == '\0')
			continue;
#ifdef VERBOSE
		if (logfp)
			fprintf(logfp, "UTMP %-*.*s %-*.*s %20.20s %.16s\n",
				UT_NAMESIZE, UT_NAMESIZE, ut.ut_name,
				UT_LINESIZE, UT_LINESIZE, ut.ut_line,
				ctime(&ut.ut_time) + 4,
				ut.ut_host);
#endif

		/* Someone is logged in here -- mark the tty */
		/*
		 * FTP makes utmp entries with values like "ftp23223". Ignore
		 * those.
		 */
		if (!strncmp(ut.ut_line, "tty", 3) || !strcmp(ut.ut_line, "console"))
			set_active(getdevnum(ut.ut_line));

#if 0				/* This only works if screen is putting
				 * parent ttys in utmp */

		/* If this is a screen session, mark the parent tty */
		if (ut.ut_host[0] == ':') {
			for (i = 0; i < 16 && ut.ut_host[i] != '\0'; i++)
				if (ut.ut_host[i] == ':') {
					ut.ut_host[i] = '\0';
					set_active(getdevnum(ut.ut_host));
				}
		}
#endif

	}

	close(fd);
#ifdef VERBOSE
	if (logfp)
		fflush(logfp);
#endif
}
