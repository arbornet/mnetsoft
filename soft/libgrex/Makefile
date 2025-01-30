# $Id: Makefile 1592 2017-05-21 03:21:35Z cross $

PREFIX=		/cyberspace
O=		o
A=		a
OBJS=		background.$O cmpstring.$O daemonize.$O \
		isshellok.$O progname.$O thread.$O warning.$O
OOBJS=		grexhash.$O
LIB=		libgrex.$A
HEADER=		libgrex.h
CFLAGS=		-g -Wall -Werror

all:		$(LIB)

$(LIB):		$(OBJS)
		ar crv $(LIB) $(OBJS)
		ranlib $(LIB)

test:		all
		( cd test && $(MAKE) $@ )

doc:
		doc++ -d html doc.dxx

depend indent lint print:
		@true

install:	all
		install -c -m 644 $(LIB) $(PREFIX)/lib
		install -c -m 644 $(HEADER) $(PREFIX)/include
		@( cd test && $(MAKE) $@ )

uninstall:
		rm -f $(PREFIX)/lib/$(LIB)
		rm -f $(PREFIX)/include/$(HEADER)

clean:
		rm -f $(OBJS)
		@( cd test && $(MAKE) $@ )

nuke:		clean
		rm -f $(LIB)
		rm -fr html

# Dependencies.
$(OBJS):	$(HEADER)
