# $Id: Makefile 1006 2010-12-08 02:38:52Z cross $

PREFIX=		/usr/local
O=		o
A=		a
PROG=		testrunner
OBJS=		testrunner.$O testutils.$O testgrexhash.$O testisshellok.$O \
		testcmpstring.$O testwarnerr.$O
CFLAGS=		-g -Wall

all:		$(PROG) testwarning

$(PROG):	$(OBJS) Makefile ../libgrex.$A
		$(CC) -g -o $(PROG) $(OBJS) -L../ -lgrex -lcrypto

testwarning:	testwarning.$O Makefile ../libgrex.$A
		$(CC) -g -o testwarning testwarning.$O -L../ -lgrex

test:		$(PROG) testwarning
		@./$(PROG)
		./testwarning

install:
		install -c -m 644 minunit.h $(PREFIX)/include

uninstall:
		rm -f $(PREFIX)/include/minunit.h

depend print:
		@true

clean nuke:
		rm -f $(PROG) $(OBJS) testwarning testwarning.$O

#
.SUFFIXES:	.c .o .h

.c.o:
		$(CC) -I../ $(CFLAGS) -c $<

# Dependencies follow.
$(OBJS):	Makefile minunit.h ../libgrex.h
testrunner.$O:	testlibgrex.h
