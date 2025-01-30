# MDWDIR is the path name to a directory with for Marcus Watt's tty-based
# newuser source in it.
#
MDWDIR= ../nu

# COPY is a command to use to copy a file from MDWDIR to our source dir.
# "cp" and "ln" are good choices.
#COPY= ln -f
COPY= cp -f

# CC - must be an ansi C compiler
#
# CFLAGS for webnewuser
#    -DSYSNAME      - what to call the system in various messages.
#    -DWELCOME_HTML - file to print upon success
#    -DNU_DIR       - directory where nuconfig and such like is kept.
#    -DWNU_LOG      - file to keep error log in.
#    -DGREX	    - compile for Grex version of newuser
#
# CFLAGS for newuser
#    -DTESTING      - disable the actual account installation.
#    -DBSDI         - compile for BSDI (M-Net version)
#    -DGREX	    - compile for Grex's shadow password system

# OpenBSD Grex live configuration:
CC= cc
CFLAGS= -g \
	-DGREX \
	-DHTTP_UID=67 \
	-Dopenbsd \
	-DSYSNAME='"Grex"' \
	-DNU_DIR='"/usr/noton/nu/"' \
	-DWNU_LOG='"/var/log/wnulog"' \
	-DWELCOME_HTML='"/usr/local/www/nu/welcome.html"' \
	-I../nu
LIB=	-ltermcap -lutil -lcrypto

# OpenBSD Grex test configuration:
#CC= cc
#CFLAGS= -g \
#	-DGREX \
#	-DTESTING \
#	-DHTTP_UID=67 \
#	-Dopenbsd \
#	-DSYSNAME='"Grex"' \
#	-DNU_DIR='"/usr/noton/nu/"' \
#	-DWNU_LOG='"/var/log/wnulog"' \
#	-DWELCOME_HTML='"/usr/local/www/nu/welcome.html"' 
#LIB= -ltermcap -lutil

# Web-Newuser source - M-Net Version:
WNUSRC=	wnu-main.c wnu-error.c wnu-keys.c wnu-getu.c wnu-parse.c wnu-login.c \
	cgi-decode.c cgi-output.c nu-lid.c
WNUOBJ=	wnu-main.o wnu-error.o wnu-keys.o wnu-getu.o wnu-parse.o wnu-login.o \
	cgi-decode.o cgi-output.o nu-lid.o

MDWLIB=	libnewuser.a

SRC= $(WNUSRC)
OBJ= $(WNUOBJ) $(MDWLIB)

wnu: $(OBJ)
	$(CC) -o wnu $(CFLAGS) $(OBJ) $(LIB)

../nu/libnewuser.a:
		(cd ../nu && make)

$(MDWLIB):	../nu/libnewuser.a
		cp ../nu/libnewuser.a .
		ar d $(MDWLIB) pwd.o
		ranlib $(MDWLIB)

print-terms: print-terms.o cgi-output.o
	$(CC) -o print-terms $(CFLAGS) print-terms.o cgi-output.o

print-terms.o: print-terms.c
wnu-main.o: wnu-main.c wnu.h ../nu/nu.h
wnu-error.o: wnu-error.c wnu.h
wnu-keys.o: wnu-keys.c ../nu/nu.h wnu.h
wnu-getu.o: wnu-getu.c wnu.h ../nu/nu.h
wnu-login.o: wnu-login.c wnu.h ../nu/nu.h
wnu-parse.o: wnu-parse.c wnu.h
wnu-junk.o: wnu-junk.c wnu.h
cgi-decode.o: cgi-decode.c
cgi-output.o: cgi-output.c

wnu.tar: MDW-CHANGES README $(WNUSRC) wnu.h nuconfig Makefile print-terms.c
	tar cvf wnu.tar README MDW-CHANGES $(WNUSRC) wnu.h nuconfig Makefile \
	print-terms.c

clean:
	rm -f nu.h map.h *.o $(MDWLIB) wnu print-terms
