# $Id: Makefile 1599 2017-05-24 21:07:19Z cross $

PREFIX=		/cyberspace
BINDIR=		$(PREFIX)/bin
LIBDIR=		$(PREFIX)/lib
LIBDATADIR=	$(PREFIX)/libdata
LIBEXECDIR=	$(PREFIX)/libexec
PERL5LIBDIR=	$(LIBDATADIR)/perl5
SITEPERLDIR=	$(PERL5LIBDIR)/site_perl
PERLLIBS=	Grex.pm
LIBS=		$(PERLLIBS)
PROGS=		a.out bbs bbsguide bbsh board change change_plan \
                change_rseps change_write_perms clickify cols \
                debriefing delete easyfiglet faq figlet-faq \
                filebrowse forward gatherfolk gatherttyuse gloria \
		gloria.update grex-limits grex-members \
		grex-principles help hvcn identify-user ignore \
                inventory listal listcommand listcommands listgroup \
                lunch member menumore minutes mkhomepage mknames \
                motd noisefind noises party party-faq phrase picospan \
		pu recipe rot13 see set_write_perms.awk sigh staff \
                staffs support use_cfdir use_gate userdate vote \
		voters web_stat_page.awk why
LIBEXECPROGS=	nightly-backup roll_party_log rotate-web-logs \
		unspam-board unspam-board-talk unspam-rt-gate \
		unspam-staff unspam-staff-talk walk_motd

all:		$(LIBS) $(PROGS)

$(PERLLIBS):
		@true

$(PROGS):
		@true

depend doc indent lint print:
		@true

test:
		@true

install:	all
		[ -d $(SITEPERLDIR)/Grex ] || mkdir -p $(SITEPERLDIR)/Grex
		install -c -m 755 $(PERLLIBS) $(SITEPERLDIR)
		install -c -m 755 $(PROGS) $(BINDIR)
		install -c -m 755 $(LIBEXECPROGS) $(LIBEXECDIR)
		( cd birthday && $(MAKE) install )

uninstall:
		@for p in $(PROGS); \
		do \
			rm -f $(BINDIR)/$$p ; \
		done

diff:
		@for p in $(PROGS); \
		do \
			diff -u $(BINDIR)/$$p $$p || true; \
		done

clean nuke:
		@true
