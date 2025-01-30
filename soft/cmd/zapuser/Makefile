#

SRC=	zapuser.c rmdir.c log.c member.c config.c homedir.c mvdir.c \
	mathom.c immortal.c mailfile.c outbound.c
OBJ=	zapuser.o rmdir.o log.o member.o config.o homedir.o mvdir.o \
	mathom.o immortal.o mailfile.o outbound.o

CC= gcc
#CPPFLAGS= -DDEBUG
CPPFLAGS=
CFLAGS= -g
#LIBS= -lhimail -lutil
LIBS= -lutil

zapuser: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

install:	zapuser
		install -c -o root -g wheel -m 0100 zapuser /cyberspace/sbin/zapuser

zapuser.o: zapuser.c zapuser.h
rmdir.o: rmdir.c zapuser.h
log.o: log.c zapuser.h
member.o: member.c zapuser.h
config.o: config.c zapuser.h
homedir.o: homedir.c zapuser.h
mvdir.o: mvdir.c zapuser.h
mathom.o: mathom.c zapuser.h
immortal.o: immortal.c zapuser.h
mailfile.o: mailfile.c zapuser.h
outbound.o: outbound.c zapuser.h

zaplog: zaplog.o config.o
	$(CC) $(CFLAGS) -o $@ zaplog.o config.o $(LIBS)

zapuser.tar: $(SRC) zaplog.c Makefile zapuser.h zapuser.conf zapuser.8
	tar cvf $@ $(SRC) zaplog.c Makefile zapuser.h zapuser.conf zapuser.8

clean:
	rm -f $(OBJ) zapuser zaplog

