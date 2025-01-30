/*
 * Do challenge and response reboots.
 * Hopefully, robocop will run enough that we can actually do this if
 * necessary; even if the system cannot fork, is the victim of a fork
 * bomb, or what have you.  This is our last-ditch effort to revive
 * Grex remotely.
 *
 * $Id: reboot.c 1006 2010-12-08 02:38:52Z cross $
 */

#include <sys/types.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <errno.h>
#include <fcntl.h>
#include <sha2.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmp.h>

#include "extern.h"
#include "config.h"

static unsigned char roboikey[SHA256_BLOCK_LENGTH];
static unsigned char robookey[SHA256_BLOCK_LENGTH];
static int nsd;

enum
{
	ROBOPORT = 77,			/* netrjs - kind of fitting */
	ROBOSERVERTIMEOUT = 15,
	MAXSTRLEN = 2 * SHA256_BLOCK_LENGTH + 1
};

static void
doreboot(void)
{
	reboot(RB_AUTOBOOT);
}

static void
rlog(const char *fmt, ...)
{
	FILE *fp;
	va_list ap;

	fp = fopen(LOGFILE, "a");
	if (fp == NULL)
		return;
	va_start(ap, fmt);
	vfprintf(fp, fmt, ap);
	va_end(ap);
	fprintf(fp, "\n");
	fclose(fp);
}

static void
setrobokey(unsigned char ikey[SHA256_BLOCK_LENGTH],
    unsigned char okey[SHA256_BLOCK_LENGTH],
    char rawkey[MAXSTRLEN])
{
	int fd, i;
	char key[SHA256_BLOCK_LENGTH];

	for (i = 0; i < SHA256_BLOCK_LENGTH; i++)
		key[i] = rawkey[i] ^ rawkey[i + SHA256_BLOCK_LENGTH];
	for (i = 0; i < SHA256_BLOCK_LENGTH; i++) {
		ikey[i] = key[i] ^ 0x5c;
		okey[i] = key[i] ^ 0x36;
	}
	memset(rawkey, 0, MAXSTRLEN);
	memset(key, 0, SHA256_BLOCK_LENGTH);
}

void
getrobokey(void)
{
	int fd;
	char *p, robokey[MAXSTRLEN];

	memset(robokey, '\0', MAXSTRLEN);
	memset(roboikey, 0, SHA256_BLOCK_LENGTH);
	memset(robookey, 0, SHA256_BLOCK_LENGTH);
	fd = open("/root/.robokey", O_RDONLY);
	if (fd < 0) {
		rlog("Can't open /root/.robokey: %s (remote reboot disabled).",
		    strerror(errno));
		return;
	}
	if (read(fd, robokey, sizeof(robokey) - 1) <= 0) {
		rlog("Bad robokey: %s (remote reboot disabled).",
		    strerror(errno));
		close(fd);
		return;
	}
	close(fd);
	p = strchr(robokey, '\n');
	if (p != NULL)
		*p = '\0';
	setrobokey(roboikey, robookey, robokey);
}

static void
genchallenge(char chalbuf[MAXSTRLEN])
{
	u_int32_t chal[8];	/* 256 bit challenge. */

	arc4random_stir();
	chal[0] = arc4random();
	chal[1] = arc4random();
	chal[2] = arc4random();
	chal[3] = arc4random();
	chal[4] = arc4random();
	chal[5] = arc4random();
	chal[6] = arc4random();
	chal[7] = arc4random();

	snprintf(chalbuf, MAXSTRLEN,
	    "%08lx%08lx%08lx%08lx%08lx%08lx%08lx%08lx",
	    chal[0], chal[1], chal[2], chal[3],
	    chal[4], chal[5], chal[6], chal[7]);
	chalbuf[MAXSTRLEN - 1] = '\0';
}

static void
genresponse(char response[MAXSTRLEN],
    unsigned char ikey[SHA256_BLOCK_LENGTH],
    unsigned char okey[SHA256_BLOCK_LENGTH],
    char challenge[MAXSTRLEN])
{
	SHA2_CTX inner, outer;
	unsigned char innerout[SHA256_DIGEST_LENGTH];

	memset(&inner, 0, sizeof(inner));
	memset(&outer, 0, sizeof(outer));

	SHA256Init(&inner);
	SHA256Update(&inner, ikey, SHA256_BLOCK_LENGTH);
	SHA256Update(&inner, challenge, MAXSTRLEN - 1);
	SHA256Final(innerout, &inner);

	SHA256Init(&outer);
	SHA256Update(&outer, okey, SHA256_BLOCK_LENGTH);
	SHA256Update(&outer, innerout, SHA256_DIGEST_LENGTH);
	SHA256End(&outer, response);
}

void
getresponse(char robokey[MAXSTRLEN], char challenge[MAXSTRLEN], char response[MAXSTRLEN])
{
	unsigned char ikey[SHA256_BLOCK_LENGTH];
	unsigned char okey[SHA256_BLOCK_LENGTH];

	setrobokey(ikey, okey, robokey);
	genresponse(response, ikey, okey, challenge);
}

static int
checkresponse(char *challenge, char *response)
{
	char expected[MAXSTRLEN];

	genresponse(expected, roboikey, robookey, challenge);
	if (strncmp("r=", response, 2) != 0)
		return(-1);
	if (strcmp(expected, response + 2) != 0) {
		rlog("challenge/response failed.  Expected %s, got %s\n", expected, response + 2);
		return(-1);
	}
	
	return(1);
}

static void
printandreboot(int sd)
{
	printprocs(sd);
	doreboot();
}

