/*
 * Dynamically loaded shared objects for the resh commands.
 */

#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef	RESH_PATH
#define	RESH_PATH	"/cyberspace/grex/resh/lib/cmd"
#endif	/* !RESH_PATH */

int
dynegg(char *cmd)
{
	void *h, *f;
	int (*func)(char *), r;

	h = dlopen(RESH_PATH "/eggbasket.so", RTLD_NOW | RTLD_LOCAL);
	if (h == NULL)
		return(0);
	f = dlsym(h, "tryegg");
	if (f == NULL) {
		dlclose(h);
		return(0);
	}
	func = (int (*)(char *))f;
	r = func(cmd);
	dlclose(h);

	return(r);
}

int
run_command(char *cmd, int argc, char *argv[])
{
	char *p, path[128];
	void *h, *f;
	int (*func)(int, char *[]), r;

	p = strrchr(cmd, '/');
	if (p != NULL)
		cmd = p + 1;
	if (snprintf(path, sizeof(path), "%s/%s.so", RESH_PATH, cmd) > sizeof(path)) {
		warning("%s: command too long.", cmd);
		return(-1);
	}
	h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (h == NULL) {
		warning("%s: command not found: %s", cmd, dlerror());
		return(-1);
	}
	f = dlsym(h, "main");
	if (f == NULL) {
		perror(dlerror());
		dlclose(h);
		return(-1);
	}
	func = (int (*)(int, char *[]))f;
	r = func(argc, argv);
	if (dlclose(h) != 0)
		perror(dlerror());

	return(r);
}
