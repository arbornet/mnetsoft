#include "gate.h"

int cant_tab= 0;	/* If non-zero, assume terminal doesn't tab */
int dont_tab= 0;	/* Terminal can't tab, or tab stops being offset */

term_mode cooked, cbreak;

#ifdef HAVE_SIGACTION
/* SIGACT:  A slightly more convenient interface to sigaction().
 */

int sigact(int signum, void(*handler)(), struct sigaction *old)
{
    struct sigaction sa;
    sa.sa_handler= handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags= 0;
    return sigaction(signum, &sa, old);
}
#endif


/* INITMODES: Set up the cooked and cbreak modes for future use.  Note that
 * this doesn't actually change the modes.
 */

void initmodes()
{
    /* Get ioctl modes */
    errno= 0;
    GTTY(0,&cooked);
#ifdef F_STTY
    ioctl(0, TIOCGETC, &tch);
#ifdef TIOCGLTC
    ioctl(0, TIOCGLTC, &ltch);
#endif
#endif /*F_STTY*/
    if (errno != 0)
    {
	fprintf(stderr,"%s: Input not a tty\n",progname);
	exit(RET_ABORT);
    }
    cbreak= cooked;		/* Structure assignment works? */
#if defined(F_TERMIO) || defined(F_TERMIOS)
#ifdef F_TERMIOS
    ospeed= cfgetospeed(&cooked);
#else
    ospeed= cooked.c_ospeed;
#endif
    cbreak.c_lflag&= ~(ECHO | ICANON);
    cbreak.c_cc[VMIN]= 1;
    cbreak.c_cc[VTIME]= 0;
    cant_tab= cooked.c_oflag & XTABS;
    cbreak.c_oflag&= ~XTABS;
#ifdef OCRNL
    cbreak.c_oflag&= ~OCRNL;
#endif /*OCRNL*/
#endif /*F_TERMIO or F_TERMIOS*/
#ifdef F_STTY
    ospeed= cooked.sg_ospeed;
    cbreak.sg_flags &= ~ECHO;
    cbreak.sg_flags |= CBREAK;
    cant_tab= cooked.sg_flags & XTABS;
    cbreak.sg_flags&= ~XTABS;
#endif /*F_STTY*/
    dont_tab= cant_tab || (base_col != 0);

}


/* SETMODE - toggles between cbreak and cooked mode.  Given a 1, it sets
 * cbreak, given a 0 it sets cooked.
 */
void set_mode(int into_cbreak)
{
    in_cbreak= into_cbreak;
    if (into_cbreak)
	STTY(0,&cbreak);
    else
	STTY(0,&cooked);
}


/* INTR - Interupt Driver.  Ask if we really want to discard this.  If no
 * resume cleanly.
 */

RETSIGTYPE intr()
{
    term_mode old;

    if (!subtask)
    {
#ifdef HAVE_SIGACTION
	sigact(SIGINT, SIG_IGN, NULL);
#else
	signal(SIGINT,SIG_IGN);
#endif
	GTTY(0,&old);
	set_mode(1);
	putchar('\n');
	if (readyes("OK to abort response? "))
	    done(RET_ABORT);
	STTY(0,&old);
#ifdef HAVE_SIGACTION
	sigact(SIGINT, intr, NULL);
#else
	signal(SIGINT,(void (*)())intr);
#endif
	resume= 1;
    }
    if (use_jump) longjmp(jenv,1);
}


/* HUP - on hangup, just quit.  Might just as well leave this untrapped. */

RETSIGTYPE hup()
{
    done(RET_ABORT);
}


