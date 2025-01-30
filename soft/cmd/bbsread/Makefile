# define CONFLIST to full path name of conference list file.
# define CONFDIR to full path name of conference directory.
# define YAPP to be Yapp compatible instead of PicoSpan compatible.

CC=		gcc
CFLAGS=		-O2 -Wall -Werror -DCONFLIST='"/cyberspace/grex/bbs/conflist"' -DCONFDIR='"/cyberspace/grex/bbs/"' -DBSD
PROGS=		bbsread extract forget findconf partname
MAN1=		bbsread.1 extract.1 forget.1
BINDIR=		/cyberspace/bin
MAN1DIR=	/cyberspace/man/man1

SHAR=		Makefile arg.c atoh.c bbsread.c bbsread.h cfpath.c config.c \
		extract.c findconf.c forget.c match.c parse.c partname.c pf.c \
		range.c readitem.c rfp.c sum.c usys.c bbsread.1 extract.1 \
		forget.1

BBSREAD_OBJS=	bbsread.o arg.o atoh.o cfpath.o config.o match.o \
		parse.o pf.o range.o readitem.o rfp.o sum.o usys.o
EXTRACT_OBJS=	extract.o atoh.o cfpath.o match.o parse.o range.o \
		readitem.o sum.o
FORGET_OBJS=	forget.o cfpath.o config.o match.o parse.o pf.o
FINDCONF_OBJS=	findconf.o cfpath.o match.o
PARTNAME_OBJS=	partname.o cfpath.o config.o match.o

all:		$(PROGS)

bbsread:	$(BBSREAD_OBJS)
		$(CC) -o $@ $(BBSREAD_OBJS)

extract:	$(EXTRACT_OBJS)
		$(CC) -o $@ $(EXTRACT_OBJS)

forget:		$(FORGET_OBJS)
		$(CC) -o $@ $(FORGET_OBJS)

findconf:	$(FINDCONF_OBJS)
		$(CC) -o $@ $(FINDCONF_OBJS)

partname:	$(PARTNAME_OBJS)
		$(CC) -o $@ $(PARTNAME_OBJS)

bbsread.tar:	$(SHAR)
		tar cvf $@ $(SHAR)

bbsread.shar:	$(SHAR)
		shar $(SHAR) > bbsread.shar

tags:		$(SRCS) $(INCL)
		ctags $(SRCS) $(INCL)

clean:
		rm  -f *.o $(PROGS)

install:	all
		install -c -o root -g bin -m 711 $(PROGS) $(BINDIR)
		install -c -o root -g bin -m 644 $(MAN1) $(MAN1DIR)

# Dependencies.
bbsread.o:	bbsread.c bbsread.h
forget.o:	forget.c bbsread.h
extract.o:	extract.c bbsread.h
arg.o:		arg.c bbsread.h
cfpath.o:	cfpath.c bbsread.h
pf.o:		pf.c bbsread.h
config.o:	config.c bbsread.h
range.o:	range.c bbsread.h
readitem.o:	readitem.c bbsread.h
rfp.o:		rfp.c bbsread.h
sum.o:		sum.c bbsread.h
usys.o:		usys.c bbsread.h
