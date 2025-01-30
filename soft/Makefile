# $Id: Makefile 1006 2010-12-08 02:38:52Z cross $

SUBDIRS=	libgrex cmd scripts

all doc indent lint test print depend install uninstall clean nuke:
		@for d in $(SUBDIRS); \
		do \
			( echo '==>' $$d && cd $$d && $(MAKE) $@ ) ; \
		done