static void
printwho(int sd)
{
	int fd;
	struct utmp u;
	char tmpbuf[UT_LINESIZE + UT_NAMESIZE +  UT_HOSTSIZE + 128 + 1];

	fd = open(_PATH_UTMP, O_RDONLY);
	if (fd < 0)
		return;
	nwrite(sd, "---\r\n", strlen("---\r\n"));
	while (read(fd, &u, sizeof(u)) == sizeof(u)) {
		if (u.ut_name[0] == '\0')
			continue;
		memset(tmpbuf, 0, sizeof(tmpbuf));
		strlcpy(tmpbuf, "name: ", sizeof(tmpbuf));
		strncat(tmpbuf, u.ut_name, UT_NAMESIZE);
		strlcat(tmpbuf, "\n  ", sizeof(tmpbuf));
		strlcat(tmpbuf, "line: ", sizeof(tmpbuf));
		strncat(tmpbuf, u.ut_line, UT_LINESIZE);
		strlcat(tmpbuf, "\n  ", sizeof(tmpbuf));
		strlcat(tmpbuf, "host: ", sizeof(tmpbuf));
		strncat(tmpbuf, u.ut_host, UT_HOSTSIZE);
		strlcat(tmpbuf, "\n  ", sizeof(tmpbuf));
		strlcat(tmpbuf, "time: ", sizeof(tmpbuf));
		strlcat(tmpbuf, ctime(&u.ut_time), sizeof(tmpbuf));
		strlcat(tmpbuf, "  ", sizeof(tmpbuf));

		nwrite(sd, "- ", strlen("- "));
		nswrite(sd, tmpbuf, strlen(tmpbuf));
	}
	close(fd);
}

static void
kickrobo(int sd)
{
	nprint(sd, "k");
	shutdown(sd, SHUT_RDWR);
	close(sd);
	raise(SIGWAKE);
}

static void
restartrobo(int sd)
{
	int i;

	nprint(sd, "k");
	shutdown(sd, SHUT_RDWR);
	for (i = 0; i < sysconf(_SC_OPEN_MAX); i++)
		(void)close(i);
	execl("/usr/local/libexec/robocop", "robocop", NULL);
}

static void
quitrobo(int sd)
{
	nprint(sd, "k");
	shutdown(sd, SHUT_RDWR);
	raise(SIGTERM);
}

static void
doroboserver(int sd)
{
	char challenge[MAXSTRLEN];
	char response[MAXSTRLEN + 2];
	char cmd[120];

	memset(challenge, 0, MAXSTRLEN);
	memset(response, 0, MAXSTRLEN);
	genchallenge(challenge);

	if (nprint(sd, "") < 0) return;
	if (nread(sd, cmd, sizeof(cmd)) < 0) return;
	if (nprint(sd, "c=%s", challenge) < 0) return;
	if (nread(sd, response, sizeof(response)) < 0) return;
	if (checkresponse(challenge, response) < 0) {
		nprint(sd, "!");
		return;
	}
	rlog("authentication successful. command: %s", cmd);
	alarm(0);

	/* Check commands and do the associated action. */
	if (STREQ(cmd, "w")) printwho(sd);
	if (STREQ(cmd, "s")) restartrobo(sd);
	if (STREQ(cmd, "Q")) quitrobo(sd);
	if (STREQ(cmd, "r")) doreboot();
	if (STREQ(cmd, "p")) printprocs(sd);
	if (STREQ(cmd, "pr")) printandreboot(sd);
	if (STREQ(cmd, "a")) kickrobo(sd);
	nprint(sd, "k");
}

void
roboalarm(int signo)
{
	if (signo != SIGALRM)
		return;
	(void)close(nsd);
	raise(SIGWAKE);
}

void
roboserver(int sd)
{
	signal(SIGALRM, roboalarm);
	alarm(ROBOSERVERTIMEOUT);
	doroboserver(nsd);
	signal(SIGALRM, SIG_IGN);
}

int
robolisten(void)
{
	int sd, tmp;
	struct sockaddr_in sock;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		rlog("Can't open socket: %s.", strerror(errno));
		return(-1);
	}
	/* I don't care if the following succeeds or not. */
        tmp = 1;
        setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (void *)&tmp, sizeof(tmp));
	memset(&sock, 0, sizeof(sock));
	sock.sin_addr.s_addr	= htonl(INADDR_ANY);
	sock.sin_port		= htons(ROBOPORT);
	sock.sin_family		= AF_INET;

	if (bind(sd, (struct sockaddr *)&sock, sizeof(sock)) < 0) {
		rlog("Can't bind socket: %s.", strerror(errno));
		close(sd);
		return(-1);
	}
	if (listen(sd, 128) < 0) {
		rlog("Can't listen on socket: %s.", strerror(errno));
		close(sd);
		return(-1);
	}

	return(sd);
}

void
roborje(int sd)
{
	struct sockaddr_in sock;
	socklen_t clen;
	char buf[128];

	memset(&sock, 0, sizeof(sock));
	clen = sizeof(sock);
	nsd = accept(sd, (struct sockaddr *)&sock, &clen);
	if (nsd < 0) {
		rlog("accept failed: %s.", strerror(errno));
		return;
	}
	inet_ntop(AF_INET, &sock.sin_addr, buf, sizeof(buf));
	rlog("roboserver connection from %s", buf);
	roboserver(nsd);
	close(nsd);
}

void
testroboserver(void)
{
	char testkey[MAXSTRLEN];

	memset(testkey, '\0', MAXSTRLEN);
	strlcpy(testkey, "HERE LIES THE DEEP", MAXSTRLEN);
	setrobokey(roboikey, robookey, testkey);
	doroboserver(0);
}
