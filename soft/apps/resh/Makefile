# $Id: Makefile 1216 2011-12-28 07:39:53Z cross $

O=		o
D=		so

CC=		cc
RM=		rm
CFLAGS=		-g -I. -I/cyberspace/include
LDFLAGS=	-L/cyberspace/lib
LIBS=		-lgrex -lreadline -ltermcap

PROG=		resh
OBJS=		resh.$O dyncmd.$O read.$O util.$O token.$O
EGGS=		eggs/catalog.egg eggs/chad.egg eggs/cls.egg eggs/cmd.egg \
		eggs/dir.egg eggs/head.egg eggs/joshua.egg eggs/porter.egg \
		eggs/push.egg

SHOBJS=		bbs.$D cat.$D clear.$D eggs.$D fortune.$D help.$D \
		lc.$D ls.$D man.$D party.$D passwd.$D pg.$D ps.$D \
		pwd.$D validate.$D who.$D

all:		$(PROG) pg

$(PROG):	$(OBJS) $(SHOBJS)
		$(CC) -o $(PROG) $(OBJS) $(LDFLAGS) $(LIBS)

pg:		pg.o util.o
		$(CC) -o pg pg.o util.o $(LDFLAGS) $(LIBS)

eggs.c:		$(EGGS)
		./mkegg $(EGGS) > eggs.c

$(SHOBJS):

clean:
		$(RM) -f $(PROG) $(OBJS) $(SHOBJS) pg pg.o eggs.c

install:	all
		install -c $(SHOBJS) /cyberspace/grex/resh/lib/cmd
		mv /cyberspace/grex/resh/lib/cmd/eggs.so /cyberspace/grex/resh/lib/cmd/eggbasket.so
		install -c resh-help /cyberspace/grex/resh/lib
		install -c resh /usr/local/bin

.SUFFIXES:	.so

.c.$D:
		$(CC) $(CFLAGS) -shared -fPIC $< util.o -o $@ $(LDFLAGS) $(LIBS)
