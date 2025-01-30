#
# Makefile for grex/config - install system files and do other tasks
# accordingly.
#
# $Id:$
#

ALIASES=	/etc/mail/aliases
DOVECOTCONF=	/etc/dovecot/local.conf
FSTAB=		/etc/fstab
FTPUSERS=	/etc/ftpusers
GROUP=		/etc/group
HOSTS=		/etc/hosts
#HTTPDCONF=	/var/www/conf/httpd.conf
GLOBALCSHRC=	/cyberspace/etc/global.cshrc
GLOBALLOGIN=	/cyberspace/etc/global.login
GLOBALPROFILE=	/cyberspace/etc/global.profile
IMMORTAL=	/cyberspace/etc/immortal
INETDCONF=	/etc/inetd.conf
LOGINCONF=	/etc/login.conf
LYNXCFG=	/etc/lynx.cfg
#MANCONF=	/usr/local/etc/man.d/grex.conf
MANCONF=	/etc/man.conf
MTREESPECIAL=	/etc/mtree/special
NAMEDCONF=	/var/named/etc/named.conf
NEWSYSLOGCONF=	/etc/newsyslog.conf
PFCONF=		/etc/pf.conf
POSTFIXACCESS=	/etc/postfix/access
POSTFIXMAINCF=	/etc/postfix/main.cf
POSTFIXMASTERCF= /etc/postfix/master.cf
RCCONFLOCAL=	/etc/rc.conf.local
RCLOCAL=	/etc/rc.local
RESOLVCONF=	/etc/resolv.conf
SHELLS=		/etc/shells
SKEL=		/cyberspace/etc/skel
SKELCFDIR=	/cyberspace/etc/skel/.cfdir
SKELCFONCE=	/cyberspace/etc/skel/.cfdir/.cfonce
SKELCSHRC=	/cyberspace/etc/skel/.cshrc
SKELLOGIN=	/cyberspace/etc/skel/.login
SKELMAILRC=	/cyberspace/etc/skel/.mailrc
SKELPWCHANGE=	/cyberspace/etc/skel/.needpwchange
SKELPROFILE=	/cyberspace/etc/skel/.profile
SSHDCONFIG=	/etc/ssh/sshd_config
SUDOERS=	/etc/sudoers
SYSCTLCONF=	/etc/sysctl.conf
SYSLOGCONF=	/etc/syslog.conf
THISCELL=	/etc/afs/ThisCell
ZONE=		/var/nsd/zones/master/grex.org


FILES=		$(ALIASES) $(DOVECOTCONF) $(FSTAB) $(FTPUSERS) \
		$(GROUP) $(HOSTS) $(HTTPDCONF) $(GLOBALCSHRC) \
		$(GLOBALLOGIN) $(GLOBALPROFILE) $(IMMORTAL) $(INETDCONF) \
		$(LOGINCONF) $(LYNXCFG) $(MANCONF) $(MTREESPECIAL) \
		$(NAMEDCONF) $(NEWSYSLOGCONF) $(PFCONF) \
		$(POSTFIXACCESS) $(POSTFIXMAINCF) $(POSTFIXMASTERCF) \
		$(RCCONFLOCAL) $(RCLOCAL) $(RESOLVCONF) $(SHELLS) $(SKEL) \
		$(SKELCFDIR) $(SKELCFONCE) $(SKELCSHRC) $(SKELLOGIN) \
		$(SKELMAILRC) $(SKELPWCHANGE) $(SKELPROFILE) $(SSHDCONFIG) \
		$(SUDOERS) $(SYSCTLCONF) $(SYSLOGCONF) $(THISCELL) $(ZONE)

all:
		@true

install:	$(FILES)

#
# Note: the output of "make diff" is suitable for input to:
# patch -V never -u -d root -p1
# Or, just run "make patch".
#
diff:
		@for file in $(FILES); do test -d $$file || diff -u root$$file $$file || true; done

patch:
		doas $(MAKE) diff | patch --posix -u -d root -p1

$(ALIASES):	root/$(ALIASES)
		@install -c -o root -g wheel -m 644 root/$(ALIASES) $(ALIASES)
		@newaliases
		@logger -s "Installed new $(ALIASES)"

$(DOVECOTCONF):	root/$(DOVECOTCONF)
		@install -c -o root -g wheel -m 644 root/$(DOVECOTCONF) $(DOVECOTCONF)
		@kill -HUP `ps auxww | grep 'dovecot$$' | grep -v grep | awk '{print $$2}'`
		@logger -s "Installed new $(DOVECOTCONF)"

$(FSTAB):	root/$(FSTAB)
		@install -c -o root -g wheel -m 644 root/$(FSTAB) $(FSTAB)
		@logger -s "Installed new $(FSTAB)"

