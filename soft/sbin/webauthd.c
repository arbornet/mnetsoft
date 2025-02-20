#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *SOCKET = "/var/www/run/webauthd.sock";
const char *GROUP = "www";
const int PERM = 0660;

int mksock(void);

int
mksock()
{
	int sd;
	struct group *grp;
        struct sockaddr_un unsock;

	grp = getgrnam("www");
	if (grp == NULL) {
		fprintf(stderr, "Group unknown?!\n");
		exit(EXIT_FAILURE);
	}
	umask(0700);
	unlink(SOCKET);
	sd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	memset(&unsock, 0, sizeof unsock);
	strlcpy(unsock.sun_path, SOCKET, sizeof(unsock.sun_path));
	unsock.sun_family = AF_UNIX;
	if (bind(sd, (struct sockaddr *)&unsock, sizeof(unsock)) < 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}
	if (listen(sd, 255) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	chown(SOCKET, 0, grp->gr_gid);
	chmod(SOCKET, PERM);

	return sd;
}

void
sigchld(int sig)
{
	pid_t pid;
	int status;

	status = 0;
	pid = waitpid(-1, &status, 0);
	if (status != 0)
		fprintf(stderr, "pid %zu had strange exit status %d\n", pid, status);
}

char *
chomp(char *line)
{
	char *s, *e;

	s = strchr(line, '\n');
	if (s != NULL)
		*s = '\0';
	s = strchr(line, '\r');
	if (s != NULL)
		*s = '\0';
	for (s = line; isspace(*s); s++)
		;
	if (*s == '\0')
		return s;
	for (e = s + strlen(s) - 1; e > s && isspace(*e); --e)
		*e = '\0';

	return s;
}

const char B64TAB[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int
decode(const char *src, char *dst, size_t len)
{
	const char *s;
	char *d, *c;
	int32_t v;
	int k;

	v = 0;
	k = 0;
	s = src;
	d = dst;
	for (; *s != '\0' && *s != '=' && len > 1; s++) {
		c = strchr(B64TAB, *s);
		if (c == NULL)
			return 0;
		v <<= 6;
		v |= (c - B64TAB);
		if (++k == 4) {
			if (len <= 3)
				return 0;
			len -= 3;
			*d++ = (v >> 16) & 0xFF;
			*d++ = (v >>  8) & 0xFF;
			*d++ = (v >>  0) & 0xFF;
			k = 0;
			v = 0;
		}
	}
	if (k != 0)
		v <<= 6*(4-k);
	if (k >= 1) {
		if (len <= 1)
			return 0;
		*d++ = (v >> 16) & 0xFF;
		len--;
	}
	if (k >= 2) {
		if (len <= 1)
			return 0;
		*d++ = (v >> 8) & 0xFF;
		len--;
	}
	if (k == 3) {
		if (len <= 1)
			return 0;
		*d++ = (v >> 0) & 0xFF;
		len--;
	}
	if (len < 1)
		return 0;
	*d++ = '\0';

	return 1;
}

int
authenticate(const char *user, const char *pass)
{
	struct passwd *pw;
	char *hp;

	pw = getpwnam_shadow(user);
	if (pw == NULL)
		return 0;
	hp = crypt(pass, pw->pw_passwd);

	return strcmp(hp, pw->pw_passwd) == 0;
}

const char AHEADER[] = "Authorization:";
const char BASIC[] = "Basic";
const char s200[] = "HTTP/1.1 200 OK\r\n"
		    "Connection: close\r\n";
const char s401[] = "HTTP/1.1 401 Unauthorized\r\n"
		    "WWW-Authenticate: Basic realm=\"grex.org\"\r\n"
		    "Content-Length: 0\r\n"
		    "Connection: close\r\n";
const char crnl[] = "\r\n";

void
reply200(int sd, const char *user)
{
	const char header[] = "X-Username: ";

	write(sd, s200, sizeof(s200)-1);
	write(sd, header, sizeof(header)-1);
	write(sd, user, strlen(user));
	write(sd, crnl, sizeof(crnl)-1);
	write(sd, crnl, sizeof(crnl)-1);
}

void
reply401(int sd)
{
	write(sd, s401, sizeof(s401)-1);
	write(sd, crnl, sizeof(crnl)-1);
}

void
process(int sd)
{
	FILE *fp;
	char *p, *user, *pass, *tokens[4];
	int k;
	char line[1024], authdata[1024];

	fp = fdopen(sd, "r");
	while (fgets(line, sizeof line, fp) != NULL) {
		p = chomp(line);
		if (*p == '\0')
			break;
		if (strncasecmp(p, AHEADER, sizeof(AHEADER)-1) != 0)
			continue;
		for (k = 0; k < 4; ) {
			tokens[k] = strsep(&p, " \t");
			if (tokens[k] == NULL)
				break;
			if (*tokens[k] != '\0')
				k++;
		}
		if (k < 3 || strncasecmp(tokens[1], BASIC, sizeof(BASIC)-1) != 0)
			break;
		if (!decode(tokens[2], authdata, sizeof authdata))
			break;
		user = authdata;
		p = strchr(user, ':');
		if (p == NULL)
			break;
		*p = '\0';
		pass = p + 1;
		if (authenticate(user, pass))
			reply200(sd, user);
		else
			reply401(sd);
		break;
	}
	memset(line, 0, sizeof line);
	memset(authdata, 0, sizeof authdata);
	fclose(fp);
}

int
main()
{
	pid_t pid;
	int sd, csd;
	socklen_t clen;
	struct sockaddr_un csock;

	daemon(0, 1);
	sd = mksock();
	signal(SIGCHLD, sigchld);
	for (;;) {
		clen = sizeof csock;
		memset(&csock, 0, sizeof csock);
		csd = accept(sd, (struct sockaddr *)&csock, &clen);
		if (csd < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		pid = fork();
		if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {
			close(sd);
			process(csd);
			exit(EXIT_SUCCESS);
		}
		close(csd);
	}
}
