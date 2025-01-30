#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: Makefile 1336 2013-03-02 14:47:24Z cross $
#

O=		o
CC=		cc
OPSYS=		freebsd
#OPSYS=		openbsd
PNUHOME=	/cyberspace/pnewuser
CFLAGS=		-g -Wall -Werror -Wno-long-long -ansi -pedantic \
		-DPNUHOME='"$(PNUHOME)"' -D__LONG_LONG_SUPPORTED \
		-I/usr/local/include/tcl8.5
PROGS=		genbsalt nuseradd pnewuser sendmail setquota
NUOBJS=		nuseradd-$(OPSYS).$O tcl.$O
GBOBJS=		genbsalt.$O genbsalt-$(OPSYS).$O
SQOBJS=		setquota.$O

all:		$(PROGS)

nuseradd:	$(NUOBJS) Makefile
		$(CC) -o nuseradd $(NUOBJS) -L/usr/local/lib -ltcl85 -lm -lutil

setquota:	$(SQOBJS) sysquota.h Makefile
		$(CC) -o setquota $(SQOBJS)

genbsalt:	$(GBOBJS) Makefile
		$(CC) -o genbsalt $(GBOBJS)

pnewuser:	pnewuser.pl Makefile
		sed -e 's,@@PNUHOME@@,'"$(PNUHOME)"',g' < pnewuser.pl > pnewuser
		chmod +x pnewuser

clean:
		rm -f $(PROGS) $(NUOBJS) $(GBOBJS) $(SQOBJS)

.c.o:
		$(CC) $(CFLAGS) -c $<
