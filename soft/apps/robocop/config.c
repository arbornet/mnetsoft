/* No processes owned by the following uids are ever to be killed */

long kperpage;
long pagesize;

int protect_uids[]= {
	0,	/* root */
	1,	/* daemon */
	2,	/* operator */
	3,	/* bin */
	27,	/* sshd */
	28,	/* _portmap */
	29,	/* _identd */
	30,	/* _rstatd */
	32,	/* _rusersd */
	33,	/* _fingerd */
	35,	/* _x11 */
	59,	/* _kdc */
	60,	/* _kadmin */
	62,	/* _spamd */
	66,	/* uucp */
	67,	/* www (httpd) */
	68,	/* _isakmpd */
	70,	/* named */
	71,	/* proxy */
	73,	/* _syslogd */
	74,	/* _pflogd */
	75,	/* _bgpd */
	76,	/* _tcpdump */
	81,	/* _afs */
	83,	/* _ntp */
	84,	/* _ftp */
	502,	/* mysql */
	504,	/* rt */
	506,	/* _spamdaemon */
	507,	/* _postfix */
	24594,	/* invent */
	32767,	/* nobody */
	-1};	/* END-OF-LIST */
