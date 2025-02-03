# $Id: Makefile 1601 2017-05-25 19:22:38Z cross $

PREFIX=		/arbornet
PEOPLEBIN=	/suid/people
O=		o
A=		a
PROGS=		backspace checkgrps grexmore grexmoresh members \
		probe pwlookup realnames tail+2 twit whoa ylc zengame
FBSDPROGS=	laston watch
INSTALLPROGS=	newly-validated pickashell
SPEOPLEPROGS=	setmyshell
LIBS=		-L../libgrex -lgrex
STATIC=		-static
MAN1=		laston.1
CFLAGS=		-g -Wall -Werror -std=c99

all:		$(PROGS) $(SPEOPLEPROGS)

backspace:	backspace.$O
		$(CC) -o $@ backspace.$O $(LIBS)

checkgrps:	checkgrps.$O
		$(CC) -o $@ checkgrps.$O $(LIBS)

grexmoresh:	grexmoresh.$O
		$(CC) -o $@ grexmoresh.$O $(LIBS)

grexmore:	grexmore.$O
		$(CC) -o $@ grexmore.$O $(LIBS)

laston:		laston.$O
		$(CC) -o $@ laston.$O $(LIBS)

members:	members.$O
		$(CC) -o $@ members.$O $(LIBS)

probe:		probe.$O
		$(CC) -o $@ probe.$O $(LIBS)

pwlookup:	pwlookup.$O
		$(CC) -o $@ pwlookup.$O $(LIBS)

realnames:	realnames.$O
		$(CC) -o $@ realnames.$O $(LIBS)

setmyshell:	setmyshell.$O
		$(CC) -o $@ setmyshell.$O $(LIBS) $(STATIC)

tail+2:		tail+2.$O
		$(CC) -o $@ tail+2.$O $(LIBS)

twit:		twit.$O
		$(CC) -o $@ twit.$O $(LIBS)

watch:		watch.$O
		$(CC) -o $@ watch.$O $(LIBS)

whoa:		whoa.go
		go build whoa.go

ylc:		ylc.$O
		$(CC) -o $@ ylc.$O


zengame:	zengame.$O
		$(CC) -o $@ zengame.$O

test:		all
		@true

install:	test
		install -c -m 755 -s $(PROGS) $(PREFIX)/bin
		install -c -m 755 $(INSTALLPROGS) $(PREFIX)/bin
		for p in homedir fullname shell ; do \
			rm -f $(PREFIX)/bin/$$p && \
			ln -s $(PREFIX)/bin/pwlookup $(PREFIX)/bin/$$p ; \
		done
		install -c -m 644 $(MAN1) $(PREFIX)/man/man1
		install -c -m 4110 -o root -g people -s $(SPEOPLEPROGS) $(PEOPLEBIN)

uninstall:
		for p in $(PROGS); do rm -f $(PREFIX)/bin/$$p; done
		for m in $(MAN1); do rm -f $(PREFIX)/man/man1/$$m; done
		rm -f $(PREFIX)/bin/homedir
		rm -f $(PREFIX)/bin/fullname
		rm -f $(PREFIX)/bin/shell

depend docs indent lint print:
		@true

clean nuke:
		rm -f $(PROGS) $(SPEOPLEPROGS) *.o


.SUFFIXES:	.c .o .h

.c.o:
		$(CC) -I../libgrex $(CFLAGS) -c $<

# Dependencies follow.
#$(PROGS):	Makefile ../libgrex/libgrex.h ../libgrex/libgrex.$A