$(FTPUSERS):	root/$(FTPUSERS)
		@install -c -o root -g wheel -m 644 root/$(FTPUSERS) $(FTPUSERS)
		@logger -s "Installed new $(FTPUSERS)"

$(GROUP):	root/$(GROUP)
		@install -c -o root -g treasurer -m 664 root/$(GROUP) $(GROUP)
		@logger -s "Installed new $(GROUP)"

$(HOSTS):	root/$(HOSTS)
		@install -c -o root -g wheel -m 644 root/$(HOSTS) $(HOSTS)
		@logger -s "Installed new $(HOSTS)"

$(HTTPDCONF):	root/$(HTTPDCONF)
		@install -c -o root -g bin -m 644 root/$(HTTPDCONF) $(HTTPDCONF)
		@apachectl restart
		@logger -s "Installed new $(HTTPDCONF)"

$(GLOBALCSHRC):	root/$(GLOBALCSHRC)
		@install -c -o root -g wheel -m 644 root/$(GLOBALCSHRC) $(GLOBALCSHRC)
		@logger -s "Installed new $(GLOBALCSHRC)"

$(GLOBALLOGIN):	root/$(GLOBALLOGIN)
		@install -c -o root -g wheel -m 644 root/$(GLOBALLOGIN) $(GLOBALLOGIN)
		@logger -s "Installed new $(GLOBALLOGIN)"

$(GLOBALPROFILE):	root/$(GLOBALPROFILE)
		@install -c -o root -g wheel -m 644 root/$(GLOBALPROFILE) $(GLOBALPROFILE)
		@logger -s "Installed new $(GLOBALPROFILE)"

$(IMMORTAL):	root/$(IMMORTAL)
		@install -c -o root -g wheel -m 644 root/$(IMMORTAL) $(IMMORTAL)
		@logger -s "Installed new $(IMMORTAL)"

$(INETDCONF):	root/$(INETDCONF)
		@install -c -o root -g wheel -m 644 root/$(INETDCONF) $(INETDCONF)
		@[ -f /var/run/inetd.pid ] && kill -HUP `cat /var/run/inetd.pid`
		@logger -s "Installed new $(INETDCONF)"

$(LOGINCONF):	root/$(LOGINCONF)
		@install -c -o root -g wheel -m 644 root/$(LOGINCONF) $(LOGINCONF)
		@logger -s "Installed new $(LOGINCONF)"

$(LYNXCFG):	root/$(LYNXCFG)
		@install -c -o root -g wheel -m 444 root/$(LYNXCFG) $(LYNXCFG)
		@logger -s "Installed new $(LYNXCFG)"

$(MANCONF):	root/$(MANCONF)
		@install -c -o root -g wheel -m 644 root/$(MANCONF) $(MANCONF)
		@logger -s "Installed new $(MANCONF)"

$(MTREESPECIAL):	root/$(MTREESPECIAL)
		@install -c -o root -g wheel -m 600 root/$(MTREESPECIAL) $(MTREESPECIAL)
		@logger -s "Installed new $(MTREESPECIAL)"

$(NAMEDCONF):	root/$(NAMEDCONF)
		@install -c -o root -g named -m 640 root/$(NAMEDCONF) $(NAMEDCONF)
		@rndc reload
		@logger -s "Installed new $(NAMEDCONF)"

$(NEWSYSLOGCONF):	root/$(NEWSYSLOGCONF)
		@install -c -o root -g wheel -m 644 root/$(NEWSYSLOGCONF) $(NEWSYSLOGCONF)
		@logger -s "Installed new $(NEWSYSLOGCONF)"

$(PFCONF):	root/$(PFCONF)
		@install -c -o root -g wheel -m 600 root/$(PFCONF) $(PFCONF)
		@pfctl -f $(PFCONF)
		@logger -s "Installed new $(PFCONF)"

$(POSTFIXACCESS):	root/$(POSTFIXACCESS)
		@install -c -o root -g wheel -m 644 root/$(POSTFIXACCESS) $(POSTFIXACCESS)
		@postmap hash:$(POSTFIXACCESS)
		@logger -s "Installed new $(POSTFIXACCESS)"

$(POSTFIXMAINCF):	root/$(POSTFIXMAINCF)
		@install -c -o root -g wheel -m 644 root/$(POSTFIXMAINCF) $(POSTFIXMAINCF)
		@postfix reload
		@logger -s "Installed new $(POSTFIXMAINCF)"

$(POSTFIXMASTERCF):	root/$(POSTFIXMASTERCF)
		@install -c -o root -g wheel -m 644 root/$(POSTFIXMASTERCF) $(POSTFIXMASTERCF)
		@postfix reload
		@logger -s "Installed new $(POSTFIXMASTERCF)"

