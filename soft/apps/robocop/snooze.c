#include "config.h"
#include "extern.h"

#include <signal.h>
#include <setjmp.h>

int             awakened;
int		robosocketid = -1;

/*
 * SNOOZE - like sleep(), but uses select() instead.  If we are hit by
 * SIGWAKE, then interupt the sleep.  Returns 0 if we slept to the end,
 * non-zero if we were awakened by a signal.
 */

jmp_buf         jmpenv;

void 
awaken()
{
	signal(SIGWAKE, SIG_IGN);
	awakened++;

	curr_time = time(0);
	if (logfp)
		fprintf(logfp, "%20.20s: robocop %d awakened\n",
			ctime(&curr_time) + 4, mypid);

	longjmp(jmpenv, 1);
}


int 
snooze(time_t secs, int always)
{
	struct timeval  tv;
	fd_set fds, *fdsp;
	time_t then, secsrem;

	if (awakened && !always)
		return awakened;

	then = time(NULL) + secs;
	secsrem = then - time(NULL);
	while (secsrem > 0 && secsrem < 3600) {
		tv.tv_sec = secsrem;
		tv.tv_usec = 0;

		fdsp = &fds;
		if (robosocketid < 0)
			robosocketid = robolisten();
		if (robosocketid < 0)
			fdsp = NULL;
		if (fdsp != NULL) {
			FD_ZERO(fdsp);
			FD_SET(robosocketid, fdsp);
		}
		if (setjmp(jmpenv) == 0) {
			signal(SIGWAKE, awaken);
			select(robosocketid + 1, fdsp, NULL, NULL, &tv);
			signal(SIGWAKE, SIG_IGN);
		}
		if (fdsp != 0 && FD_ISSET(robosocketid, fdsp)) {
			FD_CLR(robosocketid, fdsp);
			signal(SIGWAKE, awaken);
			roborje(robosocketid);
			signal(SIGWAKE, SIG_IGN);
		}
		secsrem = then - time(NULL);
	}

	return awakened;
}
