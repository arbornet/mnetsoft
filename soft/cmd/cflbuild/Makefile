#

O=		o
PROG=		cflbuild
OBJS=		cflbuild.$O cflsub.$O
LDFLAGS= 
CFLAGS=		-I. -Wall
PREFIX=		/cyberspace

all:		$(PROG)

$(PROG):	$(OBJS)
		$(CC) $(CFLAGS) -o $(PROG) $(OBJS) $(LDFLAGS)

install:	all
		install -c -m 755 $(PROG) $(PREFIX)/bin

clean: 
		rm -f $(PROG) $(OBJS)
