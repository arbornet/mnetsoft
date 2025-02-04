#

GATEDIR=	gate/gate-2.06
PARTYDIR=	party/party-2.13
ORVILLEDIR=	orville/orville-write-2.55

PREFIX=		/arbornet
BINDIR=		$(PREFIX)/bin
ETCDIR=		$(PREFIX)/etc
LIBDIR=		$(PREFIX)/lib
LIBEXECDIR=	$(PREFIX)/libexec
MANDIR=		$(PREFIX)/man
SBINDIR=	$(PREFIX)/sbin
VARDIR=		$(PREFIX)/var

SUIDBIN=	/suid/bin

all:
	@echo 'Run make [configure|build|install|clean|distclean]'

configure:
	@echo 'Configuring gate....'
	(cd $(GATEDIR) && \
	 ./configure \
	    --prefix=$(PREFIX) \
	    --with-aspell \
	    --with-picospan \
	    --with-editor=nano)

	@echo 'Configuring party....'
	(cd $(PARTYDIR) && \
	 ./configure \
	    --prefix=$(PREFIX) \
	    --with-suid=party \
	    --bindir=$(SUIDBIN) \
	    --localstatedir=$(PREFIX)/var \
	    --sysconfdir=$(PREFIX)/etc)

	@echo 'Configuring orville write....'
	(cd $(ORVILLEDIR) && ./configure --prefix=$(PREFIX))

build:
	@echo 'Building gate....'
	(cd $(GATEDIR) && make)

	@echo 'Building party....'
	(cd $(PARTYDIR) && make)

	@echo 'Building orville write....'
	(cd $(ORVILLEDIR) && make)

install:
	@echo 'Installing gate....'
	(cd $(GATEDIR) && make install)

	@echo 'Installing party....'
	(cd $(PARTYDIR) && \
	    install -c -o party -g bin -m 4711 party $(SUIDBIN)/party && \
	    install -c -o party -g bin -m 755 \
	        ../party.wrapper $(BINDIR)/party && \
	    install -c -o root -g wheel -m 644 party.1 $(MANDIR)/man1 && \
	    install -c -o root -g wheel -m 755 \
	        ../roll_party_log $(LIBEXECDIR) && \
	    ln -fs $(SUIDBIN)/party $(BINDIR)/cafe && \
	    ln -fs $(SUIDBIN)/party $(BINDIR)/silicon && \
	    ln -fs $(SUIDBIN)/party $(BINDIR)/pwho && \
	    ( [ -d $(VARDIR)/party ] || mkdir $(VARDIR)/party ) && \
	    chown party:party $(VARDIR)/party && \
	    chmod 775 $(VARDIR)/party && \
	    ( [ -d $(VARDIR)/party/log ] || mkdir $(VARDIR)/party/log ) && \
	    chown party:party $(VARDIR)/party/log && \
	    chmod 775 $(VARDIR)/party/log && \
	    ( rm -f $(VARDIR)/party/log/partytmp; \
	        install -c -o party -g party -m 664 \
		    /dev/null $(VARDIR)/party/log/partytmp ) && \
	    ( [ -d $(ETCDIR)/party ] || mkdir $(ETCDIR)/party ) && \
	    chown party:party $(ETCDIR)/party && \
	    chmod 775 $(ETCDIR)/party && \
	    touch $(ETCDIR)/party/party.motd && \
	    for f in partytab chantab noisetab partyhlp party.welcome; do \
	        [ -f $(ETCDIR)/party/$$f ] || install \
		    -c -o party -g party -m 664 ../$$f $(ETCDIR)/party/$$f; \
	    done \
	)

	@echo 'Installing orville write....'
	(cd $(ORVILLEDIR) && \
	    install -c -o root -g tty -m 6711 write mesg amin $(SUIDBIN) && \
	    install -c -o root -g tty -m 2711 mesg amin $(SUIDBIN) && \
	    ln -sf $(SUIDBIN)/write $(BINDIR)/write && \
	    ln -sf $(SUIDBIN)/write $(BINDIR)/chat && \
	    ln -sf $(SUIDBIN)/write $(BINDIR)/jot && \
	    ln -sf $(SUIDBIN)/write $(BINDIR)/tel && \
	    ln -sf $(SUIDBIN)/write $(BINDIR)/telegram && \
	    ln -sf $(SUIDBIN)/mesg $(BINDIR)/messge && \
	    ln -sf $(SUIDBIN)/amin $(BINDIR)/amin && \
	    install -c -o root -g wheel -m 755 huh helpers $(BINDIR) && \
	    ( [ -f $(ETCDIR)/orville.conf ] || \
	        install -c -o root -g wheel -m 644 \
	            ../../../config/root/arbornet/etc/orville.conf \
		    $(ETCDIR) ) && \
	    ( [ -f $(LIBDIR)/write.help ] || \
	        install -c -o root -g wheel -m 755 \
	            ../write.help $(LIBDIR) ) && \
	    ( [ -f $(LIBDIR)/write.nohelper ] || \
	        install -c -o root -g wheel -m 755 \
	            ../write.nohelper $(LIBDIR) ) && \
	    ( rm -f $(VARDIR)/wrttmp $(VARDIR)/wrthist $(VARDIR)/WriteHelpLog; \
	        install -c -o root -g tty -m 660 \
		    /dev/null $(VARDIR)/wrttmp; \
	        install -c -o root -g tty -m 660 \
	            /dev/null $(VARDIR)/wrthist; \
	        install -c -o root -g tty -m 660 \
	            /dev/null $(VARDIR)/WriteHelpLog ) && \
	    for f in write.1 mesg.1 amin.1 huh.1 helpers.1; do \
	        install -c -o root -g wheel -m 644 $$f $(MANDIR)/man1/$$f; \
	    done \
	)

clean:
	@echo 'Cleaning gate....'
	(cd $(GATEDIR) && $(MAKE) clean && rm -f gate.1)

	@echo 'Cleaning party....'
	(cd $(PARTYDIR) && $(MAKE) clean)

	@echo 'Cleaning orville write....'
	(cd $(ORVILLEDIR) && $(MAKE) clean)

distclean:	clean
	@echo 'Distribution cleaning gate....'
	(cd $(GATEDIR) && $(MAKE) distclean && rm -f gate.1)

	@echo 'Distribution cleaning party....'
	(cd $(PARTYDIR) && $(MAKE) distclean)

	@echo 'Distribution cleaning orville write....'
	(cd $(ORVILLEDIR) && $(MAKE) distclean)
