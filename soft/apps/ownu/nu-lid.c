
/*
 *	pwd.c
 */
/*
 * Copyright (c) 1995 Marcus D. Watts  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Marcus D. Watts.
 * 4. The name of the developer may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * MARCUS D. WATTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "port/sys.h"
#include "newuser.h"
 
/*	the following three return TRUE if ok to use loginid */

int
check_lid(void)
{
	int len, c;
	char *p;

	len = strlen(loginid);
	if (len == 0) {
		fprintf(stderr, "Please try something just a trifle longer!\n");
		return 0;
	}
	if (len > _PW_NAME_LEN) {
		fprintf(stderr,
		    "Please try something shorter (%d characters max).\n",
		    _PW_NAME_LEN);
		return 0;
	}
	if (isdigit(*loginid)) {
		fprintf(stderr, "Sorry, first character must be alphabetical!\n");
		return 0;
	}
	for (p = loginid; (c = *p) != '\0'; ++p)
		if (islower(c))
			;
		else if (isdigit(c))
			;
		else {
			fprintf(stderr,
			    "Sorry, can't use %c (must be lowercase or digit)\n", c);
			return 0;
		}

	return 1;
}

/******************** loginid validation: mostly runs with locks on!! ****/

/*
 *	leaves lock around if ok: assuming next thing
 *	to do  is add_user()!
 *	returns RC=1 (and locked) if loginid is ok;
 *	returns RC=0 (and unlocked) if loginid is in use.
 *
 *	side-effect: calls setuserdir and so constructs userdir.
 */

int
lid_free(void)
{
	int rc;

	if (isalias(loginid) != 0) {
		fprintf (stderr,
"Sorry, %s is in use as a mail alias.  Try something else.\n", loginid);
		return 0;
	}
	if (*rpwd && is_reserved(loginid))
		return 0;
	set_lock();
	fprintf (stderr, "Please wait, ");
	rc = (check_pwd(passwd));
	if (rc && STREQ(passwd, ETCPASSWD) == 0)
		rc = check_pwd(ETCPASSWD);
	if (rc)
		setuserdir();
	if (rc && checkdirs)
		rc = check_usr();
	if (rc && !checkdirs)
		rc = check_thisone();
	if (!rc) un_lock();
	return rc;
}

static int
check_thisone(void)
{
	if (access(userdir, 0) < 0 && errno == ENOENT)
		return 1;
	fprintf(stderr,
"directory in use; can't use that loginid.  Sorry.\n");
	return 0;
}


/*
 * is_reserved returns a 1 iff the name exists & the user can't supply the pw
 */

static int
is_reserved(char *lid)
{
	return 0;

	FILE *fd;
	struct passwd *pwd;
	int rc;
	static char ok[MAXSTRINGSIZE];
	int s;
	char *clear;

	if (STREQ(lid, ok)) return 0;
	fd = fopen(rpwd, "r");
	if (!fd)
		die("cannot open reserved");
	rc = 0;
	while ((pwd = gpwent(fd)) != NULL)
	{
		if (STREQ(pwd->pw_name, lid) != 0) continue;
		rc = 1;
		break;
	}
	fclose(fd);
	if (pwd)
	{
		rloginid = pwd->pw_name;
		rfullname = pwd->pw_gecos;
		showinfo(NURESERVED);
		rloginid = 0;
		rfullname = 0;
		s = setterm(3);
		clear = get_line("Password", "");
		setterm(s);
		putc('\n', stderr);
		if (!*clear)
		{
			/**/
		}
		else if (strcmp(crypt(clear, pwd->pw_passwd), pwd->pw_passwd) == 0)
		{ /* it matched */
			strlcpy(ok, lid, MAXSTRINGSIZE);
			rc = 0;
		} else {
			fprintf (stderr,
"Your password didn't match; try another loginid.\n");
		}
	}
	return rc;
}