$(RCCONFLOCAL):	root/$(RCCONFLOCAL)
		@install -c -o root -g wheel -m 644 root/$(RCCONFLOCAL) $(RCCONFLOCAL)
		@logger -s "Installed new $(RCCONFLOCAL)"

$(RCLOCAL):	root/$(RCLOCAL)
		@install -c -o root -g wheel -m 644 root/$(RCLOCAL) $(RCLOCAL)
		@logger -s "Installed new $(RCLOCAL)"

$(RESOLVCONF):	root/$(RESOLVCONF)
		@install -c -o root -g wheel -m 644 root/$(RESOLVCONF) $(RESOLVCONF)
		@logger -s "Installed new $(RESOLVCONF)"

$(SHELLS):	root/$(SHELLS)
		@install -c -o root -g wheel -m 644 root/$(SHELLS) $(SHELLS)
		@logger -s "Installed new $(SHELLS)"

$(SKEL):	root/$(SKEL)
		@test -d $(SKEL) || mkdir -p $(SKEL)
		@chown root:wheel $(SKEL)
		@chmod 755 $(SKEL)
		@logger -s "Installed new $(SKEL)"

$(SKELCFDIR):	root/$(SKELCFDIR)
		@test -d $(SKELCFDIR) || mkdir -p $(SKELCFDIR)
		@chown root:wheel $(SKELCFDIR)
		@chmod 755 $(SKELCFDIR)
		@logger -s "Installed new $(SKELCFDIR)"

$(SKELCFONCE):	root/$(SKELCFONCE) $(SKELCFDIR)
		@install -c -o root -g wheel -m 644 root/$(SKELCFONCE) $(SKELCFONCE)
		@logger -s "Installed new $(SKELCFONCE)"

$(SKELCSHRC):	root/$(SKELCSHRC)
		@install -c -o root -g wheel -m 644 root/$(SKELCSHRC) $(SKELCSHRC)
		@logger -s "Installed new $(SKELCSHRC)"

$(SKELLOGIN):	root/$(SKELLOGIN)
		@install -c -o root -g wheel -m 644 root/$(SKELLOGIN) $(SKELLOGIN)
		@logger -s "Installed new $(SKELLOGIN)"

$(SKELMAILRC):	root/$(SKELMAILRC)
		@install -c -o root -g wheel -m 644 root/$(SKELMAILRC) $(SKELMAILRC)
		@logger -s "Installed new $(SKELMAILRC)"

$(SKELPWCHANGE):	root/$(SKELPWCHANGE)
		@install -c -o root -g wheel -m 644 root/$(SKELPWCHANGE) $(SKELPWCHANGE)
		@logger -s "Installed new $(SKELPWCHANGE)"

$(SKELPROFILE):	root/$(SKELPROFILE)
		@install -c -o root -g wheel -m 644 root/$(SKELPROFILE) $(SKELPROFILE)
		@logger -s "Installed new $(SKELPROFILE)"

$(SSHDCONFIG):	root/$(SSHDCONFIG)
		@install -c -o root -g wheel -m 644 root/$(SSHDCONFIG) $(SSHDCONFIG)
		@[ -f /var/run/sshd.pid ] && kill -HUP `cat /var/run/sshd.pid`
		@logger -s "Installed new $(SSHDCONFIG)"

$(SUDOERS):	root/$(SUDOERS)
		@install -c -o root -g wheel -m 440 root/$(SUDOERS) $(SUDOERS)
		@logger -s "Installed new $(SUDOERS)

$(SYSLOGCONF):	root/$(SYSLOGCONF)
		@install -c -o root -g wheel -m 644 root/$(SYSLOGCONF) $(SYSLOGCONF)
		@[ -f /var/run/syslog.pid ] && kill -HUP `cat /var/run/syslog.pid`
		@logger -s "Installed new $(SYSLOGCONF)"

$(SYSCTLCONF):	root/$(SYSCTLCONF)
		@install -c -o root -g wheel -m 644 root/$(SYSCTLCONF) $(SYSCTLCONF)
		@logger -s "Installed new $(SYSCTLCONF)"

$(THISCELL):	root/$(THISCELL)
		@install -c -o root -g wheel -m 644 root/$(THISCELL) $(THISCELL)
		@logger -s "Installed new $(THISCELL)"

$(ZONE):	root/$(ZONE)
		@install -c -o root -g wheel -m 644 root/$(ZONE) $(ZONE)
		@rndc reload
		@logger -s "Installed new $(ZONE)"

#
# Other targets, mostly for managing ports.
#

packagelist:
		pkg_info -aI > packages/packages.list

plist:
		pkg_info -qPa > packages/plist

packageinfo:	packagelist plist
