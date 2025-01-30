# $Id: Makefile 1006 2010-12-08 02:38:52Z cross $

OBJ=		main.o active.o config.o killuser.o scanutmp.o sysproc.o \
		ttynumber.o usermem.o hen.o scanproc.o reboot.o restart.o \
		loguser.o snooze.o screen.o nprint.o ps.o
LDFLAGS=	 -lkvm
DESTDIR=	/cyberspace/libexec

# testing
#CFLAGS=	-g -DDEBUG -DMDEBUG -DVERBOSE -DLOGFILE='"/var/log/robocop.log"' \
#		-DPIDFILE='"/var/run/robocop.pid"'

# more realistic testing
#CFLAGS=	-O2 -s -DDEBUG -DLOGFILE='"/var/log/robotest.log"' \
#		-DPIDFILE='"/var/run/robotest.pid"'

# do kill_orphans, but not memory hogs
#CFLAGS=	-O2 -s -DMDEBUG -DLOGFILE='"/var/log/robocop.log"' \
#		-DPIDFILE='"/var/run/robocop.pid"'

# all on, verbose, not optimized
CFLAGS=		-g -s -DVERBOSE -DLOGFILE='"/var/log/robocop.log"' \
		-DPIDFILE='"/var/run/robocop.pid"'

# all on, optimized
#CFLAGS=	-O2 -s -DLOGFILE='"/var/log/robocop.log"' \
#		-DPIDFILE='"/var/run/robocop.pid"'

all:		robocop shockcop robochallenge

robocop:	$(OBJ)
		$(CC) -o $@ $(CFLAGS) $(OBJ) $(LDFLAGS)

robochallenge:	robochallenge.o reboot.o nprint.o ps.o config.o
		$(CC) -o $@ robochallenge.o reboot.o nprint.o ps.o config.o -lkvm

testps:		testps.o ps.o nprint.o config.o
		$(CC) -o $@ testps.o ps.o nprint.o config.o -lkvm

testroboserver:	testroboserver.o ps.o nprint.o reboot.o config.o
		$(CC) -o $@ testroboserver.o reboot.o ps.o nprint.o config.o -lkvm

shockcop:	shockcop.c
		$(CC) -o $@ $(CFLAGS) shockcop.c

ps_test:	ps_test.c
		$(CC) -o $@ -g ps_test.c $(LDFLAGS)

install:	all
		install -c -o root -g wheel -m 100 -s robocop $(DESTDIR)
		install -c -o root -g wheel -m 110 -s shockcop /cyberspace/sbin
		install -c -o root -g wheel -m 110 -s robochallenge /cyberspace/sbin
		install -c -o root -g wheel -m 550 grexctl /cyberspace/sbin

clean:
		rm -f *.o robocop shockcop ps_test testps testreboot robochallenge testroboserver

# dependencies.
main.o:		main.c config.h extern.h
active.o:	active.c config.h extern.h
killuser.o:	killuser.c config.h extern.h
scanutmp.o:	scanutmp.c config.h extern.h
sysproc.o:	sysproc.c config.h extern.h
ttynumber.o:	ttynumber.c config.h extern.h
usermem.o:	usermem.c config.h extern.h
hen.o:		hen.c config.h extern.h
screen.o:	screen.c config.h extern.h
scanproc.o:	scanproc.c config.h extern.h
restart.o:	restart.c config.h extern.h
loguser.o:	loguser.c config.h extern.h
snooze.o:	snooze.c config.h extern.h
