BINDIR=		/suid/bin

bank:		bank.o
		cc -o bank bank.o

install:	bank
		install -c -o bank -g games -m 4511 bank $(BINDIR)/bank