/* DONE - clean up tty modes before exiting, and do work-arounds for Yapp's
 * habit of ignoring PicoSpan's strange return codes
 *
 * NOTES ON COMMUNICATIONS WITH PICOSPAN AND YAPP:
 *
 * There are four basic things PicoSpan or Yapp might do after we exit:
 *   ENTER -- Silently enter the response.
 *   ABORT -- Don't enter the response.
 *   ASKOK -- Ask the user if he wants to ENTER or ABORT.
 *   ERROR -- Print various error messages and ABORT.
 *
 * In PicoSpan the behavior is selected by the return code.  We have the
 * return codes RET_ENTER, RET_ABORT, RET_ASKOK for the first three, and
 * anything else for the last.
 *
 * Early versions of Yapp always did an ENTER unless the buffer file didn't
 * exist, in which case it did an ERROR.  This was inadequate to support
 * programs like gate and red.  Dave later fixed this so that if the buffer
 * doesn't exist, it does an ABORT.
 *
 * The folks at River had the earlier version of Yapp, and they modified it
 * so that it always does an ASKOK.  This sucks.
 *
 * Caucus does something like one or the other broken Yapp versions. I forget.
 *
 * The done() routine takes a PicoSpan-style return code and does the
 * appropriate wrap-up.
 *
 * Gate's basic idea here is to always ask the conferencing system to either
 * ENTER or ABORT.  We fake ASKOK by asking the question ourselves.  This
 * lets us give the user the option of continuing text entry.
 *
 * WARNING:  If done() is called with RET_ENTER or RET_ASKOK it is possible
 * for the done() call to return to the calling program, in the event that
 * the user decides he isn't done after all.  When called with RET_ABORT it
 * can be relied on never to return.
 */

void done(int rc)
{
    int didcheck= 0;
    int ch;

    if (rc == RET_ENTER && askok)
	rc= RET_ASKOK;

#ifndef NO_SPELL
    if (rc != RET_ABORT)
    {
	if (spellcheck == 1)
	    printf("Checking Spelling...\n");
	if (spellcheck == 1 ||
	    (spellcheck == 2 && askquest("Check spelling? ","ny") == 'y'))
	{
	    cmd_spell("");
	    didcheck= 1;
	}
    }
#endif

#ifndef RIVER
    /* Fake the ASKOK thing, but with a con */
    if (rc == RET_ASKOK || didcheck)
    {
	while((ch= askquest("Ok to enter this response? [ync?] ","nyc?h>"))
		== '?' && ch != 'h')
	{
	    printf("  y - yes: enter this response as is.\n");
	    printf("  n - no: discard this response.\n");
	    printf("  c - continue: return to > prompt.\n");
	}
	switch(ch)
	{
	case 'y':
	    rc= RET_ENTER;
	    break;
	case 'n':
	    rc= RET_ABORT;
	    break;
	case 'c':
	case '>':
	    printf("(continue text entry)\n");
	    return;
	}
    }
#endif /* RIVER */

    /* OK, we really are going to exit now */

#ifdef YAPP
    /* In Yapp, an abort is signalled by a deleted buffer file */
    if (rc == RET_ABORT)
	unlink(filename);
    else
#endif /*YAPP*/
	/* Make file readable to others before exiting.  Yapp always needs
	 * this, PicoSpan only needs it when changing bulletins and such.
	 */
	chmod(filename,0644);

    if (in_cbreak) set_mode(0);

    exit(rc);
}


/* SUSP - ^Z handler. Trap it, reset modes, send ourselves a suspend, then,
 * when we restart, reread the modes (they may have been changed) and
 * then restore cbreak mode, if we were in it.  Finally use the longjump to
 * resume cleanly.
 */

#ifdef SIGTSTP
RETSIGTYPE susp()
{
    int was_cbreak= in_cbreak;
#ifdef HAVE_SIGACTION
    sigset_t set;

    /* Reset tty to standard mode */
    set_mode(0);

    /* Reset signal handler to default */
    sigact(SIGTSTP, SIG_DFL, NULL);

    /* Unblock suspend signal */
    sigemptyset(&set);
    sigaddset(&set,SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
#else
#ifdef HAVE_SIGMASK
    /* Block any further signals */
    int mask= sigblock(sigmask(SIGTSTP));
#endif

    /* Reset tty to standard mode */
    set_mode(0);

    /* Reset signal handler to default, and unblock it */
    signal(SIGTSTP,SIG_DFL);
#ifdef HAVE_SIGMASK
    sigsetmask(0);
#endif
#endif

    kill(0,SIGTSTP);

    /* STOP HERE */

#ifdef HAVE_SIGACTION
    sigact(SIGTSTP, susp, NULL);
#else
#ifdef HAVE_SIGMASK
    sigsetmask(mask);
#endif
    signal(SIGTSTP,(void (*)())susp);
#endif

    /* Restore cooked tty modes, re-reading them from system first in
     * case they were changed while we were suspended */
    initmodes();
    if (was_cbreak) set_mode(1);

    /* And back into the fray */
    resume= 1;
    junk_above= lines_above= 0;
    if (use_jump) longjmp(jenv,2);
}
#endif /*SIGTSTP*/
