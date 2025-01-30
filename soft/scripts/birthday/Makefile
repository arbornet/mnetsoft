# $Id: Makefile 1006 2010-12-08 02:38:52Z cross $

PREFIX=		/cyberspace
LIBEXECDIR=	$(PREFIX)/libexec
PROGS=          birthday.awk birthday.update

all:		$(PROGS)

$(PROGS):
		@true

depend doc indent lint print:
		@true

test:
		@true

install:	all
		install -c -m 755 $(PROGS) $(LIBEXECDIR)

uninstall:
		@for p in $(PROGS); \
		do \
			rm -f $(LIBEXECDIR)/$$p ; \
		done

diff:
		@for p in $(PROGS); \
		do \
			diff -u $(LIBEXECDIR)/$$p $$p || true; \
		done

clean nuke:
		@true
