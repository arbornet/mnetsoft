# Vote Program

# Compiler
CC          = gcc
BASECFLAGS  = -O2 -Wall -Werror -pedantic -std=c99
BASELDFLAGS = 

# Local stuff - customize this section
LOCPROGS    = 
LOCCFLAGS   = 
LOCLDFLAGS  = 
LOCOBJS     = 
LOCHDRS	    = 

# Flags
CFLAGS	    = ${BASECFLAGS} ${LOCCFLAGS}
LDFLAGS     = ${BASELDFLAGS} ${LOCLDFLAGS}

all: board webballot webcast

vote.o: vote.h

board.o: board.c
board: board.o
	gcc board.o -o board

webballot.c: vote.h

webballot.o: vote.h
webballot: webballot.o vote.o
	gcc webballot.o vote.o -o webballot

#webbaltest.o: webballot.c vote.h
#	${CC} -DNOVAL ${CFLAGS} -c webballot.c -o webbaltest.o
#webbaltest: webbaltest.o vote.o
#	gcc webbaltest.o vote.o -o webbaltest

webcast.o: vote.h formvars.h webcast.c
webcast: webcast.o vote.o formvars.o
	gcc webcast.o vote.o formvars.o -o webcast

#webcasttest.o: webcast.c vote.h formvars.h
#	${CC} -DNOVAL ${CFLAGS} -c webcast.c -o webcasttest.o
#webcasttest: webcasttest.o vote.o formvars.o
#	gcc webcasttest.o vote.o formvars.o -o webcasttest

clean:
	rm -f *.o board webballot webcast date2epoch
